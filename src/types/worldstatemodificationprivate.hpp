#ifndef INCLUDE_CONTEXTUALPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP
#define INCLUDE_CONTEXTUALPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP

#include <memory>
#include <optional>
#include <contextualplanner/types/worldstatemodification.hpp>
#include <contextualplanner/util/util.hpp>

namespace cp
{

enum class WorldStateModificationNodeType
{
  AND,
  ASSIGN,
  FOR_ALL,
  INCREASE,
  DECREASE,
  MULTIPLY,
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
      parameterOpt(pParameterOpt),
      _successions()
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
        nodeType == WorldStateModificationNodeType::MULTIPLY ||
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

  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const SetOfFacts& pSetOfFact) const override;

  void forAllThatCanBeModified(const std::function<void (const FactOptional&)>& pFactCallback) const override;

  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>& pFactCallback,
                                     const SetOfFacts& pSetOfFact) const;
  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback,
                       const SetOfFacts& pSetOfFact) const override;
  bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pFactCallback,
                           std::map<Parameter, std::set<Entity>>& pParameters,
                           const WorldState& pWorldState,
                           const std::string& pFromDeductionId) const override;
  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pCallback,
                            std::map<Parameter, std::set<Entity>>& pParameters,
                            const WorldState& pWorldState,
                            const std::string& pFromDeductionId) const override;
  void updateSuccesions(const Domain& pDomain,
                        const WorldStateModificationContainerId& pContainerId,
                        const std::set<FactOptional>& pOptionalFactsToIgnore) override;
  void removePossibleSuccession(const ActionId& pActionIdToRemove) override;
  void printSuccesions(std::string& pRes) const override;

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const SetOfFacts& pSetOfFact) const override;

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
  Successions _successions;

  void _forAllInstruction(const std::function<void (const WorldStateModification&)>& pCallback,
                          const SetOfFacts& pSetOfFact) const;
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

  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const SetOfFacts&) const override { pFactCallback(factOptional); }

  void forAllThatCanBeModified(const std::function<void (const FactOptional&)>& pFactCallback) const override { pFactCallback(factOptional); }

  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>& pFactCallback,
                                     const SetOfFacts&) const override { pFactCallback(factOptional); }


  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>& pFactCallback, const SetOfFacts&) const override
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

  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pCallback,
                            std::map<Parameter, std::set<Entity>>&,
                            const WorldState&,
                            const std::string&) const override
  {
    if (!_successions.empty())
       return pCallback(_successions, factOptional, nullptr, [](const std::map<Parameter, std::set<Entity>>&){ return true; });
    return false;
  }

  void updateSuccesions(const Domain& pDomain,
                        const WorldStateModificationContainerId& pContainerId,
                        const std::set<FactOptional>& pOptionalFactsToIgnore) override
  {
    _successions.clear();
    _successions.addSuccesionsOptFact(factOptional, pDomain, pContainerId, pOptionalFactsToIgnore);
  }

  void removePossibleSuccession(const ActionId& pActionIdToRemove) override
  {
    _successions.actions.erase(pActionIdToRemove);
  }

  void printSuccesions(std::string& pRes) const override
  {
    _successions.print(pRes, factOptional);
  }

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const SetOfFacts& pSetOfFact) const override;

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

private:
  Successions _successions;
};



struct WorldStateModificationNumber : public WorldStateModification
{
  WorldStateModificationNumber(const Number& pNb)
    : WorldStateModification(),
      _nb(pNb)
  {
  }

  static std::unique_ptr<WorldStateModificationNumber> create(const std::string& pStr);

  std::string toStr(bool) const override;

  bool hasFact(const cp::Fact&) const override { return false; }
  bool hasFactOptional(const cp::FactOptional&) const override { return false; }
  bool isOnlyASetOfFacts() const override { return false; }

  void replaceArgument(const Entity&,
                       const Entity&) override {}
  void forAll(const std::function<void (const FactOptional&)>&,
              const SetOfFacts&) const override {}
  void forAllThatCanBeModified(const std::function<void (const FactOptional&)>&) const override {}
  void iterateOverAllAccessibleFacts(const std::function<void (const FactOptional&)>&,
                                     const SetOfFacts&) const override {}
  bool forAllUntilTrue(const std::function<bool (const FactOptional&)>&,
                       const SetOfFacts&) const override { return false; }
  bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>&,
                           std::map<Parameter, std::set<Entity>>&,
                           const WorldState&,
                           const std::string&) const override { return false; }
  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>&,
                            std::map<Parameter, std::set<Entity>>&,
                            const WorldState&,
                            const std::string&) const override { return false; }
  void updateSuccesions(const Domain&,
                        const WorldStateModificationContainerId&,
                        const std::set<FactOptional>&) override {}
  void removePossibleSuccession(const ActionId&) override {}
  void printSuccesions(std::string& pRes) const override {}

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getFluent(const SetOfFacts&) const override
  {
    return Entity::createNumberEntity(toStr(true));
  }

  const FactOptional* getOptionalFact() const override
  {
    return nullptr;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>*) const override
  {
    return std::make_unique<WorldStateModificationNumber>(_nb);
  }

  std::unique_ptr<WorldStateModification> cloneParamSet(const std::map<Parameter, std::set<Entity>>&) const override
  {
    return std::make_unique<WorldStateModificationNumber>(_nb);
  }

private:
  Number _nb;
};


} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP
