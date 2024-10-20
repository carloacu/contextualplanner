#include "worldstatemodificationprivate.hpp"
#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/worldstate.hpp>
#include "expressionParsed.hpp"
#include <contextualplanner/util/util.hpp>

namespace cp
{
namespace
{
bool _areEqual(
    const std::unique_ptr<WorldStateModification>& pCond1,
    const std::unique_ptr<WorldStateModification>& pCond2)
{
  if (!pCond1 && !pCond2)
    return true;
  if (pCond1 && pCond2)
    return *pCond1 == *pCond2;
  return false;
}


bool _isOkWithLocalParameters(const std::map<Parameter, std::set<Entity>>& pLocalParameterToFind,
                              std::map<Parameter, std::set<Entity>>& pParametersToFill,
                              const WorldStateModification& pWModif,
                              const WorldState& pWorldState,
                              std::map<Parameter, std::set<Entity>>& pParametersToModifyInPlace)
{
  if (!pParametersToFill.empty() &&
      !pParametersToFill.begin()->second.empty())
  {
    bool res = false;
    const auto* wSMFPtr = dynamic_cast<const WorldStateModificationFact*>(&pWModif);
    if (wSMFPtr != nullptr)
    {
      std::set<Entity>& parameterPossibilities = pParametersToFill.begin()->second;

      while (!parameterPossibilities.empty())
      {
        auto factWithValueToAssign = wSMFPtr->factOptional.fact;
        factWithValueToAssign.replaceArguments(pLocalParameterToFind);
        auto itBeginOfParamPoss = parameterPossibilities.begin();
        factWithValueToAssign.setFluent(*itBeginOfParamPoss);

        const auto& factAccessorsToFacts = pWorldState.factsMapping();
        std::map<Parameter, std::set<Entity>> newParameters;
        if (factWithValueToAssign.isInOtherFactsMap(factAccessorsToFacts, true, &newParameters, &pParametersToModifyInPlace))
        {
          res = true;
          applyNewParams(pParametersToModifyInPlace, newParameters);
          break;
        }
        parameterPossibilities.erase(itBeginOfParamPoss);
      }
    }
    return res;
  }
  return true;
}


const WorldStateModificationNode* _toWmNode(const WorldStateModification& pOther)
{
  return dynamic_cast<const WorldStateModificationNode*>(&pOther);
}

const WorldStateModificationFact* _toWmFact(const WorldStateModification& pOther)
{
  const WorldStateModificationFact* wmFactPtr = dynamic_cast<const WorldStateModificationFact*>(&pOther);
  return wmFactPtr;
}

const WorldStateModificationNumber* _toWmNumber(const WorldStateModification& pOther)
{
  return dynamic_cast<const WorldStateModificationNumber*>(&pOther);
}

}

std::string WorldStateModificationNode::toStr(bool pPrintAnyFluent) const
{
  bool printAnyFluent = pPrintAnyFluent && nodeType != WorldStateModificationNodeType::ASSIGN &&
      nodeType != WorldStateModificationNodeType::INCREASE && nodeType != WorldStateModificationNodeType::DECREASE &&
      nodeType != WorldStateModificationNodeType::MULTIPLY &&
      nodeType != WorldStateModificationNodeType::PLUS && nodeType != WorldStateModificationNodeType::MINUS;

  std::string leftOperandStr;
  if (leftOperand)
    leftOperandStr = leftOperand->toStr(printAnyFluent);
  std::string rightOperandStr;
  bool isRightOperandAFactWithoutParameter = false;
  if (rightOperand)
  {
    const auto* rightOperandFactPtr = _toWmFact(*rightOperand);
    if (rightOperandFactPtr != nullptr && rightOperandFactPtr->factOptional.fact.arguments().empty() &&
        !rightOperandFactPtr->factOptional.fact.fluent())
      isRightOperandAFactWithoutParameter = true;
    rightOperandStr = rightOperand->toStr(printAnyFluent);
  }

  switch (nodeType)
  {
  case WorldStateModificationNodeType::AND:
    return leftOperandStr + " & " + rightOperandStr;
  case WorldStateModificationNodeType::ASSIGN:
  {
    if (isRightOperandAFactWithoutParameter)
      rightOperandStr += "()"; // To significate it is a fact
    return "assign(" + leftOperandStr + ", " + rightOperandStr + ")";
  }
  case WorldStateModificationNodeType::FOR_ALL:
    if (!parameterOpt)
      throw std::runtime_error("for all statement without a parameter detected");
    return "forall(" + parameterOpt->toStr() + ", " + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::INCREASE:
    return "increase(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::DECREASE:
    return "decrease(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::MULTIPLY:
    return leftOperandStr + " * " + rightOperandStr;
  case WorldStateModificationNodeType::PLUS:
    return leftOperandStr + " + " + rightOperandStr;
  case WorldStateModificationNodeType::MINUS:
    return leftOperandStr + " - " + rightOperandStr;
  }
  return "";
}


void WorldStateModificationNode::forAll(const std::function<void (const FactOptional&)>& pFactCallback,
                                        const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->forAll(pFactCallback, pSetOfFact);
    if (rightOperand)
      rightOperand->forAll(pFactCallback, pSetOfFact);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pSetOfFact));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      pWsModification.forAll(pFactCallback, pSetOfFact);
    }, pSetOfFact);
  }
  else if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(multiplyNbOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
}


void WorldStateModificationNode::forAllThatCanBeModified(const std::function<void (const FactOptional&)>& pFactCallback) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->forAllThatCanBeModified(pFactCallback);
    if (rightOperand)
      rightOperand->forAllThatCanBeModified(pFactCallback);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      return pFactCallback(leftFactPtr->factOptional);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL && rightOperand)
  {
    auto* rightFactPtr = _toWmFact(*rightOperand);
    if (rightFactPtr != nullptr)
      return pFactCallback(rightFactPtr->factOptional);
  }
  else if ((nodeType == WorldStateModificationNodeType::INCREASE ||
            nodeType == WorldStateModificationNodeType::DECREASE ||
            nodeType == WorldStateModificationNodeType::MULTIPLY) && leftOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      return pFactCallback(leftFactPtr->factOptional);
  }
}


void WorldStateModificationNode::iterateOverAllAccessibleFacts(
    const std::function<void (const FactOptional&)>& pFactCallback,
    const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->iterateOverAllAccessibleFacts(pFactCallback, pSetOfFact);
    if (rightOperand)
      rightOperand->iterateOverAllAccessibleFacts(pFactCallback, pSetOfFact);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pSetOfFact));
      if (!factToCheck.fact.fluent())
        factToCheck.fact.setFluentValue(Entity::anyEntityValue());
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      pWsModification.iterateOverAllAccessibleFacts(pFactCallback, pSetOfFact);
    }, pSetOfFact);
  }
  else if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(multiplyNbOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
}


bool WorldStateModificationNode::forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback,
                                                 const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->forAllUntilTrue(pFactCallback, pSetOfFact)) ||
        (rightOperand && rightOperand->forAllUntilTrue(pFactCallback, pSetOfFact));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pSetOfFact));
      return pFactCallback(factToCheck);
    }
  }

  if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    bool res = false;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      if (!res)
        res = pWsModification.forAllUntilTrue(pFactCallback, pSetOfFact);
    }, pSetOfFact);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }

  if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(multiplyNbOrStr(leftOperand->getFluent(pSetOfFact), rightOperand->getFluent(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }

  return false;
}


bool WorldStateModificationNode::canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pFactCallback,
                                                     std::map<Parameter, std::set<Entity>>& pParameters,
                                                     const WorldState& pWorldState,
                                                     const std::string& pFromDeductionId) const
{
  const auto& setOfFacts = pWorldState.factsMapping();

  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId)) ||
        (rightOperand && rightOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(setOfFacts));
      std::map<Parameter, std::set<Entity>> localParameterToFind;

      if (!factToCheck.fact.fluent())
      {
        factToCheck.fact.setFluent(Entity("??tmpValueFromSet_" + pFromDeductionId, factToCheck.fact.predicate.fluent));
        localParameterToFind[Parameter(factToCheck.fact.fluent()->value, factToCheck.fact.predicate.fluent)];
      }
      bool res = pFactCallback(factToCheck, &localParameterToFind, [&](const std::map<Parameter, std::set<Entity>>& pLocalParameterToFind){
        return _isOkWithLocalParameters(pLocalParameterToFind, localParameterToFind, *rightOperand, pWorldState, pParameters);
      });
      return res;
    }
  }

  if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    bool res = false;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      if (!res)
        res = pWsModification.canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId);
    }, setOfFacts);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(multiplyNbOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  return false;
}


bool WorldStateModificationNode::iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pCallback,
                                                      std::map<Parameter, std::set<Entity>>& pParameters,
                                                      const WorldState& pWorldState,
                                                      const std::string& pFromDeductionId) const
{
  const auto& setOfFacts = pWorldState.factsMapping();

  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->iterateOnSuccessions(pCallback, pParameters, pWorldState, pFromDeductionId)) ||
        (rightOperand && rightOperand->iterateOnSuccessions(pCallback, pParameters, pWorldState, pFromDeductionId));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand && !_successions.empty())
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(setOfFacts));
      std::map<Parameter, std::set<Entity>> localParameterToFind;

      if (!factToCheck.fact.fluent())
      {
        factToCheck.fact.setFluent(Entity("??tmpValueFromSet_" + pFromDeductionId, factToCheck.fact.predicate.fluent));
        localParameterToFind[Parameter(factToCheck.fact.fluent()->value, factToCheck.fact.predicate.fluent)];
      }
      bool res = pCallback(_successions, factToCheck, &localParameterToFind, [&](const std::map<Parameter, std::set<Entity>>& pLocalParameterToFind){
        return _isOkWithLocalParameters(pLocalParameterToFind, localParameterToFind, *rightOperand, pWorldState, pParameters);
      });
      return res;
    }
  }

  if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    bool res = false;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      if (!res)
        res = pWsModification.iterateOnSuccessions(pCallback, pParameters, pWorldState, pFromDeductionId);
    }, setOfFacts);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand && !_successions.empty())
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand && !_successions.empty())
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand && !_successions.empty())
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(multiplyNbOrStr(leftOperand->getFluent(setOfFacts), rightOperand->getFluent(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  return false;
}



void WorldStateModificationNode::updateSuccesions(const Domain& pDomain,
                                                  const WorldStateModificationContainerId& pContainerId,
                                                  const std::set<FactOptional>& pOptionalFactsToIgnore)
{
  _successions.clear();

  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
    if (rightOperand)
      rightOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
    {
      auto* leftFactPtr = _toWmFact(*leftOperand);
      if (leftFactPtr != nullptr)
        _successions.addSuccesionsOptFact(leftFactPtr->factOptional, pDomain, pContainerId, pOptionalFactsToIgnore);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    if (rightOperand)
      rightOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
  }
}

void WorldStateModificationNode::removePossibleSuccession(const ActionId& pActionIdToRemove)
{
  _successions.actions.erase(pActionIdToRemove);

  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->removePossibleSuccession(pActionIdToRemove);
    if (rightOperand)
      rightOperand->removePossibleSuccession(pActionIdToRemove);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
      leftOperand->removePossibleSuccession(pActionIdToRemove);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    if (rightOperand)
      rightOperand->removePossibleSuccession(pActionIdToRemove);
  }
}

void WorldStateModificationNode::printSuccesions(std::string& pRes) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->printSuccesions(pRes);
    if (rightOperand)
      rightOperand->printSuccesions(pRes);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
    {
      auto* leftFactPtr = _toWmFact(*leftOperand);
      if (leftFactPtr != nullptr)
        _successions.print(pRes, leftFactPtr->factOptional);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    if (rightOperand)
      rightOperand->printSuccesions(pRes);
  }
}


bool WorldStateModificationNode::operator==(const WorldStateModification& pOther) const
{
  auto* otherNodePtr = _toWmNode(pOther);
  return otherNodePtr != nullptr &&
      nodeType == otherNodePtr->nodeType &&
      _areEqual(leftOperand, otherNodePtr->leftOperand) &&
      _areEqual(rightOperand, otherNodePtr->rightOperand) &&
      parameterOpt == otherNodePtr->parameterOpt;
}


std::optional<Entity> WorldStateModificationNode::getFluent(const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::PLUS)
  {
    auto leftValue = leftOperand->getFluent(pSetOfFact);
    auto rightValue = rightOperand->getFluent(pSetOfFact);
    return plusIntOrStr(leftValue, rightValue);
  }
  if (nodeType == WorldStateModificationNodeType::MINUS)
  {
    auto leftValue = leftOperand->getFluent(pSetOfFact);
    auto rightValue = rightOperand->getFluent(pSetOfFact);
    return minusIntOrStr(leftValue, rightValue);
  }
  return {};
}


void WorldStateModificationNode::_forAllInstruction(const std::function<void (const WorldStateModification &)>& pCallback,
                                                    const SetOfFacts& pSetOfFact) const
{
  if (leftOperand && rightOperand && parameterOpt)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      std::set<Entity> parameterValues;
      pSetOfFact.extractPotentialArgumentsOfAFactParameter(parameterValues, leftFactPtr->factOptional.fact, parameterOpt->name);
      if (!parameterValues.empty())
      {
        for (const auto& paramValue : parameterValues)
        {
          auto newWsModif = rightOperand->clone(nullptr);
          newWsModif->replaceArgument(parameterOpt->toEntity(), paramValue);
          pCallback(*newWsModif);
        }
      }
    }
  }
}

bool WorldStateModificationFact::operator==(const WorldStateModification& pOther) const
{
  auto* otherFactPtr = _toWmFact(pOther);
  return otherFactPtr != nullptr &&
      factOptional == otherFactPtr->factOptional;
}

std::optional<Entity> WorldStateModificationFact::getFluent(const SetOfFacts& pSetOfFact) const
{
  return pSetOfFact.getFactFluent(factOptional.fact);
}

std::unique_ptr<WorldStateModificationNumber> WorldStateModificationNumber::create(const std::string& pStr)
{
  return std::make_unique<WorldStateModificationNumber>(stringToNumber(pStr));
}

std::string WorldStateModificationNumber::toStr(bool) const
{
  return numberToString(_nb);
}

bool WorldStateModificationNumber::operator==(const WorldStateModification& pOther) const
{
  auto* otherNumberPtr = _toWmNumber(pOther);
  return otherNumberPtr != nullptr && _nb == otherNumberPtr->_nb;
}


} // !cp
