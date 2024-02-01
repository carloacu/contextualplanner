#include <contextualplanner/contextualplanner.hpp>
#include <algorithm>
#include <optional>
#include <contextualplanner/types/setofinferences.hpp>
#include <contextualplanner/util/util.hpp>
#include "types/factsalreadychecked.hpp"
#include "types/treeofalreadydonepaths.hpp"

namespace cp
{

namespace
{

enum class PossibleEffect
{
  SATISFIED,
  SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD,
  NOT_SATISFIED
};

PossibleEffect _merge(PossibleEffect pEff1,
                      PossibleEffect pEff2)
{
  if (pEff1 == PossibleEffect::SATISFIED ||
      pEff2 == PossibleEffect::SATISFIED)
    return PossibleEffect::SATISFIED;
  if (pEff1 == PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD ||
      pEff2 == PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD)
    return PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD;
  return PossibleEffect::NOT_SATISFIED;
}

struct PlanCost
{
  bool success = true;
  std::size_t nbOfGoalsNotSatisfied = 0;
  std::size_t nbOfGoalsSatisfied = 0;
  std::size_t nbOfActionDones = 0;

  bool isBetterThan(const PlanCost& pOther) const
  {
    if (success != pOther.success)
      return success;
    if (nbOfGoalsNotSatisfied != pOther.nbOfGoalsNotSatisfied)
      return nbOfGoalsNotSatisfied > pOther.nbOfGoalsNotSatisfied;
    if (nbOfGoalsSatisfied != pOther.nbOfGoalsSatisfied)
      return nbOfGoalsSatisfied > pOther.nbOfGoalsSatisfied;
    return nbOfActionDones < pOther.nbOfActionDones;
  }
};

struct PotentialNextActionComparisonCache
{
  PlanCost currentCost;
  std::list<const ProblemModification*> effectsWithWorseCosts;
};


struct PotentialNextAction
{
  PotentialNextAction()
    : actionId(""),
      actionPtr(nullptr),
      parameters(),
      satisfyObjective(false)
  {
  }
  PotentialNextAction(const ActionId& pActionId,
                      const Action& pAction);

  ActionId actionId;
  const Action* actionPtr;
  std::map<std::string, std::set<std::string>> parameters;
  bool satisfyObjective;

  bool isMoreImportantThan(const PotentialNextAction& pOther,
                           const Problem& pProblem,
                           const Historical* pGlobalHistorical) const;
};


PotentialNextAction::PotentialNextAction(const ActionId& pActionId,
                                         const Action& pAction)
  : actionId(pActionId),
    actionPtr(&pAction),
    parameters(),
    satisfyObjective(false)
{
  for (const auto& currParam : pAction.parameters)
    parameters[currParam];
}


void _getPreferInContextStatistics(std::size_t& nbOfPreconditionsSatisfied,
                                   std::size_t& nbOfPreconditionsNotSatisfied,
                                   const Action& pAction,
                                   const std::set<Fact>& pFacts)
{
  auto onFact = [&](const FactOptional& pFactOptional)
  {
    if (pFactOptional.isFactNegated)
    {
      if (pFacts.count(pFactOptional.fact) == 0)
        ++nbOfPreconditionsSatisfied;
      else
        ++nbOfPreconditionsNotSatisfied;
    }
    else
    {
      if (pFacts.count(pFactOptional.fact) > 0)
        ++nbOfPreconditionsSatisfied;
      else
        ++nbOfPreconditionsNotSatisfied;
    }
  };

  if (pAction.preferInContext)
    pAction.preferInContext->forAll(onFact);
}


bool PotentialNextAction::isMoreImportantThan(const PotentialNextAction& pOther,
                                              const Problem& pProblem,
                                              const Historical* pGlobalHistorical) const
{
  if (actionPtr == nullptr)
    return false;
  auto& action = *actionPtr;
  if (pOther.actionPtr == nullptr)
    return true;
  auto& otherAction = *pOther.actionPtr;

  auto nbOfTimesAlreadyDone = pProblem.historical.getNbOfTimeAnActionHasAlreadyBeenDone(actionId);
  auto otherNbOfTimesAlreadyDone = pProblem.historical.getNbOfTimeAnActionHasAlreadyBeenDone(pOther.actionId);

  if (action.highImportanceOfNotRepeatingIt)
  {
    if (otherAction.highImportanceOfNotRepeatingIt)
    {
      if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
        return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;
    }
    else if (nbOfTimesAlreadyDone > 0)
    {
      return false;
    }
  }
  else if (otherAction.highImportanceOfNotRepeatingIt && otherNbOfTimesAlreadyDone > 0)
  {
    return true;
  }

  // Compare according to prefer in context
  std::size_t nbOfPreferInContextSatisfied = 0;
  std::size_t nbOfPreferInContextNotSatisfied = 0;
  _getPreferInContextStatistics(nbOfPreferInContextSatisfied, nbOfPreferInContextNotSatisfied, action, pProblem.worldState.facts());
  std::size_t otherNbOfPreconditionsSatisfied = 0;
  std::size_t otherNbOfPreconditionsNotSatisfied = 0;
  _getPreferInContextStatistics(otherNbOfPreconditionsSatisfied, otherNbOfPreconditionsNotSatisfied, otherAction, pProblem.worldState.facts());
  if (nbOfPreferInContextSatisfied != otherNbOfPreconditionsSatisfied)
    return nbOfPreferInContextSatisfied > otherNbOfPreconditionsSatisfied;
  if (nbOfPreferInContextNotSatisfied != otherNbOfPreconditionsNotSatisfied)
    return nbOfPreferInContextNotSatisfied < otherNbOfPreconditionsNotSatisfied;

  if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
    return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;

  if (pGlobalHistorical != nullptr)
  {
    nbOfTimesAlreadyDone = pGlobalHistorical->getNbOfTimeAnActionHasAlreadyBeenDone(actionId);
    otherNbOfTimesAlreadyDone = pGlobalHistorical->getNbOfTimeAnActionHasAlreadyBeenDone(pOther.actionId);
    if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
      return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;
  }
  return actionId < pOther.actionId;
}


bool _lookForAPossibleEffect(bool& pSatisfyObjective,
                             std::map<std::string, std::set<std::string>>& pParameters,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const ProblemModification& pEffectToCheck,
                             const Goal& pGoal,
                             const Problem& pProblem,
                             const FactOptional& pFactOptionalToSatisfy,
                             const Domain& pDomain,
                             FactsAlreadyChecked& pFactsAlreadychecked);


PossibleEffect _lookForAPossibleDeduction(TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                                          const std::vector<std::string>& pParameters,
                                          const std::unique_ptr<Condition>& pCondition,
                                          const ProblemModification& pEffect,
                                          const FactOptional& pFactOptional,
                                          std::map<std::string, std::set<std::string>>& pParentParameters,
                                          const Goal& pGoal,
                                          const Problem& pProblem,
                                          const FactOptional& pFactOptionalToSatisfy,
                                          const Domain& pDomain,
                                          FactsAlreadyChecked& pFactsAlreadychecked)
{
  if (!pCondition ||
      (pCondition->containsFactOpt(pFactOptional, pParentParameters, pParameters) &&
       pCondition->canBecomeTrue(pProblem.worldState)))
  {
    std::map<std::string, std::set<std::string>> parametersToValues;
    for (const auto& currParam : pParameters)
      parametersToValues[currParam];

    bool satisfyObjective = false;
    if (_lookForAPossibleEffect(satisfyObjective, parametersToValues, pTreeOfAlreadyDonePath, pEffect,
                                pGoal, pProblem, pFactOptionalToSatisfy, pDomain, pFactsAlreadychecked))
    {
      bool tryToMatchWothFactOfTheWorld = false;
      while (true)
      {
        bool actionIsAPossibleFollowUp = true;
        // fill parent parameters
        for (auto& currParentParam : pParentParameters)
        {
          if (currParentParam.second.empty() &&
              pFactOptional.fact.hasArgumentOrValue(currParentParam.first))
          {
            pCondition->untilFalse(
                  [&](const FactOptional& pConditionFactOptional)
            {
              auto parentParamValue = pFactOptional.fact.tryToExtractArgumentFromExample(currParentParam.first, pConditionFactOptional.fact);
              if (parentParamValue.empty())
                return true;
              // Maybe the extracted parameter is also a parameter so we replace by it's value
              auto itParam = parametersToValues.find(parentParamValue);
              if (itParam != parametersToValues.end())
                currParentParam.second = itParam->second;
              else
                currParentParam.second.insert(parentParamValue);
              return currParentParam.second.empty();
            }, pProblem.worldState, parametersToValues);

            if (!currParentParam.second.empty())
              break;
            if (currParentParam.second.empty())
            {
              actionIsAPossibleFollowUp = false;
              break;
            }
          }
        }

        // Check that the new fact pattern is not already satisfied
        if (actionIsAPossibleFollowUp)
        {
          if (!pProblem.worldState.isOptionalFactSatisfiedInASpecificContext(pFactOptional, {}, {}, &pParentParameters, nullptr))
            return PossibleEffect::SATISFIED;
          return PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD;
        }

        // If we did not succedded to fill the parameters from effet we try to resolve according to the constant facts in the world
        if (tryToMatchWothFactOfTheWorld)
          break;
        tryToMatchWothFactOfTheWorld = true;
        const auto& swFactNamesToFacts = pProblem.worldState.factNamesToFacts();
        pCondition->forAll([&](const FactOptional& pConditionFactOptional) {
          if (!pConditionFactOptional.isFactNegated &&
              !pProblem.worldState.canFactNameBeModified(pConditionFactOptional.fact.name))
          {
            auto itNameToWorldFacts = swFactNamesToFacts.find(pConditionFactOptional.fact.name);
            if (itNameToWorldFacts != swFactNamesToFacts.end())
            {
              for (const auto& currWorldFact : itNameToWorldFacts->second)
              {
                if (pConditionFactOptional.fact.isPatternOf(parametersToValues, currWorldFact))
                {
                  for (auto& currParamToValues : parametersToValues)
                  {
                    auto parentParamValue = pConditionFactOptional.fact.tryToExtractArgumentFromExample(currParamToValues.first, currWorldFact);
                    if (!parentParamValue.empty())
                      currParamToValues.second.insert(parentParamValue);
                  }
                }
              }
            }
          }
        });
      }
    }
  }
  return PossibleEffect::NOT_SATISFIED;
}

PossibleEffect _lookForAPossibleExistingOrNotFactFromActions(
    const FactOptional& pFactOptional,
    std::map<std::string, std::set<std::string>>& pParentParameters,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const std::map<std::string, std::set<ActionId>>& pPreconditionToActions,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto res = PossibleEffect::NOT_SATISFIED;
  auto it = pPreconditionToActions.find(pFactOptional.fact.name);
  if (it != pPreconditionToActions.end())
  {
    auto& actions = pDomain.actions();
    for (const auto& currActionId : it->second)
    {
      auto itAction = actions.find(currActionId);
      if (itAction != actions.end())
      {
        auto& action = itAction->second;
        auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currActionId);
        if (newTreePtr != nullptr)
          res = _merge(_lookForAPossibleDeduction(*newTreePtr, action.parameters, action.precondition,
                                                  action.effect, pFactOptional, pParentParameters, pGoal,
                                                  pProblem, pFactOptionalToSatisfy,
                                                  pDomain, pFactsAlreadychecked), res);
        if (res == PossibleEffect::SATISFIED)
          return res;
      }
    }
  }
  return res;
}


PossibleEffect _lookForAPossibleExistingOrNotFactFromInferences(
    const FactOptional& pFactOptional,
    std::map<std::string, std::set<std::string>>& pParentParameters,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const std::map<std::string, std::set<InferenceId>>& pConditionToInferences,
    const std::map<InferenceId, Inference>& pInferences,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto res = PossibleEffect::NOT_SATISFIED;
  auto it = pConditionToInferences.find(pFactOptional.fact.name);
  if (it != pConditionToInferences.end())
  {
    for (const auto& currInferenceId : it->second)
    {
      auto itInference = pInferences.find(currInferenceId);
      if (itInference != pInferences.end())
      {
        auto& inference = itInference->second;
        if (inference.factsToModify)
        {
          auto* newTreePtr = pTreeOfAlreadyDonePath.getNextInflectionTreeIfNotAnExistingLeaf(currInferenceId);
          if (newTreePtr != nullptr)
            res = _merge(_lookForAPossibleDeduction(*newTreePtr, inference.parameters, inference.condition,
                                                    inference.factsToModify->clone(nullptr), pFactOptional,
                                                    pParentParameters, pGoal, pProblem, pFactOptionalToSatisfy,
                                                    pDomain, pFactsAlreadychecked), res);
          if (res == PossibleEffect::SATISFIED)
            return res;
        }
      }
    }
  }
  return res;
}


bool _lookForAPossibleEffect(bool& pSatisfyObjective,
                             std::map<std::string, std::set<std::string>>& pParameters,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const ProblemModification& pEffectToCheck,
                             const Goal& pGoal,
                             const Problem& pProblem,
                             const FactOptional& pFactOptionalToSatisfy,
                             const Domain& pDomain,
                             FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto doesSatisfyObjective = [&](const FactOptional& pFactOptional)
  {
    if (pFactOptionalToSatisfy.isFactNegated != pFactOptional.isFactNegated)
      return false;
    const ConditionNode* objNodePtr = pGoal.objective().fcNodePtr();
    ConditionNodeType objNodeType = objNodePtr != nullptr ? objNodePtr->nodeType : ConditionNodeType::AND;
    bool objIsAComparison = objNodeType == ConditionNodeType::SUPERIOR || objNodeType == ConditionNodeType::INFERIOR;
    std::map<std::string, std::set<std::string>> newParameters;
    bool res = pFactOptionalToSatisfy.fact.isInOtherFact(pFactOptional.fact, false, &newParameters, &pParameters, nullptr, objIsAComparison);
    if (res && objIsAComparison && objNodePtr != nullptr && objNodePtr->rightOperand)
    {
      const auto* objValPtr = objNodePtr->rightOperand->fcNbPtr();
      if (objValPtr != nullptr)
        res = compIntNb(pFactOptional.fact.value, objValPtr->nb, objNodeType == ConditionNodeType::SUPERIOR);
    }
    applyNewParams(pParameters, newParameters);
    return res;
  };

  if (pEffectToCheck.worldStateModification &&
      pEffectToCheck.worldStateModification->forAllUntilTrue(doesSatisfyObjective, pProblem.worldState))
  {
    pSatisfyObjective = true;
    return true;
  }
  if (pEffectToCheck.potentialWorldStateModification &&
      pEffectToCheck.potentialWorldStateModification->forAllUntilTrue(doesSatisfyObjective, pProblem.worldState))
  {
    pSatisfyObjective = true;
    return true;
  }

  auto& setOfInferences = pDomain.getSetOfInferences();
  return pEffectToCheck.forAllUntilTrue([&](const cp::FactOptional& pFactOptional) {
    // Condition only for optimization
    if (pParameters.empty())
    {
      if (!pFactOptional.isFactNegated)
      {
        if (pProblem.worldState.facts().count(pFactOptional.fact) > 0)
          return false;
      }
      else
      {
        if (pProblem.worldState.facts().count(pFactOptional.fact) == 0)
          return false;
      }
    }

    if ((!pFactOptional.isFactNegated && pFactsAlreadychecked.factsToAdd.count(pFactOptional.fact) == 0) ||
        (pFactOptional.isFactNegated && pFactsAlreadychecked.factsToRemove.count(pFactOptional.fact) == 0))
    {
      FactsAlreadyChecked subFactsAlreadychecked = pFactsAlreadychecked;
      if (!pFactOptional.isFactNegated)
        subFactsAlreadychecked.factsToAdd.insert(pFactOptional.fact);
      else
        subFactsAlreadychecked.factsToRemove.insert(pFactOptional.fact);

      auto& preconditionToActions = !pFactOptional.isFactNegated ? pDomain.preconditionToActions() : pDomain.notPreconditionToActions();
      PossibleEffect possibleEffect = _lookForAPossibleExistingOrNotFactFromActions(pFactOptional, pParameters, pTreeOfAlreadyDonePath,
                                                                                    preconditionToActions, pGoal, pProblem, pFactOptionalToSatisfy,
                                                                                    pDomain, subFactsAlreadychecked);
      if (possibleEffect != PossibleEffect::SATISFIED)
      {
        for (auto& currSetOfInferences : setOfInferences)
        {
          auto& inferences = currSetOfInferences.second.inferences();
          auto& conditionToReachableInferences = !pFactOptional.isFactNegated ?
                currSetOfInferences.second.reachableInferenceLinks().conditionToInferences :
                currSetOfInferences.second.reachableInferenceLinks().notConditionToInferences;
          possibleEffect = _merge(_lookForAPossibleExistingOrNotFactFromInferences(pFactOptional, pParameters, pTreeOfAlreadyDonePath,
                                                                                   conditionToReachableInferences, inferences,
                                                                                   pGoal, pProblem, pFactOptionalToSatisfy,
                                                                                   pDomain, subFactsAlreadychecked), possibleEffect);
          if (possibleEffect == PossibleEffect::SATISFIED)
            break;
          auto& conditionToUnreachableInferences = !pFactOptional.isFactNegated ?
                currSetOfInferences.second.unreachableInferenceLinks().conditionToInferences :
                currSetOfInferences.second.unreachableInferenceLinks().notConditionToInferences;
          possibleEffect = _merge(_lookForAPossibleExistingOrNotFactFromInferences(pFactOptional, pParameters, pTreeOfAlreadyDonePath,
                                                                                   conditionToUnreachableInferences, inferences,
                                                                                   pGoal, pProblem, pFactOptionalToSatisfy,
                                                                                   pDomain, subFactsAlreadychecked), possibleEffect);
          if (possibleEffect == PossibleEffect::SATISFIED)
            break;
        }
      }

      if (possibleEffect != PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD)
        pFactsAlreadychecked.swap(subFactsAlreadychecked);
      return possibleEffect == PossibleEffect::SATISFIED;
    }
    return false;
  }, pProblem.worldState);
}

void _notifyActionDone(Problem& pProblem,
                       bool& pGoalChanged,
                       const std::map<SetOfInferencesId, SetOfInferences>& pSetOfInferences,
                       const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                       const std::unique_ptr<WorldStateModification>& pEffect,
                       const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                       const std::map<int, std::vector<Goal>>* pGoalsToAdd,
                       const std::vector<Goal>* pGoalsToAddInCurrentPriority,
                       LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  pProblem.historical.notifyActionDone(pOnStepOfPlannerResult.actionInvocation.actionId);

  pProblem.worldState.notifyActionDone(pOnStepOfPlannerResult, pEffect, pGoalChanged, pProblem.goalStack, pSetOfInferences, pNow);

  pGoalChanged = pProblem.goalStack.notifyActionDone(pOnStepOfPlannerResult, pNow, pGoalsToAdd,
                                                     pGoalsToAddInCurrentPriority, pProblem.worldState, pLookForAnActionOutputInfosPtr) || pGoalChanged;
}

void _updateProblemForNextPotentialPlannerResult(
    Problem& pProblem,
    bool& pGoalChanged,
    const ActionInvocationWithGoal& pOneStepOfPlannerResult,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  auto itAction = pDomain.actions().find(pOneStepOfPlannerResult.actionInvocation.actionId);
  if (itAction != pDomain.actions().end())
  {
    if (pGlobalHistorical != nullptr)
      pGlobalHistorical->notifyActionDone(pOneStepOfPlannerResult.actionInvocation.actionId);
    auto& setOfInferences = pDomain.getSetOfInferences();
    _notifyActionDone(pProblem, pGoalChanged, setOfInferences, pOneStepOfPlannerResult, itAction->second.effect.worldStateModification, pNow,
                      &itAction->second.effect.goalsToAdd, &itAction->second.effect.goalsToAddInCurrentPriority,
                      pLookForAnActionOutputInfosPtr);

    if (itAction->second.effect.potentialWorldStateModification)
    {
      auto potentialEffect = itAction->second.effect.potentialWorldStateModification->cloneParamSet(pOneStepOfPlannerResult.actionInvocation.parameters);
      pProblem.worldState.modify(potentialEffect, pProblem.goalStack, setOfInferences, pNow);
    }
  }
}


PlanCost _extractPlanCost(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos& pLookForAnActionOutputInfos)
{
  PlanCost res;
  const bool tryToDoMoreOptimalSolution = false;
  std::set<std::string> actionAlreadyInPlan;
  bool shouldBreak = false;
  while (!pProblem.goalStack.goals().empty())
  {
    if (shouldBreak)
    {
      res.success = false;
      break;
    }
    auto subPlan = planForMoreImportantGoalPossible(pProblem, pDomain, tryToDoMoreOptimalSolution,
                                                    pNow, pGlobalHistorical, &pLookForAnActionOutputInfos);
    if (subPlan.empty())
      break;
    for (const auto& currActionInSubPlan : subPlan)
    {
      ++res.nbOfActionDones;
      const auto& actionToDoStr = currActionInSubPlan.actionInvocation.toStr();
      if (actionAlreadyInPlan.count(actionToDoStr) > 0)
        shouldBreak = true;
      actionAlreadyInPlan.insert(actionToDoStr);
      bool goalChanged = false;
      _updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical,
                                                  &pLookForAnActionOutputInfos);
      if (goalChanged)
        break;
    }
  }

  res.success = pLookForAnActionOutputInfos.isFirstGoalInSuccess();
  res.nbOfGoalsNotSatisfied = pLookForAnActionOutputInfos.nbOfNotSatisfiedGoals();
  res.nbOfGoalsSatisfied = pLookForAnActionOutputInfos.nbOfSatisfiedGoals();
  return res;
}


bool _isMoreOptimalNextAction(
    std::optional<PotentialNextActionComparisonCache>& pPotentialNextActionComparisonCacheOpt,
    const PotentialNextAction& pNewPotentialNextAction, // not const because cache cost can be modified
    const PotentialNextAction& pCurrentNextAction, // not const because cache cost can be modified
    const Problem& pProblem,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    std::size_t pLength,
    const Historical* pGlobalHistorical)
{
  if (pTryToDoMoreOptimalSolution &&
      pLength == 0 &&
      pNewPotentialNextAction.actionPtr != nullptr &&
      pCurrentNextAction.actionPtr != nullptr &&
      pNewPotentialNextAction.actionPtr->effect != pCurrentNextAction.actionPtr->effect)
  {
    ActionInvocationWithGoal oneStepOfPlannerResult1(pNewPotentialNextAction.actionId, pNewPotentialNextAction.parameters, {}, 0);
    ActionInvocationWithGoal oneStepOfPlannerResult2(pCurrentNextAction.actionId, pCurrentNextAction.parameters, {}, 0);
    std::unique_ptr<std::chrono::steady_clock::time_point> now;

    PlanCost newCost;
    {
      auto localProblem1 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      _updateProblemForNextPotentialPlannerResult(localProblem1, goalChanged, oneStepOfPlannerResult1, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      newCost = _extractPlanCost(localProblem1, pDomain, now, nullptr, lookForAnActionOutputInfos);
    }

    if (!pPotentialNextActionComparisonCacheOpt)
    {
      auto localProblem2 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      _updateProblemForNextPotentialPlannerResult(localProblem2, goalChanged, oneStepOfPlannerResult2, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      pPotentialNextActionComparisonCacheOpt = PotentialNextActionComparisonCache();
      pPotentialNextActionComparisonCacheOpt->currentCost = _extractPlanCost(localProblem2, pDomain, now, nullptr, lookForAnActionOutputInfos);
    }

    if (newCost.isBetterThan(pPotentialNextActionComparisonCacheOpt->currentCost))
    {
      pPotentialNextActionComparisonCacheOpt->currentCost = newCost;
      pPotentialNextActionComparisonCacheOpt->effectsWithWorseCosts.push_back(&pCurrentNextAction.actionPtr->effect);
      return true;
    }
    if (pPotentialNextActionComparisonCacheOpt->currentCost.isBetterThan(newCost))
    {
      pPotentialNextActionComparisonCacheOpt->effectsWithWorseCosts.push_back(&pNewPotentialNextAction.actionPtr->effect);
      return false;
    }
  }

  return pNewPotentialNextAction.isMoreImportantThan(pCurrentNextAction, pProblem, pGlobalHistorical);
}


void _nextStepOfTheProblemForAGoalAndSetOfActions(PotentialNextAction& pCurrentResult,
                                                  std::optional<PotentialNextActionComparisonCache>& pPotentialNextActionComparisonCacheOpt,
                                                  TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                                                  const std::set<ActionId>& pActions,
                                                  const Goal& pGoal,
                                                  const Problem& pProblem,
                                                  const FactOptional& pFactOptionalToSatisfy,
                                                  const Domain& pDomain,
                                                  bool pTryToDoMoreOptimalSolution,
                                                  std::size_t pLength,
                                                  const Historical* pGlobalHistorical)
{
  PotentialNextAction newPotNextAction;
  auto& domainActions = pDomain.actions();
  for (const auto& currAction : pActions)
  {
    auto itAction = domainActions.find(currAction);
    if (itAction != domainActions.end())
    {
      auto& action = itAction->second;
      FactsAlreadyChecked factsAlreadyChecked;
      auto newPotRes = PotentialNextAction(currAction, action);
      auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currAction);
      if (newTreePtr != nullptr && // To skip leaf of already seen path
          _lookForAPossibleEffect(newPotRes.satisfyObjective, newPotRes.parameters, *newTreePtr,
                                  action.effect, pGoal, pProblem, pFactOptionalToSatisfy,
                                  pDomain, factsAlreadyChecked) &&
          (!action.precondition || action.precondition->isTrue(pProblem.worldState, {}, {}, &newPotRes.parameters)))
      {
        if (_isMoreOptimalNextAction(pPotentialNextActionComparisonCacheOpt, newPotRes, newPotNextAction, pProblem, pDomain, pTryToDoMoreOptimalSolution, pLength, pGlobalHistorical))
        {
          assert(newPotRes.actionPtr != nullptr);
          newPotNextAction = newPotRes;
        }
      }
    }
  }

  if (_isMoreOptimalNextAction(pPotentialNextActionComparisonCacheOpt, newPotNextAction, pCurrentResult, pProblem, pDomain, pTryToDoMoreOptimalSolution, pLength, pGlobalHistorical))
  {
    assert(newPotNextAction.actionPtr != nullptr);
    pCurrentResult = newPotNextAction;
  }
}


ActionId _nextStepOfTheProblemForAGoal(
    std::map<std::string, std::set<std::string>>& pParameters,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    std::size_t pLength,
    const Historical* pGlobalHistorical)
{
  PotentialNextAction res;
  std::optional<PotentialNextActionComparisonCache> potentialNextActionComparisonCacheOpt;
  for (const auto& currFact : pProblem.worldState.factNamesToFacts())
  {
    auto itPrecToActions = pDomain.preconditionToActions().find(currFact.first);
    if (itPrecToActions != pDomain.preconditionToActions().end())
      _nextStepOfTheProblemForAGoalAndSetOfActions(res, potentialNextActionComparisonCacheOpt,
                                                   pTreeOfAlreadyDonePath,
                                                   itPrecToActions->second, pGoal,
                                                   pProblem, pFactOptionalToSatisfy,
                                                   pDomain, pTryToDoMoreOptimalSolution,
                                                   pLength, pGlobalHistorical);
  }
  auto& actionsWithoutFactToAddInPrecondition = pDomain.actionsWithoutFactToAddInPrecondition();
  _nextStepOfTheProblemForAGoalAndSetOfActions(res, potentialNextActionComparisonCacheOpt,
                                               pTreeOfAlreadyDonePath,
                                               actionsWithoutFactToAddInPrecondition, pGoal,
                                               pProblem, pFactOptionalToSatisfy,
                                               pDomain, pTryToDoMoreOptimalSolution,
                                               pLength, pGlobalHistorical);
  pParameters = std::move(res.parameters);
  return res.actionId;
}


bool _lookForAnActionToDoRec(
    std::list<ActionInvocationWithGoal>& pActionInvocations,
    Problem& pProblem,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    const Historical* pGlobalHistorical,
    const Goal& pGoal,
    int pPriority)
{
  pProblem.worldState.refreshCacheIfNeeded(pDomain);
  TreeOfAlreadyDonePath treeOfAlreadyDonePath;
  for (int i = 0; i < 100; ++i)
  {
    std::unique_ptr<ActionInvocationWithGoal> potentialRes;
    const FactOptional* factOptionalToSatisfyPtr = nullptr;
    pGoal.objective().untilFalse(
          [&](const FactOptional& pFactOptional)
    {
      if (!pProblem.worldState.isOptionalFactSatisfied(pFactOptional))
      {
        factOptionalToSatisfyPtr = &pFactOptional;
        return false;
      }
      return true;
    }, pProblem.worldState, {});

    if (factOptionalToSatisfyPtr != nullptr)
    {
      std::map<std::string, std::set<std::string>> parameters;
      auto actionId =
          _nextStepOfTheProblemForAGoal(parameters, treeOfAlreadyDonePath,
                                        pGoal, pProblem, *factOptionalToSatisfyPtr,
                                        pDomain, pTryToDoMoreOptimalSolution, 0, pGlobalHistorical);
      if (!actionId.empty())
        potentialRes = std::make_unique<ActionInvocationWithGoal>(actionId, parameters, pGoal.clone(), pPriority);
    }

    if (potentialRes && potentialRes->fromGoal)
    {
      auto problemForPlanCost = pProblem;
      bool goalChanged = false;
      _updateProblemForNextPotentialPlannerResult(problemForPlanCost, goalChanged, *potentialRes, pDomain, pNow, nullptr, nullptr);
      if (problemForPlanCost.worldState.isGoalSatisfied(pGoal) ||
          _lookForAnActionToDoRec(pActionInvocations, problemForPlanCost, pDomain, pTryToDoMoreOptimalSolution, pNow, nullptr, pGoal, pPriority))
      {
        potentialRes->fromGoal->notifyActivity();
        pActionInvocations.emplace_front(std::move(*potentialRes));
        return true;
      }
    }
    else
    {
      return false; // Fail to find an next action to do
    }
  }
  return false;
}

}


std::list<ActionInvocationWithGoal> planForMoreImportantGoalPossible(
    Problem& pProblem,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    const Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  std::list<ActionInvocationWithGoal> res;
  pProblem.goalStack.iterateOnGoalsAndRemoveNonPersistent(
        [&](const Goal& pGoal, int pPriority){
            return _lookForAnActionToDoRec(res, pProblem, pDomain, pTryToDoMoreOptimalSolution, pNow, pGlobalHistorical, pGoal, pPriority);
          },
        pProblem.worldState, pNow,
        pLookForAnActionOutputInfosPtr);
  return res;
}


void notifyActionDone(Problem& pProblem,
                      const Domain& pDomain,
                      const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  auto itAction = pDomain.actions().find(pOnStepOfPlannerResult.actionInvocation.actionId);
  if (itAction != pDomain.actions().end())
  {
    auto& setOfInferences = pDomain.getSetOfInferences();
    bool goalChanged = false;
    _notifyActionDone(pProblem, goalChanged, setOfInferences, pOnStepOfPlannerResult, itAction->second.effect.worldStateModification, pNow,
                      &itAction->second.effect.goalsToAdd, &itAction->second.effect.goalsToAddInCurrentPriority,
                      nullptr);
  }
}



std::list<ActionInvocationWithGoal> planForEveryGoals(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical)
{
  const bool tryToDoMoreOptimalSolution = true;
  std::map<std::string, std::size_t> actionAlreadyInPlan;
  std::list<ActionInvocationWithGoal> res;
  while (!pProblem.goalStack.goals().empty())
  {
    auto subPlan = planForMoreImportantGoalPossible(pProblem, pDomain, tryToDoMoreOptimalSolution,
                                                     pNow, pGlobalHistorical);
    if (subPlan.empty())
      break;
    for (auto& currActionInSubPlan : subPlan)
    {
      const auto& actionToDoStr = currActionInSubPlan.actionInvocation.toStr();
      auto itAlreadyFoundAction = actionAlreadyInPlan.find(actionToDoStr);
      if (itAlreadyFoundAction == actionAlreadyInPlan.end())
      {
        actionAlreadyInPlan[actionToDoStr] = 1;
      }
      else
      {
        if (itAlreadyFoundAction->second > 10)
          break;
        ++itAlreadyFoundAction->second;
      }
      bool goalChanged = false;
      _updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical, nullptr);
      res.emplace_back(std::move(currActionInSubPlan));
      if (goalChanged)
        break;
    }
  }
  return res;
}



std::string planToStr(const std::list<ActionInvocationWithGoal>& pPlan,
                      const std::string& pSep)
{
  auto size = pPlan.size();
  if (size == 1)
    return pPlan.front().actionInvocation.toStr();
  std::string res;
  bool firstIteration = true;
  for (const auto& currAction : pPlan)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += pSep;
    res += currAction.actionInvocation.toStr();
  }
  return res;
}


} // !cp
