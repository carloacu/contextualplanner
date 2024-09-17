#include <contextualplanner/types/successionscache.hpp>
#include <set>
#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/factoptional.hpp>
#include <contextualplanner/types/worldstatemodification.hpp>

namespace cp
{

void SuccessionsCache::clear()
{
  factsToSuccessions.clear();
}

void SuccessionsCache::update(const Domain& pDomain,
                              const ActionId& pActionIdToExclude,
                              const SetOfInferencesId& pSetOfInferencesIdToExclude,
                              const InferenceId& pInferenceIdToExclude,
                              const cp::Condition& pPrecondition,
                              const std::unique_ptr<cp::WorldStateModification>& pWorldModifPtr1,
                              const std::unique_ptr<cp::WorldStateModification>& pWorldModifPtr2)
{
  std::set<FactOptional> optionalFactsToIgnore;
  pPrecondition.forAll([&](const FactOptional& pFactOptional, bool pIgnoreFluent) {
    if (!pIgnoreFluent)
      optionalFactsToIgnore.insert(pFactOptional);
  });

  factsToSuccessions.clear();
  if (pWorldModifPtr1)
    _updateForAWsm(pDomain, pActionIdToExclude, pSetOfInferencesIdToExclude, pInferenceIdToExclude,
                   optionalFactsToIgnore, *pWorldModifPtr1);
  if (pWorldModifPtr2)
    _updateForAWsm(pDomain, pActionIdToExclude, pSetOfInferencesIdToExclude, pInferenceIdToExclude,
                   optionalFactsToIgnore, *pWorldModifPtr2);
}


std::string SuccessionsCache::print() const
{
  std::string res;
  for (const auto& currFactToSuccessions : factsToSuccessions)
  {
    if (res != "")
      res += "\n";

    res += "fact: " + currFactToSuccessions.first.toStr() + "\n";
    for (const auto& currActionId : currFactToSuccessions.second.actions)
      res += "action: " + currActionId + "\n";
    for (const auto& currInferenceSet : currFactToSuccessions.second.inferences)
      for (const auto& currInferenceId : currInferenceSet.second)
        res += "inference: " + currInferenceSet.first + "|" + currInferenceId + "\n";
  }
  return res;
}


void SuccessionsCache::_updateForAWsm(const Domain& pDomain,
                                      const ActionId& pActionIdToExclude,
                                      const SetOfInferencesId& pSetOfInferencesIdToExclude,
                                      const InferenceId& pInferenceIdToExclude,
                                      const std::set<FactOptional>& pOptionalFactsToIgnore,
                                      const cp::WorldStateModification& pWorldModif)
{
  pWorldModif.forAllFactOpt([&](const cp::FactOptional& pFactOptional) {
    if (pOptionalFactsToIgnore.count(pFactOptional) > 0)
      return;

    SuccessionsForAFact* successionsForAFactPtr = nullptr;
    auto& preconditionToActions = !pFactOptional.isFactNegated ? pDomain.preconditionToActions() : pDomain.notPreconditionToActions();
    auto actionsFromPreconditions = preconditionToActions.find(pFactOptional.fact);
    for (const auto& currActionId : actionsFromPreconditions)
    {
      if (currActionId != pActionIdToExclude)
      {
        if (successionsForAFactPtr == nullptr)
          successionsForAFactPtr = &factsToSuccessions[pFactOptional];
        successionsForAFactPtr->actions.insert(currActionId);
      }
    }

    auto& setOfInferences = pDomain.getSetOfInferences();
    for (auto& currSetOfInferences : setOfInferences)
    {
      bool isInSetOfInferenceToExclude = currSetOfInferences.first == pSetOfInferencesIdToExclude;
      std::set<InferenceId>* inferencesPtr = nullptr;

      auto& conditionToReachableInferences = !pFactOptional.isFactNegated ?
            currSetOfInferences.second.reachableInferenceLinks().conditionToInferences :
            currSetOfInferences.second.reachableInferenceLinks().notConditionToInferences;

      auto inferencesFromCondtion = conditionToReachableInferences.find(pFactOptional.fact);
      for (const auto& currInferenceId : inferencesFromCondtion)
      {
        if (!isInSetOfInferenceToExclude || currInferenceId != pInferenceIdToExclude)
        {
          if (successionsForAFactPtr == nullptr)
            successionsForAFactPtr = &factsToSuccessions[pFactOptional];
          if (inferencesPtr == nullptr)
            inferencesPtr = &successionsForAFactPtr->inferences[currSetOfInferences.first];
          inferencesPtr->insert(currInferenceId);
        }
      }
    }
  });
}


} // !cp
