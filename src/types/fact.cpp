#include <contextualplanner/types/fact.hpp>
#include <memory>
#include <assert.h>
#include <contextualplanner/types/factoptional.hpp>


namespace cp
{
namespace {

void _parametersToStr(std::string& pStr,
                      const std::vector<FactOptional>& pParameters)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += ", ";
    pStr += param.toStr();
  }
}
}

const std::string Fact::anyValue = "*";
const FactOptional Fact::anyValueFact(Fact::anyValue);
std::string Fact::punctualPrefix = "~punctual~";
std::string Fact::unreachablePrefix = "~unreachable~";

Fact::Fact(const std::string& pStr,
           const char* pSeparatorPtr,
           bool* pIsFactNegatedPtr,
           std::size_t pBeginPos,
           std::size_t* pResPos)
  : name(),
    parameters(),
    value()
{
  auto resPos = fillFactFromStr(pStr, pSeparatorPtr, pBeginPos, pIsFactNegatedPtr);
  if (pResPos != nullptr)
    *pResPos = resPos;
}

Fact::~Fact()
{
}

bool Fact::operator<(const Fact& pOther) const
{
  if (name != pOther.name)
    return name < pOther.name;
  if (value != pOther.value)
    return value < pOther.value;
  std::string paramStr;
  _parametersToStr(paramStr, parameters);
  std::string otherParamStr;
  _parametersToStr(otherParamStr, pOther.parameters);
  return paramStr < otherParamStr;
}

bool Fact::operator==(const Fact& pOther) const
{
  return name == pOther.name && value == pOther.value && parameters == pOther.parameters;
}

bool Fact::isPunctual() const
{
  return name.compare(0, punctualPrefix.size(), punctualPrefix) == 0;
}

bool Fact::isUnreachable() const
{
  return name.compare(0, unreachablePrefix.size(), unreachablePrefix) == 0;
}

bool Fact::areEqualExceptAnyValues(const Fact& pOther) const
{
  if (!(name == pOther.name &&
        (value == pOther.value || value == anyValue || pOther.value == anyValue) &&
        parameters.size() == pOther.parameters.size()))
    return false;

  auto itParam = parameters.begin();
  auto itOtherParam = pOther.parameters.begin();
  while (itParam != parameters.end())
  {
    if (*itParam != *itOtherParam && *itParam != anyValueFact && *itOtherParam != anyValueFact)
      return false;
    ++itParam;
    ++itOtherParam;
  }
  return true;
}


std::string Fact::tryToExtractParameterValueFromExemple(
    const std::string& pParameterValue,
    const Fact& pOther) const
{
  if (name != pOther.name ||
      parameters.size() != pOther.parameters.size())
    return "";

  std::string res;
  if (value == pParameterValue)
    res = pOther.value;
   else if (value != pOther.value)
    return "";

  auto itParam = parameters.begin();
  auto itOtherParam = pOther.parameters.begin();
  while (itParam != parameters.end())
  {
    if (itParam->fact.name == pParameterValue)
      res = itOtherParam->fact.name;
    else if (itParam->fact.name != itOtherParam->fact.name)
      return "";

    auto subRes = itParam->fact.tryToExtractParameterValueFromExemple(pParameterValue, itOtherParam->fact);
    if (subRes != "")
      return subRes;
    ++itParam;
    ++itOtherParam;
  }
  return res;
}


void Fact::fillParameters(
    const std::map<std::string, std::string>& pParameters)
{
  auto itValueParam = pParameters.find(value);
  if (itValueParam != pParameters.end())
    value = itValueParam->second;

  for (auto& currParam : parameters)
  {
    auto& currFactParam = currParam.fact;
    if (currFactParam.value.empty() && currFactParam.parameters.empty())
    {
      auto itValueParam = pParameters.find(currFactParam.name);
      if (itValueParam != pParameters.end())
        currFactParam.name = itValueParam->second;
    }
    else
    {
      currFactParam.fillParameters(pParameters);
    }
  }
}

void Fact::fillParameters(
    const std::map<std::string, std::set<std::string>>& pParameters)
{
  auto itValueParam = pParameters.find(value);
  if (itValueParam != pParameters.end() && !itValueParam->second.empty())
    value = *itValueParam->second.begin();

  for (auto& currParam : parameters)
  {
    auto& currFactParam = currParam.fact;
    if (currFactParam.value.empty() && currFactParam.parameters.empty())
    {
      auto itValueParam = pParameters.find(currFactParam.name);
      if (itValueParam != pParameters.end() && !itValueParam->second.empty())
        currFactParam.name = *itValueParam->second.begin();
    }
    else
    {
      currFactParam.fillParameters(pParameters);
    }
  }
}

std::string Fact::toStr() const
{
  std::string res = name;
  if (!parameters.empty())
  {
    res += "(";
    _parametersToStr(res, parameters);
    res += ")";
  }
  if (!value.empty())
    res += "=" + value;
  return res;
}


Fact Fact::fromStr(const std::string& pStr,
                   bool* pIsFactNegatedPtr)
{
  return Fact(pStr, nullptr, pIsFactNegatedPtr);
}


std::size_t Fact::fillFactFromStr(
    const std::string& pStr,
    const char* pSeparatorPtr,
    std::size_t pBeginPos,
    bool* pIsFactNegatedPtr)
{
  static const char separatorOfParameters = ',';
  std::size_t pos = pBeginPos;
  while (pos < pStr.size())
  {
    if (pStr[pos] == ' ')
    {
      ++pos;
      continue;
    }
    if (pStr[pos] == '!')
    {
      if (pIsFactNegatedPtr != nullptr)
        *pIsFactNegatedPtr = true;
      ++pos;
      continue;
    }
    if ((pSeparatorPtr != nullptr && pStr[pos] == *pSeparatorPtr) || pStr[pos] == ')')
      return pos;

    bool insideParenthesis = false;
    auto beginPos = pos;
    while (pos < pStr.size())
    {
      if (!insideParenthesis && ((pSeparatorPtr != nullptr && pStr[pos] == *pSeparatorPtr) || pStr[pos] == ' ' || pStr[pos] == ')'))
        break;
      if (pStr[pos] == '(' || pStr[pos] == separatorOfParameters)
      {
        insideParenthesis = true;
        if (name.empty())
          name = pStr.substr(beginPos, pos - beginPos);
        ++pos;
        parameters.emplace_back(pStr, &separatorOfParameters, pos, &pos);
        beginPos = pos;
        continue;
      }
      if (pStr[pos] == ')' || pStr[pos] == '=')
      {
        insideParenthesis = false;
        if (name.empty())
          name = pStr.substr(beginPos, pos - beginPos);
        ++pos;
        beginPos = pos;
        continue;
      }
      ++pos;
    }
    if (name.empty())
      name = pStr.substr(beginPos, pos - beginPos);
    else
      value = pStr.substr(beginPos, pos - beginPos);
  }
  return pos;
}


bool Fact::replaceParametersByAny(const std::vector<std::string>& pParameters)
{
  bool res = false;
  for (const auto& currParam : pParameters)
  {
    for (auto& currFactParam : parameters)
    {
      if (currFactParam == currParam)
      {
        currFactParam = anyValueFact;
        res = true;
      }
    }
    if (value == currParam)
    {
      value = anyValue;
      res = true;
    }
  }
  return res;
}


bool Fact::isInFacts(const std::set<Fact>& pFacts,
    bool pParametersAreForTheFact,
    std::map<std::string, std::set<std::string>>& pNewParameters,
    const std::map<std::string, std::set<std::string>>* pParametersPtr,
    bool pCanModifyParameters,
    bool* pTriedToMidfyParametersPtr) const
{
  bool res = false;
  for (const auto& currFact : pFacts)
    if (isInFact(currFact, pParametersAreForTheFact, pNewParameters, pParametersPtr,
                 pCanModifyParameters, pTriedToMidfyParametersPtr))
      res = true;
  return res;
}


bool Fact::isInFact(const Fact& pFact,
    bool pParametersAreForTheFact,
    std::map<std::string, std::set<std::string>>& pNewParameters,
    const std::map<std::string, std::set<std::string>>* pParametersPtr,
    bool pCanModifyParameters,
    bool* pTriedToMidfyParametersPtr) const
{
  if (pFact.name != name ||
      pFact.parameters.size() != parameters.size())
    return false;

  auto doesItMatch = [&](const std::string& pFactValue, const std::string& pValueToLookFor) {
    if (pFactValue == pValueToLookFor)
      return true;

    if (pParametersPtr != nullptr)
    {
      auto itParam = pParametersPtr->find(pFactValue);
      if (itParam != pParametersPtr->end())
      {
        if (!itParam->second.empty())
          return itParam->second.count(pValueToLookFor) > 0;
        if (pCanModifyParameters)
          pNewParameters[pFactValue].insert(pValueToLookFor);
        else if (pTriedToMidfyParametersPtr != nullptr)
          *pTriedToMidfyParametersPtr = true;
        return true;
      }
    }
    return false;
  };

  {
    bool doesParametersMatches = true;
    auto itFactParameters = pFact.parameters.begin();
    auto itLookForParameters = parameters.begin();
    while (itFactParameters != pFact.parameters.end())
    {
      if (*itFactParameters != *itLookForParameters)
      {
        if (!itFactParameters->fact.parameters.empty() ||
            !itFactParameters->fact.value.empty() ||
            !itLookForParameters->fact.parameters.empty() ||
            !itLookForParameters->fact.value.empty() ||
            (!pParametersAreForTheFact && !doesItMatch(itFactParameters->fact.name, itLookForParameters->fact.name)) ||
            (pParametersAreForTheFact && !doesItMatch(itLookForParameters->fact.name, itFactParameters->fact.name)))
          doesParametersMatches = false;
      }
      ++itFactParameters;
      ++itLookForParameters;
    }
    if (!doesParametersMatches)
      return false;
  }

  if (pParametersAreForTheFact)
  {
    if (doesItMatch(value, pFact.value))
      return true;
  }
  else
  {
    if (doesItMatch(pFact.value, value))
      return true;
  }
  return false;
}

void Fact::replaceFactInParameters(const cp::Fact& pOldFact,
                                   const Fact& pNewFact)
{
  for (auto& currParameter : parameters)
  {
    if (currParameter.fact == pOldFact)
      currParameter.fact = pNewFact;
    else
      currParameter.fact.replaceFactInParameters(pOldFact, pNewFact);
  }
}


void Fact::splitFacts(
    std::vector<std::pair<bool, cp::Fact>>& pFacts,
    const std::string& pStr,
    char pSeparator)
{
  std::size_t pos = 0u;
  bool isFactNegated = false;
  std::unique_ptr<cp::Fact> currFact;
  while (pos < pStr.size())
  {
    currFact = std::make_unique<cp::Fact>(pStr, &pSeparator, &isFactNegated, pos, &pos);
    ++pos;
    if (!currFact->name.empty())
    {
      pFacts.emplace_back(isFactNegated, std::move(*currFact));
      isFactNegated = false;
      currFact = std::unique_ptr<cp::Fact>();
    }
  }
  if (currFact && !currFact->name.empty())
    pFacts.emplace_back(isFactNegated, std::move(*currFact));
}


void Fact::splitFactOptional(
    std::vector<cp::FactOptional>& pFactsOptional,
    const std::string& pStr,
    char pSeparator)
{
  std::size_t pos = 0u;
  while (pos < pStr.size())
  {
    auto currFact = std::make_unique<cp::FactOptional>(pStr, &pSeparator, pos, &pos);
    ++pos;
    if (!currFact->fact.name.empty())
      pFactsOptional.emplace_back(std::move(*currFact));
  }
}

} // !cp
