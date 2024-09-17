#include <contextualplanner/types/worldstatemodification.hpp>
#include <sstream>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/worldstate.hpp>
#include "expressionParsed.hpp"
#include <contextualplanner/util/util.hpp>

namespace cp
{
namespace
{
const char* _assignFunctionName = "assign";
const char* _setFunctionName = "set"; // deprecated
const char* _forAllFunctionName = "forall";
const char* _forAllOldFunctionName = "forAll";
const char* _addFunctionName = "add";
const char* _increaseFunctionName = "increase";
const char* _decreaseFunctionName = "decrease";
const char* _andFunctionName = "and";
const char* _whenFunctionName = "when";
const char* _notFunctionName = "not";


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


enum class WorldStateModificationNodeType
{
  AND,
  ASSIGN,
  FOR_ALL,
  INCREASE,
  DECREASE,
  PLUS,
  MINUS
};


struct WorldStateModificationNode : public WorldStateModification
{
  WorldStateModificationNode(WorldStateModificationNodeType pNodeType,
                             std::unique_ptr<WorldStateModification> pLeftOperand,
                             std::unique_ptr<WorldStateModification> pRightOperand,
                             const std::optional<Parameter>& pParameterOpt = {})
    : WorldStateModification(),
      nodeType(pNodeType),
      leftOperand(std::move(pLeftOperand)),
      rightOperand(std::move(pRightOperand)),
      parameterOpt(pParameterOpt)
  {
  }

  std::string toStr(bool pPrintAnyFluent) const override;

  bool hasFact(const Fact& pFact) const override
  {
    return (leftOperand && leftOperand->hasFact(pFact)) ||
        (rightOperand && rightOperand->hasFact(pFact));
  }

  bool hasFactOptional(const cp::FactOptional& FactOptional) const override
  {
    return (leftOperand && leftOperand->hasFactOptional(FactOptional)) ||
        (rightOperand && rightOperand->hasFactOptional(FactOptional));
  }

  bool isOnlyASetOfFacts() const override
  {
    if (nodeType == WorldStateModificationNodeType::ASSIGN ||
        nodeType == WorldStateModificationNodeType::FOR_ALL ||
        nodeType == WorldStateModificationNodeType::INCREASE ||
        nodeType == WorldStateModificationNodeType::DECREASE ||
        nodeType == WorldStateModificationNodeType::PLUS ||
        nodeType == WorldStateModificationNodeType::MINUS)
      return false;
    return (leftOperand && leftOperand->isOnlyASetOfFacts()) &&
        (rightOperand && rightOperand->isOnlyASetOfFacts());
  }

  void replaceArgument(const Entity& pOldFact,
                       const Entity& pNewFact) override
  {
    if (leftOperand)
      leftOperand->replaceArgument(pOldFact, pNewFact);
    if (rightOperand)
      rightOperand->replaceArgument(pOldFact, pNewFact);
  }

  void forAllFactOpt(const std::function<void (const FactOptional&)>& pFactCallback) const override;
  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const WorldState& pWorldState) const override;
  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>& pFactCallback,
                                     const WorldState& pWorldState) const;
  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback,
                       const WorldState& pWorldState) const override;
  bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pFactCallback,
                           std::map<Parameter, std::set<Entity>>& pParameters,
                           const WorldState& pWorldState,
                           const std::string& pFromDeductionId) const override;
  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const WorldState& pWorldState) const override
  {
    if (nodeType == WorldStateModificationNodeType::PLUS)
    {
      auto leftValue = leftOperand->getFluent(pWorldState);
      auto rightValue = rightOperand->getFluent(pWorldState);
      return plusIntOrStr(leftValue, rightValue);
    }
    if (nodeType == WorldStateModificationNodeType::MINUS)
    {
      auto leftValue = leftOperand->getFluent(pWorldState);
      auto rightValue = rightOperand->getFluent(pWorldState);
      return minusIntOrStr(leftValue, rightValue);
    }
    return {};
  }

  const FactOptional* getOptionalFact() const override
  {
    return nullptr;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>* pParametersToArgumentPtr) const override
  {
    return std::make_unique<WorldStateModificationNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pParametersToArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          rightOperand ? rightOperand->clone(pParametersToArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          parameterOpt);
  }


  std::unique_ptr<WorldStateModification> cloneParamSet(const std::map<Parameter, std::set<Entity>>& pParametersToPossibleArgumentPtr) const override
  {
    return std::make_unique<WorldStateModificationNode>(
          nodeType,
          leftOperand ? leftOperand->cloneParamSet(pParametersToPossibleArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          rightOperand ? rightOperand->cloneParamSet(pParametersToPossibleArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          parameterOpt);
  }

  WorldStateModificationNodeType nodeType;
  std::unique_ptr<WorldStateModification> leftOperand;
  std::unique_ptr<WorldStateModification> rightOperand;
  std::optional<Parameter> parameterOpt;

private:
  void _forAllInstruction(const std::function<void (const WorldStateModification&)>& pCallback,
                          const WorldState& pWorldState) const;
};


struct WorldStateModificationFact : public WorldStateModification
{
  WorldStateModificationFact(const FactOptional& pFactOptional)
    : WorldStateModification(),
      factOptional(pFactOptional)
  {
  }

  std::string toStr(bool pPrintAnyFluent) const override { return factOptional.toStr(nullptr, pPrintAnyFluent); }

  bool hasFact(const cp::Fact& pFact) const override
  {
    return factOptional.fact == pFact;
  }

  bool hasFactOptional(const cp::FactOptional& FactOptional) const override
  {
    return factOptional == FactOptional;
  }

  bool isOnlyASetOfFacts() const override { return true; }

  void replaceArgument(const Entity& pOld,
                       const Entity& pNew) override
  {
    factOptional.fact.replaceArgument(pOld, pNew);
  }

  void forAllFactOpt(const std::function<void (const FactOptional&)>& pFactCallback) const override { pFactCallback(factOptional); }
  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const WorldState&) const override { pFactCallback(factOptional); }

  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>& pFactCallback,
                                     const WorldState&) const override { pFactCallback(factOptional); }


  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback, const WorldState&) const override
  {
    return pFactCallback(factOptional);
  }

  bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pFactCallback,
                           std::map<Parameter, std::set<Entity>>&,
                           const WorldState&,
                           const std::string&) const override
  {
    return pFactCallback(factOptional, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
  }

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const WorldState& pWorldState) const override
  {
    return pWorldState.getFactFluent(factOptional.fact);
  }

  const FactOptional* getOptionalFact() const override
  {
    return &factOptional;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>* pParametersToArgumentPtr) const override
  {
    auto res = std::make_unique<WorldStateModificationFact>(factOptional);
    if (pParametersToArgumentPtr != nullptr)
      res->factOptional.fact.replaceArguments(*pParametersToArgumentPtr);
    return res;
  }

  std::unique_ptr<WorldStateModification> cloneParamSet(const std::map<Parameter, std::set<Entity>>& pParametersToPossibleArgumentPtr) const override
  {
    auto res = std::make_unique<WorldStateModificationFact>(factOptional);
    res->factOptional.fact.replaceArguments(pParametersToPossibleArgumentPtr);
    return res;
  }

  FactOptional factOptional;
};



struct WorldStateModificationNumber : public WorldStateModification
{
  WorldStateModificationNumber(int pNb)
    : WorldStateModification(),
      nb(pNb)
  {
  }

  std::string toStr(bool) const override
  {
    std::stringstream ss;
    ss << nb;
    return ss.str();
  }

  bool hasFact(const cp::Fact&) const override { return false; }
  bool hasFactOptional(const cp::FactOptional&) const override { return false; }
  bool isOnlyASetOfFacts() const override { return false; }

  void replaceArgument(const Entity&,
                       const Entity&) override {}
  void forAllFactOpt(const std::function<void (const FactOptional&)>&) const override {}
  void forAll(const std::function<void (const FactOptional&)>&,
              const WorldState&) const override {}
  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>&,
                                     const WorldState&) const override {}
  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>&,
                       const WorldState&) const override { return false; }
  bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>&,
                           std::map<Parameter, std::set<Entity>>&,
                           const WorldState&,
                           const std::string&) const override { return false; }
  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const WorldState&) const override
  {
    return Entity::createNumberEntity(toStr(true));
  }

  const FactOptional* getOptionalFact() const override
  {
    return nullptr;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>*) const override
  {
    return std::make_unique<WorldStateModificationNumber>(nb);
  }

  std::unique_ptr<WorldStateModification> cloneParamSet(const std::map<Parameter, std::set<Entity>>&) const override
  {
    return std::make_unique<WorldStateModificationNumber>(nb);
  }

  int nb;
};

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


std::string WorldStateModificationNode::toStr(bool pPrintAnyFluent) const
{
  bool printAnyFluent = pPrintAnyFluent && nodeType != WorldStateModificationNodeType::ASSIGN &&
      nodeType != WorldStateModificationNodeType::INCREASE && nodeType != WorldStateModificationNodeType::DECREASE &&
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
    return std::string(_assignFunctionName) + "(" + leftOperandStr + ", " + rightOperandStr + ")";
  }
  case WorldStateModificationNodeType::FOR_ALL:
    if (!parameterOpt)
      throw std::runtime_error("for all statement without a parameter detected");
    return std::string(_forAllFunctionName) + "(" + parameterOpt->toStr() + ", " + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::INCREASE:
    return std::string(_increaseFunctionName) + "(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::DECREASE:
    return std::string(_decreaseFunctionName) + "(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::PLUS:
    return leftOperandStr + " + " + rightOperandStr;
  case WorldStateModificationNodeType::MINUS:
    return leftOperandStr + " - " + rightOperandStr;
  }
  return "";
}


void WorldStateModificationNode::forAllFactOpt(const std::function<void (const FactOptional&)>& pFactCallback) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->forAllFactOpt(pFactCallback);
    if (rightOperand)
      rightOperand->forAllFactOpt(pFactCallback);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      pFactCallback(leftFactPtr->factOptional);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    if (rightOperand)
      rightOperand->forAllFactOpt(pFactCallback);
  }
  else if ((nodeType == WorldStateModificationNodeType::INCREASE || nodeType == WorldStateModificationNodeType::DECREASE) &&
           leftOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      pFactCallback(leftFactPtr->factOptional);
  }
}


void WorldStateModificationNode::forAll(const std::function<void (const FactOptional&)>& pFactCallback,
                                        const WorldState& pWorldState) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->forAll(pFactCallback, pWorldState);
    if (rightOperand)
      rightOperand->forAll(pFactCallback, pWorldState);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pWorldState));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      pWsModification.forAll(pFactCallback, pWorldState);
    }, pWorldState);
  }
  else if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck);
    }
  }
}


void WorldStateModificationNode::iterateOverAllAccessibleFacts(
    const std::function<void (const FactOptional&)>& pFactCallback,
    const WorldState& pWorldState) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->iterateOverAllAccessibleFacts(pFactCallback, pWorldState);
    if (rightOperand)
      rightOperand->iterateOverAllAccessibleFacts(pFactCallback, pWorldState);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pWorldState));
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
      pWsModification.iterateOverAllAccessibleFacts(pFactCallback, pWorldState);
    }, pWorldState);
  }
  else if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck);
    }
  }
}


bool WorldStateModificationNode::forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback,
                                                 const WorldState& pWorldState) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->forAllUntilTrue(pFactCallback, pWorldState)) ||
        (rightOperand && rightOperand->forAllUntilTrue(pFactCallback, pWorldState));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pWorldState));
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
        res = pWsModification.forAllUntilTrue(pFactCallback, pWorldState);
    }, pWorldState);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck);
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
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
  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId)) ||
        (rightOperand && rightOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(rightOperand->getFluent(pWorldState));
      std::map<Parameter, std::set<Entity>> localParameterToFind;

      if (!factToCheck.fact.fluent())
      {
        factToCheck.fact.setFluent(Entity("??tmpValueFromSet_" + pFromDeductionId, factToCheck.fact.predicate.fluent));
        localParameterToFind[Parameter(factToCheck.fact.fluent()->value, factToCheck.fact.predicate.fluent)];
      }
      bool res = pFactCallback(factToCheck, &localParameterToFind, [&](const std::map<Parameter, std::set<Entity>>& pLocalParameterToFind){
        if (!localParameterToFind.empty() &&
            !localParameterToFind.begin()->second.empty())
        {
          res = false;
          const auto* wSMFPtr = dynamic_cast<const WorldStateModificationFact*>(&*rightOperand);
          if (wSMFPtr != nullptr)
          {
            std::set<Entity>& parameterPossibilities = localParameterToFind.begin()->second;

            while (!parameterPossibilities.empty())
            {
              auto factWithValueToAssign = wSMFPtr->factOptional.fact;
              factWithValueToAssign.replaceArguments(pLocalParameterToFind);
              auto itBeginOfParamPoss = parameterPossibilities.begin();
              factWithValueToAssign.setFluent(*itBeginOfParamPoss);

              const auto& factAccessorsToFacts = pWorldState.factsMapping();
              std::map<Parameter, std::set<Entity>> newParameters;
              if (factWithValueToAssign.isInOtherFactsMap(factAccessorsToFacts, true, &newParameters, &pParameters))
              {
                res = true;
                applyNewParams(pParameters, newParameters);
                break;
              }
              parameterPossibilities.erase(itBeginOfParamPoss);
            }
          }
          return res;
        }
        return true;
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
    }, pWorldState);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(plusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setFluent(minusIntOrStr(leftOperand->getFluent(pWorldState), rightOperand->getFluent(pWorldState)));
      return pFactCallback(factToCheck, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    }
  }

  return false;
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

void WorldStateModificationNode::_forAllInstruction(const std::function<void (const WorldStateModification &)>& pCallback,
                                                    const WorldState& pWorldState) const
{
  if (leftOperand && rightOperand && parameterOpt)
  {
    auto* leftFactPtr = _toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      std::set<Entity> parameterValues;
      pWorldState.extractPotentialArgumentsOfAFactParameter(parameterValues, leftFactPtr->factOptional.fact, parameterOpt->name);
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

bool WorldStateModificationNumber::operator==(const WorldStateModification& pOther) const
{
  auto* otherNumberPtr = _toWmNumber(pOther);
  return otherNumberPtr != nullptr && nb == otherNumberPtr->nb;
}



std::unique_ptr<WorldStateModification> _expressionParsedToWsModification(const ExpressionParsed& pExpressionParsed,
                                                                          const Ontology& pOntology,
                                                                          const SetOfEntities& pEntities,
                                                                          const std::vector<Parameter>& pParameters,
                                                                          bool pIsOkIfFluentIsMissing)
{
  std::unique_ptr<WorldStateModification> res;

  if ((pExpressionParsed.name == _assignFunctionName ||
       pExpressionParsed.name == _setFunctionName) && // set is deprecated
      pExpressionParsed.arguments.size() == 2)
  {
    auto leftOperand = _expressionParsedToWsModification(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true);
    const auto& rightOperandExp = *(++pExpressionParsed.arguments.begin());
    auto* leftFactPtr = dynamic_cast<WorldStateModificationFact*>(&*leftOperand);
    if (leftFactPtr != nullptr && !leftFactPtr->factOptional.isFactNegated &&
        rightOperandExp.arguments.empty() &&
        !rightOperandExp.followingExpression && rightOperandExp.value == "")
    {
      if (rightOperandExp.name == Fact::undefinedValue.value)
      {
        leftFactPtr->factOptional.isFactNegated = true;
        leftFactPtr->factOptional.fact.setFluentValue(Entity::anyEntityValue());
        res = std::make_unique<WorldStateModificationFact>(std::move(*leftFactPtr));
      }
      else if (pExpressionParsed.name == _assignFunctionName && !rightOperandExp.isAFunction &&
               rightOperandExp.name != "")
      {
        leftFactPtr->factOptional.fact.setFluent(Entity::fromUsage(rightOperandExp.name, pOntology, pEntities, pParameters));
        res = std::make_unique<WorldStateModificationFact>(std::move(*leftFactPtr));
      }
    }

    if (!res)
    {
      auto rightOperand = _expressionParsedToWsModification(rightOperandExp, pOntology, pEntities, pParameters, true);
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::ASSIGN,
                                                         std::move(leftOperand), std::move(rightOperand));
    }
  }
  else if (pExpressionParsed.name == _notFunctionName &&
           pExpressionParsed.arguments.size() == 1)
  {
    auto factNegationed = pExpressionParsed.arguments.front().toFact(pOntology, pEntities, pParameters, false);
    factNegationed.isFactNegated = !factNegationed.isFactNegated;
    res = std::make_unique<WorldStateModificationFact>(factNegationed);
  }
  else if ((pExpressionParsed.name == _forAllFunctionName || pExpressionParsed.name == _forAllOldFunctionName) &&
           (pExpressionParsed.arguments.size() == 2 || pExpressionParsed.arguments.size() == 3))
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    std::shared_ptr<Type> paramType;
    if (firstArg.followingExpression)
      paramType = pOntology.types.nameToType(firstArg.followingExpression->name);
    Parameter forAllParameter(firstArg.name, paramType);
    auto newParameters = pParameters;
    newParameters.push_back(forAllParameter);

    ++itArg;
    auto& secondArg = *itArg;
    if (pExpressionParsed.arguments.size() == 3)
    {
      ++itArg;
      auto& thridArg = *itArg;
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::FOR_ALL,
                                                         std::make_unique<WorldStateModificationFact>(secondArg.toFact(pOntology, pEntities, newParameters, false)),
                                                         _expressionParsedToWsModification(thridArg, pOntology, pEntities, newParameters, false),
                                                         forAllParameter);
    }
    else if (secondArg.name == _whenFunctionName &&
             secondArg.arguments.size() == 2)
    {
      auto itWhenArg = secondArg.arguments.begin();
      auto& firstWhenArg = *itWhenArg;
      ++itWhenArg;
      auto& secondWhenArg = *itWhenArg;
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::FOR_ALL,
                                                         std::make_unique<WorldStateModificationFact>(firstWhenArg.toFact(pOntology, pEntities, newParameters, false)),
                                                         _expressionParsedToWsModification(secondWhenArg, pOntology, pEntities, newParameters, false),
                                                         forAllParameter);
    }
  }
  else if (pExpressionParsed.name == _andFunctionName &&
           pExpressionParsed.arguments.size() >= 2)
  {
    std::list<std::unique_ptr<WorldStateModification>> elts;
    for (auto& currExp : pExpressionParsed.arguments)
      elts.emplace_back(_expressionParsedToWsModification(currExp, pOntology, pEntities, pParameters, false));

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND, std::move(*(--(--elts.end()))), std::move(elts.back()));
    elts.pop_back();
    elts.pop_back();

    while (!elts.empty())
    {
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND, std::move(elts.back()), std::move(res));
      elts.pop_back();
    }
  }
  else if ((pExpressionParsed.name == _increaseFunctionName || pExpressionParsed.name == _addFunctionName) &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    ++itArg;
    auto& secondArg = *itArg;
    std::unique_ptr<WorldStateModification> rightOpPtr;
    try {
      rightOpPtr = std::make_unique<WorldStateModificationNumber>(lexical_cast<int>(secondArg.name));
    }  catch (...) {}
    if (!rightOpPtr)
      rightOpPtr = _expressionParsedToWsModification(secondArg, pOntology, pEntities, pParameters, false);

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::INCREASE,
                                                       _expressionParsedToWsModification(firstArg, pOntology, pEntities, pParameters, true),
                                                       std::move(rightOpPtr));
  }
  else if (pExpressionParsed.name == _decreaseFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    ++itArg;
    auto& secondArg = *itArg;
    std::unique_ptr<WorldStateModification> rightOpPtr;
    try {
      rightOpPtr = std::make_unique<WorldStateModificationNumber>(lexical_cast<int>(secondArg.name));
    }  catch (...) {}
    if (!rightOpPtr)
      rightOpPtr = _expressionParsedToWsModification(secondArg, pOntology, pEntities, pParameters, false);

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::DECREASE,
                                                       _expressionParsedToWsModification(firstArg, pOntology, pEntities, pParameters, true),
                                                       std::move(rightOpPtr));
  }
  else
  {
    if (pExpressionParsed.arguments.empty() && pExpressionParsed.value == "")
    {
      try {
        res = std::make_unique<WorldStateModificationNumber>(lexical_cast<int>(pExpressionParsed.name));
      }  catch (...) {}
    }

    if (!res)
      res = std::make_unique<WorldStateModificationFact>(pExpressionParsed.toFact(pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing));
  }

  if (pExpressionParsed.followingExpression)
  {
    auto nodeType = WorldStateModificationNodeType::AND;
    if (pExpressionParsed.separatorToFollowingExp == '+')
      nodeType = WorldStateModificationNodeType::PLUS;
    else if (pExpressionParsed.separatorToFollowingExp == '-')
      nodeType = WorldStateModificationNodeType::MINUS;
    res = std::make_unique<WorldStateModificationNode>(nodeType,
                                                       std::move(res),
                                                       _expressionParsedToWsModification(*pExpressionParsed.followingExpression,
                                                                                         pOntology, pEntities, pParameters, false));
  }

  return res;
}

}


std::unique_ptr<WorldStateModification> WorldStateModification::fromStr(const std::string& pStr,
                                                                        const Ontology& pOntology,
                                                                        const SetOfEntities& pEntities,
                                                                        const std::vector<Parameter>& pParameters)
{
  if (pStr.empty())
    return {};
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);
  return _expressionParsedToWsModification(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::unique_ptr<WorldStateModification> WorldStateModification::createByConcatenation(const WorldStateModification& pWsModif1,
                                                                                      const WorldStateModification& pWsModif2)
{
  return std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND,
                                                      pWsModif1.clone(nullptr),
                                                      pWsModif2.clone(nullptr));
}


} // !cp
