#include <contextualplanner/types/condition.hpp>
#include <optional>
#include <sstream>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/worldstate.hpp>
#include <contextualplanner/util/util.hpp>
#include "expressionParsed.hpp"

namespace cp
{
namespace
{
const char* _equalsFunctionName = "equals";
const char* _equalsCharFunctionName = "=";
const char* _existsFunctionName = "exists";
const char* _notFunctionName = "not";
const char* _superiorFunctionName = ">";
const char* _inferiorFunctionName = "<";
const char* _andFunctionName = "and";
const char* _orFunctionName = "or";


bool _forEachValueUntil(const std::function<bool (const Entity&)>& pValueCallback,
                        bool pUntilValue,
                        const Condition& pCondition,
                        const WorldState& pWorldState,
                        const std::map<Parameter, std::set<Entity>>* pParametersPtr)
{
  if (pParametersPtr == nullptr || pParametersPtr->empty())
  {
    auto fluentOpt = pCondition.getFluent(pWorldState);
    if (fluentOpt)
      return pValueCallback(*fluentOpt);
  }

  std::list<std::map<Parameter, Entity>> paramPossibilities;
  unfoldMapWithSet(paramPossibilities, *pParametersPtr);
  for (auto& currParamPoss : paramPossibilities)
  {
    auto condToExtractValue = pCondition.clone(&currParamPoss);
    auto fluentOpt = condToExtractValue->getFluent(pWorldState);
    if (fluentOpt && pValueCallback(*fluentOpt) == pUntilValue)
      return pUntilValue;
  }

  auto* factCondPtr = pCondition.fcFactPtr();
  if (factCondPtr != nullptr &&
      (!factCondPtr->factOptional.fact.fluent() || factCondPtr->factOptional.fact.fluent()->isAnyValue()))
  {
    pWorldState.iterateOnMatchingFactsWithoutFluentConsideration(
          [&pValueCallback](const Fact& pFact) {
      if (pFact.fluent())
        pValueCallback(*pFact.fluent());
      return false;
    },
    factCondPtr->factOptional.fact, // pFact
    *pParametersPtr); // Parameters to consider as any value of set is empty, otherwise it is a filter
  }

  return !pUntilValue;
}


void _forEach(const std::function<void (const Entity&, const Fact*)>& pValueCallback,
              const Condition& pCondition,
              const WorldState& pWorldState,
              const std::map<Parameter, std::set<Entity>>* pParametersPtr)
{
  if (pParametersPtr == nullptr || pParametersPtr->empty())
  {
    auto fluentOpt = pCondition.getFluent(pWorldState);
    if (fluentOpt)
      pValueCallback(*fluentOpt, nullptr);
    return;
  }

  std::list<std::map<Parameter, Entity>> paramPossibilities;
  unfoldMapWithSet(paramPossibilities, *pParametersPtr);
  for (auto& currParamPoss : paramPossibilities)
  {
    auto factToExtractValue = pCondition.clone(&currParamPoss);
    auto fluentOpt = factToExtractValue->getFluent(pWorldState);
    if (fluentOpt)
      pValueCallback(*fluentOpt, nullptr);
  }

  auto* factCondPtr = pCondition.fcFactPtr();
  if (factCondPtr != nullptr &&
      (!factCondPtr->factOptional.fact.fluent() || factCondPtr->factOptional.fact.fluent()->isAnyValue()))
  {
    pWorldState.iterateOnMatchingFactsWithoutFluentConsideration(
          [&pValueCallback](const Fact& pFact) {
      if (pFact.fluent())
        pValueCallback(*pFact.fluent(), &pFact);
      return false;
    },
    factCondPtr->factOptional.fact, // pFact
    *pParametersPtr); // Parameters to consider as any value of set is empty, otherwise it is a filter
  }
}

bool _areEqual(
    const std::unique_ptr<Condition>& pCond1,
    const std::unique_ptr<Condition>& pCond2)
{
  if (!pCond1 && !pCond2)
    return true;
  if (pCond1 && pCond2)
    return *pCond1 == *pCond2;
  return false;
}


std::unique_ptr<Condition> _expressionParsedToCondition(const ExpressionParsed& pExpressionParsed,
                                                        const Ontology& pOntology,
                                                        const SetOfEntities& pEntities,
                                                        const std::vector<Parameter>& pParameters,
                                                        bool pIsOkIfFluentIsMissing)
{
  std::unique_ptr<Condition> res;

  auto nodeType = ConditionNodeType::AND;
  if (pExpressionParsed.followingExpression)
  {
    if (pExpressionParsed.separatorToFollowingExp == '+')
      nodeType = ConditionNodeType::PLUS;
    else if (pExpressionParsed.separatorToFollowingExp == '-')
      nodeType = ConditionNodeType::MINUS;
    else if (pExpressionParsed.separatorToFollowingExp == '>')
      nodeType = ConditionNodeType::SUPERIOR;
    else if (pExpressionParsed.separatorToFollowingExp == '<')
      nodeType = ConditionNodeType::INFERIOR;
    else if (pExpressionParsed.separatorToFollowingExp == '|')
      nodeType = ConditionNodeType::OR;
  }

  if ((pExpressionParsed.name == _equalsFunctionName || pExpressionParsed.name == _equalsCharFunctionName) &&
      pExpressionParsed.arguments.size() == 2)
  {
    auto leftOperand = _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true);
    auto* leftFactPtr = leftOperand->fcFactPtr();

    const auto& rightOperandExp = *(++pExpressionParsed.arguments.begin());
    if (leftFactPtr != nullptr && !leftFactPtr->factOptional.isFactNegated &&
        rightOperandExp.arguments.empty() &&
        !rightOperandExp.followingExpression && rightOperandExp.value == "")
    {
      if (rightOperandExp.name == Fact::undefinedValue.value)
      {
        leftFactPtr->factOptional.isFactNegated = true;
        leftFactPtr->factOptional.fact.setFluentValue(Entity::anyEntityValue());
        res = std::make_unique<ConditionFact>(std::move(*leftFactPtr));
      }
      else if (pExpressionParsed.name == _equalsCharFunctionName && !rightOperandExp.isAFunction &&
               rightOperandExp.name != "")
      {
        leftFactPtr->factOptional.fact.setFluent(
              Entity::fromUsage(rightOperandExp.name, pOntology, pEntities, pParameters));
        res = std::make_unique<ConditionFact>(std::move(*leftFactPtr));
      }
    }

    if (!res)
    {
      auto rightOperand = _expressionParsedToCondition(rightOperandExp, pOntology, pEntities, pParameters, true);
      res = std::make_unique<ConditionNode>(ConditionNodeType::EQUALITY,
                                            std::move(leftOperand), std::move(rightOperand));
    }
  }
  else if (pExpressionParsed.name == _existsFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    std::shared_ptr<Type> paramType;
    if (firstArg.followingExpression)
      paramType = pOntology.types.nameToType(firstArg.followingExpression->name);
    Parameter existsParameter(firstArg.name, paramType);
    auto newParameters = pParameters;
    newParameters.push_back(existsParameter);
    res = std::make_unique<ConditionExists>(existsParameter,
                                            _expressionParsedToCondition(*(++itArg), pOntology, pEntities, newParameters, false));
  }
  else if (pExpressionParsed.name == _notFunctionName &&
           pExpressionParsed.arguments.size() == 1)
  {
    auto& expNegationned = pExpressionParsed.arguments.front();

    res = _expressionParsedToCondition(expNegationned, pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing);
    if (res)
    {
       auto* factPtr = res->fcFactPtr();
       if (factPtr != nullptr)
         factPtr->factOptional.isFactNegated = !factPtr->factOptional.isFactNegated;
       else
         res = std::make_unique<ConditionNot>(std::move(res));
    }
  }
  else if (pExpressionParsed.name == _superiorFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::SUPERIOR,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if (pExpressionParsed.name == _inferiorFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::INFERIOR,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if ((pExpressionParsed.name == _andFunctionName || pExpressionParsed.name == _orFunctionName) &&
           pExpressionParsed.arguments.size() >= 2)
  {
    auto listNodeType = pExpressionParsed.name == _andFunctionName ? ConditionNodeType::AND : ConditionNodeType::OR;
    std::list<std::unique_ptr<Condition>> elts;
    for (auto& currExp : pExpressionParsed.arguments)
      elts.emplace_back(_expressionParsedToCondition(currExp, pOntology, pEntities, pParameters, false));

    res = std::make_unique<ConditionNode>(listNodeType, std::move(*(--(--elts.end()))), std::move(elts.back()));
    elts.pop_back();
    elts.pop_back();

    while (!elts.empty())
    {
      res = std::make_unique<ConditionNode>(listNodeType, std::move(elts.back()), std::move(res));
      elts.pop_back();
    }
  }
  else
  {
    if (pExpressionParsed.arguments.empty() && pExpressionParsed.value == "")
    {
      try {
        res = std::make_unique<ConditionNumber>(lexical_cast<int>(pExpressionParsed.name));
      }  catch (...) {}
    }

    if (!res)
    {
      bool isOkIfFluentIsMissing = pIsOkIfFluentIsMissing || nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::INFERIOR;
      res = std::make_unique<ConditionFact>(pExpressionParsed.toFact(pOntology, pEntities, pParameters, isOkIfFluentIsMissing));
    }
  }

  if (pExpressionParsed.followingExpression)
  {
    res = std::make_unique<ConditionNode>(nodeType,
                                          std::move(res),
                                          _expressionParsedToCondition(*pExpressionParsed.followingExpression, pOntology, pEntities, pParameters, false));
  }

  return res;
}


bool _existsIsTrueRec(std::map<Parameter, std::set<Entity>>& pLocalParamToValue,
                      std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                      const Condition& pCondition,
                      const WorldState& pWorldState)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    bool res = false;
    std::map<Parameter, std::set<Entity>> newParameters;

    const auto& factToOfCondition = factOfConditionPtr->factOptional.fact;
    const auto& factAccessorsToFacts = pWorldState.factsMapping();
    res = factToOfCondition.isInOtherFactsMap(factAccessorsToFacts, true, &newParameters,
                                              pConditionParametersToPossibleArguments, &pLocalParamToValue) || res;

    if (pConditionParametersToPossibleArguments != nullptr)
      applyNewParams(*pConditionParametersToPossibleArguments, newParameters);
    return res;
  }

  auto* nodeOfConditionPtr = pCondition.fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand)
  {
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::AND)
      return _existsIsTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->leftOperand, pWorldState) &&
          _existsIsTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->rightOperand, pWorldState);
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::OR)
      return _existsIsTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->leftOperand, pWorldState) ||
          _existsIsTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->rightOperand, pWorldState);

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::EQUALITY)
    {
      std::map<Entity, std::map<Parameter, std::set<Entity>>> leftOpPossibleValuesToParams;
      auto* leftOpFactPtr = nodeOfConditionPtr->leftOperand->fcFactPtr();
      if (leftOpFactPtr != nullptr)
      {
        auto& leftOpFact = *leftOpFactPtr;

        pWorldState.iterateOnMatchingFactsWithoutFluentConsideration([&](const Fact& pFact){
          if (pFact.fluent())
          {
            auto& newParams = leftOpPossibleValuesToParams[*pFact.fluent()];
            if (pConditionParametersToPossibleArguments != nullptr)
            {
              for (auto& currArg : *pConditionParametersToPossibleArguments)
              {
                auto argValue = leftOpFact.factOptional.fact.tryToExtractArgumentFromExampleWithoutFluentConsideration(currArg.first, pFact);
                if (argValue)
                  newParams[currArg.first].insert(*argValue);
              }
            }
          }
          return false;
        }, leftOpFact.factOptional.fact, pLocalParamToValue, pConditionParametersToPossibleArguments);
      }

      bool res = false;
      auto* rightOpFactPtr = nodeOfConditionPtr->rightOperand->fcFactPtr();
      if (rightOpFactPtr != nullptr)
      {
        auto& rightOpFact = *rightOpFactPtr;

        std::map<Parameter, std::set<Entity>> newParameters;
        pWorldState.iterateOnMatchingFactsWithoutFluentConsideration([&](const Fact& pFact){
          if (pFact.fluent())
          {
            auto itToLeftPoss = leftOpPossibleValuesToParams.find(*pFact.fluent());
            if (itToLeftPoss != leftOpPossibleValuesToParams.end())
            {
              if (pConditionParametersToPossibleArguments != nullptr)
              {
                if (!itToLeftPoss->second.empty())
                {
                  for (auto& currArg : itToLeftPoss->second)
                    newParameters[currArg.first].insert(currArg.second.begin(), currArg.second.end());
                }
                else
                {
                  for (auto& currArg : *pConditionParametersToPossibleArguments)
                  {
                    auto argValue = rightOpFact.factOptional.fact.tryToExtractArgumentFromExampleWithoutFluentConsideration(currArg.first, pFact);
                    if (argValue)
                      newParameters[currArg.first].insert(*argValue);
                  }
                }
              }
              res = true;
            }
          }
          return res && pConditionParametersToPossibleArguments == nullptr;
        }, rightOpFact.factOptional.fact, pLocalParamToValue, pConditionParametersToPossibleArguments);

        if (pConditionParametersToPossibleArguments != nullptr)
          applyNewParams(*pConditionParametersToPossibleArguments, newParameters);
      }

      return res;
    }
  }
  return false;
}


void _existsExtractPossRec(std::map<Parameter, std::set<Entity>>& pLocalParamToValue,
                           const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
                           const Condition& pCondition,
                           const SetOfFact& pFacts,
                           const Fact& pFactFromEffect,
                           const Parameter& pParameter,
                           bool pIsNegated)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    const auto& factOfConditionOpt = factOfConditionPtr->factOptional;
    if (factOfConditionOpt.isFactNegated != pIsNegated ||
        !factOfConditionOpt.fact.areEqualWithoutAnArgConsideration(pFactFromEffect, pParameter.name))
    {
      std::map<Parameter, std::set<Entity>> newParameters;
      factOfConditionOpt.fact.isInOtherFactsMap(pFacts, true, &newParameters, &pConditionParametersToPossibleArguments, &pLocalParamToValue);
    }
    return;
  }

  auto* nodeOfConditionPtr = pCondition.fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand &&
      (nodeOfConditionPtr->nodeType == ConditionNodeType::AND || nodeOfConditionPtr->nodeType == ConditionNodeType::OR))
  {
    _existsExtractPossRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->leftOperand, pFacts, pFactFromEffect, pParameter, pIsNegated);
    _existsExtractPossRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->rightOperand, pFacts, pFactFromEffect, pParameter, pIsNegated);
  }
}

}


std::unique_ptr<Condition> Condition::fromStr(const std::string& pStr,
                                              const Ontology& pOntology,
                                              const SetOfEntities& pEntities,
                                              const std::vector<Parameter>& pParameters)
{
  if (pStr.empty())
    return {};
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);
  return _expressionParsedToCondition(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::set<FactOptional> Condition::getFactToIgnoreInCorrespondingEffect() const
{
  std::set<FactOptional> res;
  forAll([&](const FactOptional& pFactOptional, bool pIgnoreFluent) {
    if (!pIgnoreFluent)
      res.insert(pFactOptional);
  });
  return res;
}


std::string ConditionNode::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                 bool pPrintAnyFluent) const
{
  bool printAnyFluent = pPrintAnyFluent && nodeType != ConditionNodeType::EQUALITY &&
      nodeType != ConditionNodeType::SUPERIOR && nodeType != ConditionNodeType::INFERIOR;

  std::string leftOperandStr;
  if (leftOperand)
    leftOperandStr = leftOperand->toStr(pFactWriterPtr, printAnyFluent);
  std::string rightOperandStr;
  if (rightOperand)
    rightOperandStr = rightOperand->toStr(pFactWriterPtr, printAnyFluent);

  switch (nodeType)
  {
  case ConditionNodeType::AND:
    return leftOperandStr + " & " + rightOperandStr;
  case ConditionNodeType::OR:
    return leftOperandStr + " | " + rightOperandStr;
  case ConditionNodeType::EQUALITY:
    return std::string(_equalsFunctionName) + "(" + leftOperandStr + ", " + rightOperandStr + ")";
  case ConditionNodeType::PLUS:
    return leftOperandStr + " + " + rightOperandStr;
  case ConditionNodeType::MINUS:
    return leftOperandStr + " - " + rightOperandStr;
  case ConditionNodeType::SUPERIOR:
    return leftOperandStr + ">" + rightOperandStr;
  case ConditionNodeType::INFERIOR:
    return leftOperandStr + "<" + rightOperandStr;
  }
  return "";
}


ConditionNode::ConditionNode(ConditionNodeType pNodeType,
                             std::unique_ptr<Condition> pLeftOperand,
                             std::unique_ptr<Condition> pRightOperand)
  : Condition(),
    nodeType(pNodeType),
    leftOperand(std::move(pLeftOperand)),
    rightOperand(std::move(pRightOperand))
{
}

bool ConditionNode::hasFact(const Fact& pFact) const
{
  return (leftOperand && leftOperand->hasFact(pFact)) ||
      (rightOperand && rightOperand->hasFact(pFact));
}


bool ConditionNode::containsFactOpt(const FactOptional& pFactOptional,
                                    const std::map<Parameter, std::set<Entity>>& pFactParameters,
                                    const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                                    const std::vector<Parameter>& pConditionParameters,
                                    bool pIsWrappingExpressionNegated) const
{
  return (leftOperand && leftOperand->containsFactOpt(pFactOptional, pFactParameters, pOtherFactParametersPtr, pConditionParameters, pIsWrappingExpressionNegated)) ||
      (rightOperand && rightOperand->containsFactOpt(pFactOptional, pFactParameters, pOtherFactParametersPtr, pConditionParameters, pIsWrappingExpressionNegated));
}


void ConditionNode::forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
                           bool pIsWrappingExpressionNegated,
                           bool pIgnoreFluent) const
{
  bool ignoreFluent = pIgnoreFluent || (nodeType != ConditionNodeType::AND && nodeType != ConditionNodeType::OR);
  if (leftOperand)
    leftOperand->forAll(pFactCallback, pIsWrappingExpressionNegated, ignoreFluent);
  if (rightOperand)
    rightOperand->forAll(pFactCallback, pIsWrappingExpressionNegated, ignoreFluent);
}


bool ConditionNode::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (nodeType == ConditionNodeType::AND || nodeType == ConditionNodeType::OR)
  {
    if (leftOperand && leftOperand->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pFactFromEffect,
                                                                             pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                                             pConditionParametersToPossibleArguments, pIsWrappingExpressionNegated))
      return true;
    if (rightOperand && rightOperand->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pFactFromEffect,
                                                                               pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                                               pConditionParametersToPossibleArguments, pIsWrappingExpressionNegated))
      return true;
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = *leftFactPtr;
      if (nodeType == ConditionNodeType::EQUALITY)
      {
        if (leftFact.factOptional.fact.areEqualWithoutFluentConsideration(pFactFromEffect, &pFactFromEffectParameters, pFactFromEffectTmpParametersPtr))
        {
          return _forEachValueUntil(
                [&](const Entity& pValue)
          {
            auto factToCheck = leftFact.factOptional.fact;
            factToCheck.setFluent(pValue);
            return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck));
          }, true, *rightOperand, pWorldState, &pConditionParametersToPossibleArguments);
        }
        else
        {
          auto* rightFactPtr = rightOperand->fcFactPtr();
          if (rightFactPtr != nullptr)
          {
            const auto& rightFact = *rightFactPtr;
            if (rightFact.factOptional.fact.areEqualWithoutFluentConsideration(pFactFromEffect, &pFactFromEffectParameters, pFactFromEffectTmpParametersPtr))
            {
              return _forEachValueUntil(
                    [&](const Entity& pValue)
              {
                auto factToCheck = rightFact.factOptional.fact;
                factToCheck.setFluent(pValue);
                return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck));
              }, true, *leftOperand, pWorldState, &pConditionParametersToPossibleArguments);
            }
          }
        }
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::INFERIOR)
      {
         return pDoesConditionFactMatchFactFromEffect(leftFact.factOptional);
      }
    }
  }
  return false;
}



bool ConditionNode::untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                               const WorldState& pWorldState) const
{
  if (nodeType == ConditionNodeType::AND || nodeType == ConditionNodeType::OR)
  {
    if (leftOperand && !leftOperand->untilFalse(pFactCallback, pWorldState))
      return false;
    if (rightOperand && !rightOperand->untilFalse(pFactCallback, pWorldState))
      return false;
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = *leftFactPtr;
      if (nodeType == ConditionNodeType::EQUALITY)
      {
        auto factToCheck = leftFact.factOptional.fact;
        factToCheck.setFluent(rightOperand->getFluent(pWorldState));
        return pFactCallback(FactOptional(factToCheck));
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::INFERIOR)
      {
         return pFactCallback(leftFact.factOptional);
      }
    }
  }
  return true;
}


bool ConditionNode::isTrue(const WorldState& pWorldState,
                           const std::set<Fact>& pPunctualFacts,
                           const std::set<Fact>& pRemovedFacts,
                           std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                           bool* pCanBecomeTruePtr,
                           bool pIsWrappingExpressionNegated) const
{
  if (nodeType == ConditionNodeType::AND)
  {
    bool canBecomeTrue = false;
    if (pCanBecomeTruePtr == nullptr)
      pCanBecomeTruePtr = &canBecomeTrue;

    if (leftOperand && !leftOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
    {
      // Sometimes for negation of fact with parameter we need to check in the inverse order
      if (pCanBecomeTruePtr != nullptr && *pCanBecomeTruePtr)
      {
        if (rightOperand && !rightOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
          return false;

        if (leftOperand && !leftOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
          return false;
        return true;
      }
      return false;
    }
    if (rightOperand && !rightOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
      return false;
  }
  else if (nodeType == ConditionNodeType::OR)
  {
    bool canBecomeTrue = false;
    if (pCanBecomeTruePtr == nullptr)
      pCanBecomeTruePtr = &canBecomeTrue;

    if (leftOperand && leftOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
      return true;
    if (rightOperand && rightOperand->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, pIsWrappingExpressionNegated))
      return true;
    return false;
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = leftFactPtr->factOptional.fact;

      if (nodeType == ConditionNodeType::EQUALITY)
      {
        const auto& factAccessorsToFacts = pWorldState.factsMapping();
        bool res = false;
        std::map<Parameter, std::set<Entity>> newParameters;
        _forEach([&](const Entity& pValue, const Fact* pFromFactPtr)
        {
          auto factToCheck = leftFactPtr->factOptional.fact;
          factToCheck.setFluent(pValue);
          bool subRes = false;
          if (factToCheck.isPunctual())
            subRes = pPunctualFacts.count(factToCheck) != 0;
          else
            subRes = factToCheck.isInOtherFactsMap(factAccessorsToFacts, true, &newParameters, pConditionParametersToPossibleArguments);

          // Try to resolve the parameters
          if (subRes && pFromFactPtr != nullptr &&
              pConditionParametersToPossibleArguments != nullptr &&
              !pConditionParametersToPossibleArguments->empty())
          {
            auto* rightFactPtr = rightOperand->fcFactPtr();
            if (rightFactPtr != nullptr)
            {
              for (auto& currArg : *pConditionParametersToPossibleArguments)
                if (currArg.second.empty())
                {
                  auto value = rightFactPtr->factOptional.fact.tryToExtractArgumentFromExampleWithoutFluentConsideration(currArg.first, *pFromFactPtr);
                  if (value)
                    currArg.second.insert(*value);
                }
            }
          }

          res = subRes || res;
        }, *rightOperand, pWorldState, pConditionParametersToPossibleArguments);

        if (pConditionParametersToPossibleArguments != nullptr)
          applyNewParams(*pConditionParametersToPossibleArguments, newParameters);
        if (!pIsWrappingExpressionNegated)
          return res;
        return !res;
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::INFERIOR)
      {
        auto* rightNbPtr = rightOperand->fcNbPtr();
        if (rightNbPtr != nullptr)
        {
          const auto& factsMapping = pWorldState.factsMapping();
          auto leftFactMatchingInWs = factsMapping.find(leftFact);
          for (const auto& currWsFact : leftFactMatchingInWs)
          {
            if (currWsFact.fluent() &&
                leftFact.areEqualWithoutFluentConsideration(currWsFact))
            {
              bool res = compIntNb(currWsFact.fluent()->value, rightNbPtr->nb, nodeType == ConditionNodeType::SUPERIOR);
              if (!pIsWrappingExpressionNegated)
                return res;
              return !res;
            }
          }
        }
      }
    }
  }
  return !pIsWrappingExpressionNegated;
}

bool ConditionNode::canBecomeTrue(const WorldState& pWorldState,
                                  const std::vector<Parameter>& pParameters,
                                  bool pIsWrappingExpressionNegated) const
{
  if (nodeType == ConditionNodeType::AND)
  {
    if (leftOperand && !leftOperand->canBecomeTrue(pWorldState, pParameters, pIsWrappingExpressionNegated))
      return pIsWrappingExpressionNegated;
    if (rightOperand && !rightOperand->canBecomeTrue(pWorldState, pParameters, pIsWrappingExpressionNegated))
      return pIsWrappingExpressionNegated;
  }
  else if (nodeType == ConditionNodeType::OR)
  {
    if (leftOperand && leftOperand->canBecomeTrue(pWorldState, pParameters, pIsWrappingExpressionNegated))
      return !pIsWrappingExpressionNegated;
    if (rightOperand && rightOperand->canBecomeTrue(pWorldState, pParameters, pIsWrappingExpressionNegated))
      return !pIsWrappingExpressionNegated;
    return pIsWrappingExpressionNegated;
  }
  else if (nodeType == ConditionNodeType::EQUALITY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    auto* rightFactPtr = rightOperand->fcFactPtr();
    if (leftFactPtr != nullptr && rightFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional.fact;
      factToCheck.setFluent(pWorldState.getFactFluent(rightFactPtr->factOptional.fact));
      return pWorldState.canFactBecomeTrue(factToCheck, pParameters);
    }
  }
  return true;
}

bool ConditionNode::operator==(const Condition& pOther) const
{
  auto* otherNodePtr = pOther.fcNodePtr();
  return otherNodePtr != nullptr &&
      nodeType == otherNodePtr->nodeType &&
      _areEqual(leftOperand, otherNodePtr->leftOperand) &&
      _areEqual(rightOperand, otherNodePtr->rightOperand);
}

std::optional<Entity> ConditionNode::getFluent(const WorldState& pWorldState) const
{
  if (nodeType == ConditionNodeType::PLUS)
  {
    auto leftValue = leftOperand->getFluent(pWorldState);
    auto rightValue = rightOperand->getFluent(pWorldState);
    return plusIntOrStr(leftValue, rightValue);
  }
  if (nodeType == ConditionNodeType::MINUS)
  {
    auto leftValue = leftOperand->getFluent(pWorldState);
    auto rightValue = rightOperand->getFluent(pWorldState);
    return minusIntOrStr(leftValue, rightValue);
  }
  return {};
}


std::unique_ptr<Condition> ConditionNode::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                bool pInvert) const
{
  if (!pInvert)
    return std::make_unique<ConditionNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>());

  std::optional<ConditionNodeType> invertedNodeOpt;
  switch (nodeType) {
  case ConditionNodeType::AND:
    invertedNodeOpt.emplace(ConditionNodeType::OR);
    break;
  case ConditionNodeType::OR:
    invertedNodeOpt.emplace(ConditionNodeType::AND);
    break;
  }
  if (invertedNodeOpt)
    return std::make_unique<ConditionNode>(
          *invertedNodeOpt,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, true) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, true) : std::unique_ptr<Condition>());

  return std::make_unique<ConditionNot>(
        std::make_unique<ConditionNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>()));
}




std::string ConditionExists::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                   bool pPrintAnyFluent) const
{
  std::string conditionStr;
  if (condition)
    conditionStr = condition->toStr(pFactWriterPtr);
  return std::string(_existsFunctionName) + "(" + parameter.toStr() + ", " + conditionStr + ")";
}

ConditionExists::ConditionExists(const Parameter& pParameter,
                                 std::unique_ptr<Condition> pCondition)
  : Condition(),
    parameter(pParameter),
    condition(std::move(pCondition))
{
}

bool ConditionExists::hasFact(const Fact& pFact) const
{
  return condition && condition->hasFact(pFact);
}


bool ConditionExists::containsFactOpt(const FactOptional& pFactOptional,
                                      const std::map<Parameter, std::set<Entity>>& pFactParameters,
                                      const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                                      const std::vector<Parameter>& pConditionParameters,
                                      bool pIsWrappingExpressionNegated) const
{
  return condition && condition->containsFactOpt(pFactOptional, pFactParameters, pOtherFactParametersPtr, pConditionParameters, pIsWrappingExpressionNegated);
}

void ConditionExists::forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
                             bool pIsWrappingExpressionNegated,
                             bool pIgnoreFluent) const
{

  if (condition)
    condition->forAll(pFactCallback, pIsWrappingExpressionNegated, pIgnoreFluent);
}


bool ConditionExists::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    const auto& factAccessorsToFacts = pWorldState.factsMapping();
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
    _existsExtractPossRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, factAccessorsToFacts, pFactFromEffect, parameter, pIsWrappingExpressionNegated);
    auto parameters = pConditionParametersToPossibleArguments;
    parameters[parameter];

    return condition->findConditionCandidateFromFactFromEffect([&](const FactOptional& pConditionFact) {
      auto factToConsider = pConditionFact.fact;
      factToConsider.replaceArguments(localParamToValue);
      return pDoesConditionFactMatchFactFromEffect(FactOptional(factToConsider)) == !pIsWrappingExpressionNegated;
    }, pWorldState, pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
    parameters, pIsWrappingExpressionNegated);
  }
  return pIsWrappingExpressionNegated;
}


bool ConditionExists::isTrue(const WorldState& pWorldState,
                             const std::set<Fact>&,
                             const std::set<Fact>&,
                             std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                             bool*,
                             bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
    return _existsIsTrueRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, pWorldState) == !pIsWrappingExpressionNegated;
  }
  return !pIsWrappingExpressionNegated;
}


bool ConditionExists::canBecomeTrue(const WorldState& pWorldState,
                                    const std::vector<Parameter>& pParameters,
                                    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    auto* factOfConditionPtr = condition->fcFactPtr();
    if (factOfConditionPtr != nullptr)
    {
      const auto& factToOfCondition = factOfConditionPtr->factOptional.fact;
      std::set<Entity> potentialArgumentsOfTheParameter;
      pWorldState.extractPotentialArgumentsOfAFactParameter(potentialArgumentsOfTheParameter,
                                                            factToOfCondition, parameter.name);
      for (auto& currPot : potentialArgumentsOfTheParameter)
      {
        auto factToCheck = factToOfCondition;
        factToCheck.replaceArguments({{parameter, currPot}});
        if (pWorldState.canFactBecomeTrue(factToCheck, pParameters))
          return true;
      }
      return pIsWrappingExpressionNegated;
    }
  }
  return true;
}

bool ConditionExists::operator==(const Condition& pOther) const
{
  auto* otherExistsPtr = pOther.fcExistsPtr();
  return otherExistsPtr != nullptr &&
      parameter == otherExistsPtr->parameter &&
      _areEqual(condition, otherExistsPtr->condition);
}

std::unique_ptr<Condition> ConditionExists::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                  bool pInvert) const
{
  auto res = std::make_unique<ConditionExists>(
        parameter,
        condition ? condition->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>());
  if (pInvert)
    return std::make_unique<ConditionNot>(std::move(res));
  return res;
}




std::string ConditionNot::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                bool pPrintAnyFluent) const
{
  std::string conditionStr;
  if (condition)
    conditionStr = condition->toStr(pFactWriterPtr);
  if (condition->fcExistsPtr() != nullptr)
    return "!" + conditionStr;
  return "!(" + conditionStr + ")";
}

ConditionNot::ConditionNot(std::unique_ptr<Condition> pCondition)
  : Condition(),
    condition(std::move(pCondition))
{
}

bool ConditionNot::hasFact(const Fact& pFact) const
{
  return condition && condition->hasFact(pFact);
}


bool ConditionNot::containsFactOpt(const FactOptional& pFactOptional,
                                   const std::map<Parameter, std::set<Entity>>& pFactParameters,
                                   const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                                   const std::vector<Parameter>& pConditionParameters,
                                   bool pIsWrappingExpressionNegated) const
{
  return condition && condition->containsFactOpt(pFactOptional, pFactParameters, pOtherFactParametersPtr, pConditionParameters, !pIsWrappingExpressionNegated);
}


void ConditionNot::forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
                          bool pIsWrappingExpressionNegated,
                          bool pIgnoreFluent) const
{

  if (condition)
    condition->forAll(pFactCallback, !pIsWrappingExpressionNegated, pIgnoreFluent);
}


bool ConditionNot::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
    return condition->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pFactFromEffect,
                                                               pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                               pConditionParametersToPossibleArguments, !pIsWrappingExpressionNegated);
  return pIsWrappingExpressionNegated;
}


bool ConditionNot::isTrue(const WorldState& pWorldState,
                          const std::set<Fact>& pPunctualFacts,
                          const std::set<Fact>& pRemovedFacts,
                          std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                          bool* pCanBecomeTruePtr,
                          bool pIsWrappingExpressionNegated) const
{
  if (condition)
    return condition->isTrue(pWorldState, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, pCanBecomeTruePtr, !pIsWrappingExpressionNegated);
  return !pIsWrappingExpressionNegated;
}


bool ConditionNot::canBecomeTrue(const WorldState& pWorldState,
                                 const std::vector<Parameter>& pParameters,
                                 bool pIsWrappingExpressionNegated) const
{
  if (condition)
    return condition->canBecomeTrue(pWorldState, pParameters, !pIsWrappingExpressionNegated);
  return true;
}

bool ConditionNot::operator==(const Condition& pOther) const
{
  auto* otherNotPtr = pOther.fcNotPtr();
  return otherNotPtr != nullptr &&
      _areEqual(condition, otherNotPtr->condition);
}

std::unique_ptr<Condition> ConditionNot::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                               bool pInvert) const
{
  if (pInvert)
    return condition ? condition->clone(pConditionParametersToArgumentPtr, false) : std::unique_ptr<Condition>();
  return std::make_unique<ConditionNot>(condition ? condition->clone(pConditionParametersToArgumentPtr, pInvert) : std::unique_ptr<Condition>());
}






ConditionFact::ConditionFact(const FactOptional& pFactOptional)
  : Condition(),
    factOptional(pFactOptional)
{
}

bool ConditionFact::hasFact(const Fact& pFact) const
{
  return factOptional.fact == pFact;
}

bool ConditionFact::containsFactOpt(const FactOptional& pFactOptional,
                                    const std::map<Parameter, std::set<Entity>>& pFactParameters,
                                    const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                                    const std::vector<Parameter>& pConditionParameters,
                                    bool pIsWrappingExpressionNegated) const
{
  if ((!pIsWrappingExpressionNegated && pFactOptional.isFactNegated == factOptional.isFactNegated) ||
      (pIsWrappingExpressionNegated && pFactOptional.isFactNegated != factOptional.isFactNegated))
    return factOptional.fact.areEqualExceptAnyValues(pFactOptional.fact, &pFactParameters, pOtherFactParametersPtr, &pConditionParameters);
  return false;
}

void ConditionFact::forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
                           bool pIsWrappingExpressionNegated,
                           bool pIgnoreFluent) const
{
  if (!pIsWrappingExpressionNegated)
  {
    pFactCallback(factOptional, pIgnoreFluent);
  }
  else
  {
    auto factOptionalCopied = factOptional;
    factOptionalCopied.isFactNegated = !factOptionalCopied.isFactNegated;
    pFactCallback(factOptionalCopied, pIgnoreFluent);
  }
}


bool ConditionFact::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState&,
    const Fact&,
    const std::map<Parameter, std::set<Entity>>&,
    const std::map<Parameter, std::set<Entity>>*,
    const std::map<Parameter, std::set<Entity>>&,
    bool pIsWrappingExpressionNegated) const
{
  bool res = pDoesConditionFactMatchFactFromEffect(factOptional);
  if (pIsWrappingExpressionNegated)
    return !res;
  return res;
}


bool ConditionFact::isTrue(const WorldState& pWorldState,
                           const std::set<Fact>& pPunctualFacts,
                           const std::set<Fact>& pRemovedFacts,
                           std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                           bool* pCanBecomeTruePtr,
                           bool pIsWrappingExpressionNegated) const
{
  bool res = pWorldState.isOptionalFactSatisfiedInASpecificContext(factOptional, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, nullptr, pCanBecomeTruePtr);
  if (!pIsWrappingExpressionNegated)
    return res;
  return !res;
}

bool ConditionFact::canBecomeTrue(const WorldState& pWorldState,
                                  const std::vector<Parameter>& pParameters,
                                  bool pIsWrappingExpressionNegated) const
{
  bool res =  pWorldState.canFactOptBecomeTrue(factOptional, pParameters);
  if (!pIsWrappingExpressionNegated)
    return res;
  return !res;
}

bool ConditionFact::operator==(const Condition& pOther) const
{
  auto* otherFactPtr = pOther.fcFactPtr();
  return otherFactPtr != nullptr &&
      factOptional == otherFactPtr->factOptional;
}

std::optional<Entity> ConditionFact::getFluent(const WorldState& pWorldState) const
{
  return pWorldState.getFactFluent(factOptional.fact);
}

std::unique_ptr<Condition> ConditionFact::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                bool pInvert) const
{
  auto res = std::make_unique<ConditionFact>(factOptional);
  if (pConditionParametersToArgumentPtr != nullptr)
    res->factOptional.fact.replaceArguments(*pConditionParametersToArgumentPtr);
  if (pInvert)
    res->factOptional.isFactNegated = !res->factOptional.isFactNegated;
  return res;
}


std::string ConditionNumber::toStr(const std::function<std::string (const Fact&)>*,
                                   bool) const
{
  std::stringstream ss;
  ss << nb;
  return ss.str();
}

ConditionNumber::ConditionNumber(int pNb)
  : Condition(),
    nb(pNb)
{
}

bool ConditionNumber::operator==(const Condition& pOther) const
{
  auto* otherNbPtr = pOther.fcNbPtr();
  return otherNbPtr != nullptr &&
      nb == otherNbPtr->nb;
}

std::optional<Entity> ConditionNumber::getFluent(const WorldState&) const
{
  return Entity::createNumberEntity(toStr(nullptr, true));
}

std::unique_ptr<Condition> ConditionNumber::clone(const std::map<Parameter, Entity>*,
                                                  bool) const
{
  return std::make_unique<ConditionNumber>(nb);
}


} // !cp
