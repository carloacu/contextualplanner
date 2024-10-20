#include "expressionParsed.hpp"
#include <stdexcept>
#include <contextualplanner/types/factoptional.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/util/util.hpp>

namespace cp
{

namespace
{

bool _isASeparatorForTheBeginOfAFollowingExpression(char pChar)
{
  return pChar == '&' || pChar == '|' || pChar == '+' || pChar == '-' || pChar == '<' || pChar == '>';
}

bool _isASeparator(char pChar)
{
  return pChar == ' ' || pChar == '(' || pChar == ')' || pChar == ',' || pChar == '=' || pChar == '!' ||
      _isASeparatorForTheBeginOfAFollowingExpression(pChar);
}

}


ExpressionParsed ExpressionParsed::clone() const
{
  ExpressionParsed res;
  res.name = name;
  for (const auto& arg : arguments)
     res.arguments.emplace_back(arg.clone());
  res.value = value;
  res.isValueNegated = isValueNegated;
  res.isAFunction = isAFunction;
  res.separatorToFollowingExp = separatorToFollowingExp;
  if (followingExpression)
    res.followingExpression = std::make_unique<ExpressionParsed>(followingExpression->clone());
  return res;
}

std::string ExpressionParsed::toStr() const
{
  std::string res = name + "(";
  bool firstIteration = true;
  for (const auto& currArg : arguments)
  {
    if (firstIteration)
      res += ", ";
    else
      firstIteration = false;
    res += currArg.toStr();
  }
  return res + ")";
}


FactOptional ExpressionParsed::toFact(const Ontology& pOntology,
                                      const SetOfEntities& pEntities,
                                      const std::vector<Parameter>& pParameters,
                                      bool pIsOkIfFluentIsMissing) const
{
  std::vector<std::string> argumentStrs;
  for (auto& currArg : arguments)
    argumentStrs.emplace_back(currArg.name);

  std::string factName;
  bool isFactNegated = false;
  if (!name.empty() && name[0] == '!')
  {
    isFactNegated = true;
    factName = name.substr(1, name.size() - 1);
  }
  else
  {
    factName = name;
  }

  FactOptional res(isFactNegated, factName, argumentStrs, value, isValueNegated, pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing);
  if (value == Fact::undefinedValue.value && !res.isFactNegated)
  {
    res.isFactNegated = true;
    res.fact.setFluentValue(Entity::anyEntityValue());
  }
  return res;
}


ExpressionParsed ExpressionParsed::fromStr(const std::string& pStr,
                                           std::size_t& pPos)
{
  ExpressionParsed res;
  auto strSize = pStr.size();
  std::size_t beginOfNamePos = pPos;

  // extract name
  while (pPos < strSize)
  {
    if (beginOfNamePos == pPos)
    {
      if (pStr[pPos] == ' ')
      {
        ++pPos;
        beginOfNamePos = pPos;
        continue;
      }
    }
    else
    {
      if (_isASeparator(pStr[pPos]))
      {
        res.name = pStr.substr(beginOfNamePos, pPos - beginOfNamePos);
        break;
      }
    }
    ++pPos;
  }

  if (res.name.empty())
  {
    if (beginOfNamePos == pPos)
      throw std::runtime_error("Predicate is missing in expression: \"" + pStr + "\"");
    res.name = pStr.substr(beginOfNamePos, pPos - beginOfNamePos);
  }

  // extract arguments
  if (pPos < strSize)
  {
    if (pStr[pPos] == '(')
    {
      res.isAFunction = true;
      if (pPos + 1 < strSize && pStr[pPos + 1] != ')')
      {
        do
        {
          ++pPos;
          res.arguments.emplace_back(fromStr(pStr, pPos));
        }
        while (pStr[pPos] == ',');
      }
      else
      {
        ++pPos;
      }

      if (pStr[pPos] == ')')
        ++pPos;
      else
        throw std::runtime_error("Arguments parenthesis is not closed: \"" + pStr + "\"");
    }
  }

  // extract value
  if (pPos < strSize)
  {
    if (pStr[pPos] == '!')
    {
      res.isValueNegated = true;
      ++pPos;
    }
    if (pStr[pPos] == '=')
    {
      res.isAFunction = true;
      ++pPos;
      std::size_t beginOfValuePos = pPos;

      while (pPos < strSize)
      {
        if (_isASeparator(pStr[pPos]))
        {
          res.value = pStr.substr(beginOfValuePos, pPos - beginOfValuePos);
          break;
        }
        ++pPos;
      }

      if (res.value.empty() && pPos > beginOfValuePos)
        res.value = pStr.substr(beginOfValuePos, pPos - beginOfValuePos);
    }
  }

  // extract following expression
  while (pPos < strSize)
  {
    if (pStr[pPos] == ' ')
    {
      ++pPos;
      continue;
    }

    if (_isASeparatorForTheBeginOfAFollowingExpression(pStr[pPos]))
    {
      res.separatorToFollowingExp = pStr[pPos];
      ++pPos;
      res.followingExpression = std::make_unique<ExpressionParsed>(fromStr(pStr, pPos));
    }

    return res;
  }

  return res;
}


ExpressionParsed ExpressionParsed::fromPddl(const std::string& pStr,
                                            std::size_t& pPos)
{
  ExpressionParsed res;
  auto strSize = pStr.size();
  skipSpaces(pStr, pPos);
  if (pPos >= strSize)
    return res;

  if (pStr[pPos] == '(')
  {
    ++pPos;
    skipSpaces(pStr, pPos);
    std::size_t beginOfTokenPos = pPos;

    bool inName = true;
    while (pPos < strSize)
    {
      if (pStr[pPos] == ')')
      {
        ++pPos;
        break;
      }
      else if (!inName || isEndOfTokenSeparator(pStr[pPos]))
      {
        if (inName)
        {
          res.name = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
          if (res.name.empty())
          {
            res = fromPddl(pStr, pPos);
            return res;
          }
          inName = false;
          continue;
        }
        auto prePos = pPos;
        res.arguments.emplace_back(fromPddl(pStr, pPos));
        if (pPos > prePos)
          continue;
      }
      ++pPos;
    }

  }
  else
  {
    res.name = parseToken(pStr, pPos);
  }

  // extract following expression
  while (pPos < strSize)
  {
    if (pStr[pPos] == ' ')
    {
      ++pPos;
      continue;
    }

    if (_isASeparatorForTheBeginOfAFollowingExpression(pStr[pPos]))
    {
      res.separatorToFollowingExp = pStr[pPos];
      ++pPos;
      res.followingExpression = std::make_unique<ExpressionParsed>(fromPddl(pStr, pPos));
    }

    break;
  }

  skipSpaces(pStr, pPos);
  return res;
}

void ExpressionParsed::skipSpaces(const std::string& pStr,
                                  std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] != ' ' && pStr[pPos] != '\n' && pStr[pPos] != '\t')
      break;
    ++pPos;
  }
}

void ExpressionParsed::moveUntilEndOfLine(const std::string& pStr,
                                          std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == '\n')
      break;
    ++pPos;
  }
}

void ExpressionParsed::moveUntilClosingParenthesis(const std::string& pStr,
                                                   std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == ')')
      break;
    ++pPos;
  }
}


std::string ExpressionParsed::parseToken(const std::string& pStr,
                                         std::size_t& pPos)
{
  std::size_t beginOfTokenPos = pPos;
  auto res = parseTokenThatCanBeEmpty(pStr, pPos);
  if (res.empty())
  {
    auto strSize = pStr.size();
    throw std::runtime_error("Empty token in str " + pStr.substr(beginOfTokenPos, strSize - beginOfTokenPos));
  }
  return res;
}


std::string ExpressionParsed::parseTokenThatCanBeEmpty(const std::string& pStr,
                                                       std::size_t& pPos)
{
  auto strSize = pStr.size();
  skipSpaces(pStr, pPos);
  std::string res = "";
  std::size_t beginOfTokenPos = pPos;
  while (pPos < strSize)
  {
    if (pStr[pPos] == ';')
    {
      res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
      ExpressionParsed::moveUntilEndOfLine(pStr, pPos);
      ++pPos;
      return res;
    }

    if (isEndOfTokenSeparator(pStr[pPos]))
    {
      res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
      break;
    }
    ++pPos;
  }
  if (res.empty())
  {
    res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
  }
  return res;
}


bool ExpressionParsed::isEndOfTokenSeparator(char pChar)
{
  return pChar == ' ' || pChar == '\n' || pChar == ')' || pChar == '(';
}


} // !cp
