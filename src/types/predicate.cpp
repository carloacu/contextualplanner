#include <contextualplanner/types/predicate.hpp>
#include <stdexcept>
#include <contextualplanner/types/setoftypes.hpp>
#include "expressionParsed.hpp"


namespace cp
{
namespace
{
void _parametersToStr(std::string& pStr,
                      const std::vector<Parameter>& pParameters,
                      const std::string& pSeparator)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += pSeparator;
    pStr += param.toStr();
  }
}

}

Predicate::Predicate(const std::string& pStr,
                     bool pStrPddlFormated,
                     const SetOfTypes& pSetOfTypes,
                     std::size_t pBeginPos,
                     std::size_t* pResPos)
  : name(),
    parameters(),
    fluent()
{
  std::size_t pos = pBeginPos;
  auto expressionParsed = pStrPddlFormated ?
        ExpressionParsed::fromPddl(pStr, pos) :
        ExpressionParsed::fromStr(pStr, pos);

  name = expressionParsed.name;
  for (auto& currArg : expressionParsed.arguments)
    if (currArg.followingExpression)
    {
      auto type = pSetOfTypes.nameToType(currArg.followingExpression->name);
      parameters.emplace_back(Parameter(currArg.name, type));
    }
  if (expressionParsed.followingExpression) {
    fluent = pSetOfTypes.nameToType(expressionParsed.followingExpression->name);
  }

  if (pResPos != nullptr)
  {
    if (pos <= pBeginPos)
      throw std::runtime_error("Failed to parse a predicate in str " + pStr.substr(pBeginPos, pStr.size() - pBeginPos));
    *pResPos = pos;
  }
}


std::string Predicate::toPddl() const
{
  std::string res = "(" + name;
  if (!parameters.empty())
  {
    res += " ";
    _parametersToStr(res, parameters, " ");
  }
  res += ")";
  if (fluent)
    res += " - " + fluent->name;
  return res;
}

std::string Predicate::toStr() const
 {
   auto res = name + "(";
   _parametersToStr(res, parameters, ", ");
   res += ")";
   if (fluent)
     res += " - " + fluent->name;
   return res;
 }


 bool Predicate::operator==(const Predicate& pOther) const
 {
   if (name == pOther.name && parameters == pOther.parameters)
   {
     if (!fluent && !pOther.fluent)
       return true;
     if (fluent && pOther.fluent && fluent->name == pOther.fluent->name)
       return true;
   }
   return false;
 }


} // !cp
