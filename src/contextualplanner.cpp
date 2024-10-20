#include <contextualplanner/contextualplanner.hpp>
#include <algorithm>
#include <optional>
#include <contextualplanner/types/setofevents.hpp>
#include <contextualplanner/util/util.hpp>
#include "types/factsalreadychecked.hpp"
#include "types/treeofalreadydonepaths.hpp"
#include "algo/converttoparallelplan.hpp"
#include "algo/notifyactiondone.hpp"

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


struct ActionPtrWithGoal
{
  ActionPtrWithGoal(const Action* pActionPtr,
                    const cp::Goal& pGoal)
   : actionPtr(pActionPtr),
     goal(pGoal)
  {
  }

  const Action* actionPtr;
  const cp::Goal& goal;
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
  std::map<Parameter, std::set<Entity>> parameters;
  bool satisfyObjective;

  bool isMoreImportantThan(const PotentialNextAction& pOther,
                           const Problem& pProblem,
                           const Historical* pGlobalHistorical) const;
  bool removeAPossibility();
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

std::list<ActionInvocationWithGoal> _planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr,
                                                                     const ActionPtrWithGoal* pPreviousActionPtr);

void _getPreferInContextStatistics(std::size_t& nbOfPreconditionsSatisfied,
                                   std::size_t& nbOfPreconditionsNotSatisfied,
                                   const Action& pAction,
                                   const std::map<Fact, bool>& pFacts)
{
  auto onFact = [&](const FactOptional& pFactOptional,
                    bool)
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


bool PotentialNextAction::removeAPossibility()
{
  for (auto& currParam : parameters)
  {
    if (currParam.second.size() > 1)
    {
      currParam.second.erase(currParam.second.begin());
      return true;
    }
  }
  return false;
}




bool _lookForAPossibleEffect(bool& pSatisfyObjective,
                             std::map<Parameter, std::set<Entity>>& pParameters,
                             bool pTryToGetAllPossibleParentParameterValues,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const std::unique_ptr<WorldStateModification>& pWorldStateModificationPtr1,
                             const std::unique_ptr<WorldStateModification>& pWorldStateModificationPtr2,
                             const Goal& pGoal,
                             const Problem& pProblem,
                             const FactOptional& pFactOptionalToSatisfy,
                             const Domain& pDomain,
                             FactsAlreadyChecked& pFactsAlreadychecked,
                             const std::string& pFromDeductionId);


PossibleEffect _lookForAPossibleDeduction(TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                                          const std::vector<Parameter>& pParameters,
                                          const std::unique_ptr<Condition>& pCondition,
                                          const std::unique_ptr<cp::WorldStateModification>& pWorldStateModificationPtr1,
                                          const std::unique_ptr<cp::WorldStateModification>& pWorldStateModificationPtr2,
                                          const FactOptional& pFactOptional,
                                          std::map<Parameter, std::set<Entity>>& pParentParameters,
                                          std::map<Parameter, std::set<Entity>>* pTmpParentParametersPtr,
                                          const Goal& pGoal,
                                          const Problem& pProblem,
                                          const FactOptional& pFactOptionalToSatisfy,
                                          const Domain& pDomain,
                                          FactsAlreadyChecked& pFactsAlreadychecked,
                                          const std::string& pFromDeductionId)
{
  if (!pCondition ||
      (pCondition->containsFactOpt(pFactOptional, pParentParameters, pTmpParentParametersPtr, pParameters) &&
       pCondition->canBecomeTrue(pProblem.worldState, pParameters)))
  {
    std::map<Parameter, std::set<Entity>> parametersToValues;
    for (const auto& currParam : pParameters)
      parametersToValues[currParam];

    bool satisfyObjective = false;
    if (_lookForAPossibleEffect(satisfyObjective, parametersToValues, false, pTreeOfAlreadyDonePath,
                                pWorldStateModificationPtr1, pWorldStateModificationPtr2,
                                pGoal, pProblem, pFactOptionalToSatisfy, pDomain, pFactsAlreadychecked, pFromDeductionId))
    {
      bool tryToMatchWothFactOfTheWorld = false;
      while (true)
      {
        bool actionIsAPossibleFollowUp = true;

        auto fillParameter = [&](const Parameter& pParameter,
                                 std::set<Entity>& pParameterValues,
                                 std::map<Parameter, std::set<Entity>>& pNewParentParameters)
        {
          if (pParameterValues.empty() &&
              pFactOptional.fact.hasParameterOrFluent(pParameter))
          {
            auto& newParamValues = pNewParentParameters[pParameter];

            pCondition->findConditionCandidateFromFactFromEffect(
                  [&](const FactOptional& pConditionFactOptional)
            {
              auto parentParamValue = pFactOptional.fact.tryToExtractArgumentFromExample(pParameter, pConditionFactOptional.fact);
              if (!parentParamValue)
                return false;

              // Maybe the extracted parameter is also a parameter so we replace by it's value
              auto itParam = parametersToValues.find(parentParamValue->toParameter());
              if (itParam != parametersToValues.end())
                newParamValues = itParam->second;
              else
                newParamValues.insert(*parentParamValue);
              return !newParamValues.empty();
            }, pProblem.worldState, pFactOptional.fact, pParentParameters, pTmpParentParametersPtr, parametersToValues);

            if (newParamValues.empty())
              actionIsAPossibleFollowUp = false;
          }
        };

        // fill parent parameters
        std::map<Parameter, std::set<Entity>> newParentParameters;
        for (auto& currParentParam : pParentParameters)
          fillParameter(currParentParam.first, currParentParam.second, newParentParameters);

        if (pTmpParentParametersPtr != nullptr)
        {
          std::map<Parameter, std::set<Entity>> newTmpParentParameters;
          for (auto& currParentParam : *pTmpParentParametersPtr)
            fillParameter(currParentParam.first, currParentParam.second, newTmpParentParameters);
          applyNewParams(*pTmpParentParametersPtr, newTmpParentParameters);
        }
        applyNewParams(pParentParameters, newParentParameters);


        // Check that the new fact pattern is not already satisfied
        if (actionIsAPossibleFollowUp)
        {
          if (!pProblem.worldState.isOptionalFactSatisfiedInASpecificContext(pFactOptional, {}, {}, &pParentParameters, pTmpParentParametersPtr, nullptr))
            return PossibleEffect::SATISFIED;
          return PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD;
        }

        // If we did not succedded to fill the parameters from effect we try to resolve according to the constant facts in the world
        if (tryToMatchWothFactOfTheWorld)
          break;
        tryToMatchWothFactOfTheWorld = true;
        const auto& swFactAccessorsToFacts = pProblem.worldState.factsMapping();
        pCondition->forAll([&](const FactOptional& pConditionFactOptional, bool) {
          if (!pConditionFactOptional.isFactNegated)
          {
            auto condFactMatchInWs = swFactAccessorsToFacts.find(pConditionFactOptional.fact);
            for (const auto& currWorldFact : condFactMatchInWs)
            {
              if (pConditionFactOptional.fact.isPatternOf(parametersToValues, currWorldFact))
              {
                for (auto& currParamToValues : parametersToValues)
                {
                  auto parentParamValue = pConditionFactOptional.fact.tryToExtractArgumentFromExample(currParamToValues.first, currWorldFact);
                  if (parentParamValue)
                    currParamToValues.second.insert(*parentParamValue);
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
    const std::set<ActionId>& pActionSuccessions,
    const FactOptional& pFactOptional,
    std::map<Parameter, std::set<Entity>>& pParentParameters,
    std::map<Parameter, std::set<Entity>>* pTmpParentParametersPtr,
    bool pTryToGetAllPossibleParentParameterValues,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto res = PossibleEffect::NOT_SATISFIED;
  std::map<Parameter, std::set<Entity>> newPossibleParentParameters;
  std::map<Parameter, std::set<Entity>> newPossibleTmpParentParameters;
  auto& actions = pDomain.actions();
  for (const auto& currActionId : pActionSuccessions)
  {
    auto itAction = actions.find(currActionId);
    if (itAction != actions.end())
    {
      auto cpParentParameters = pParentParameters;
      std::map<Parameter, std::set<Entity>> cpTmpParameters;
      if (pTmpParentParametersPtr != nullptr)
        cpTmpParameters = *pTmpParentParametersPtr;
      auto& action = itAction->second;
      auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currActionId);
      if (newTreePtr != nullptr)
        res = _merge(_lookForAPossibleDeduction(*newTreePtr, action.parameters, action.precondition,
                                                action.effect.worldStateModification,
                                                action.effect.potentialWorldStateModification,
                                                pFactOptional, cpParentParameters, &cpTmpParameters,
                                                pGoal, pProblem, pFactOptionalToSatisfy,
                                                pDomain, pFactsAlreadychecked, currActionId), res);
      if (res == PossibleEffect::SATISFIED)
      {
        if (!pTryToGetAllPossibleParentParameterValues)
        {
          pParentParameters = std::move(cpParentParameters);
          if (pTmpParentParametersPtr != nullptr)
            *pTmpParentParametersPtr = std::move(cpTmpParameters);
          return res;
        }

        for (auto& currParam : cpParentParameters)
          newPossibleParentParameters[currParam.first].insert(currParam.second.begin(), currParam.second.end());
        if (pTmpParentParametersPtr != nullptr)
          for (auto& currParam : cpTmpParameters)
            newPossibleTmpParentParameters[currParam.first].insert(currParam.second.begin(), currParam.second.end());
      }
    }
  }
  if (!newPossibleParentParameters.empty())
  {
    pParentParameters = std::move(newPossibleParentParameters);
    if (pTmpParentParametersPtr != nullptr)
      *pTmpParentParametersPtr = std::move(newPossibleTmpParentParameters);
  }
  return res;
}


PossibleEffect _lookForAPossibleExistingOrNotFactFromEvents(
    const std::map<SetOfEventsId, std::set<EventId>>& pEventSuccessions,
    const FactOptional& pFactOptional,
    std::map<Parameter, std::set<Entity>>& pParentParameters,
    std::map<Parameter, std::set<Entity>>* pTmpParentParametersPtr,
    bool pTryToGetAllPossibleParentParameterValues,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const std::map<SetOfEventsId, SetOfEvents>& pEvents,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto res = PossibleEffect::NOT_SATISFIED;
  for (const auto& currSetOfEventsSucc : pEventSuccessions)
  {
    auto itSetOfEvents = pEvents.find(currSetOfEventsSucc.first);
    if (itSetOfEvents != pEvents.end())
    {
      for (const auto& currEventIdSucc : currSetOfEventsSucc.second)
      {
        const auto& currInfrences = itSetOfEvents->second.events();
        auto itEvent = currInfrences.find(currEventIdSucc);
        if (itEvent != currInfrences.end())
        {
          auto& event = itEvent->second;
          if (event.factsToModify)
          {
            auto* newTreePtr = pTreeOfAlreadyDonePath.getNextInflectionTreeIfNotAnExistingLeaf(currEventIdSucc);
            if (newTreePtr != nullptr)
              res = _merge(_lookForAPossibleDeduction(*newTreePtr, event.parameters, event.precondition,
                                                      event.factsToModify,
                                                      {}, pFactOptional,
                                                      pParentParameters, pTmpParentParametersPtr,
                                                      pGoal, pProblem, pFactOptionalToSatisfy,
                                                      pDomain, pFactsAlreadychecked, currEventIdSucc), res);
            if (res == PossibleEffect::SATISFIED && !pTryToGetAllPossibleParentParameterValues)
              return res;
          }
        }
      }
    }
  }
  return res;
}


bool _lookForAPossibleEffect(bool& pSatisfyObjective,
                             std::map<Parameter, std::set<Entity>>& pParameters,
                             bool pTryToGetAllPossibleParentParameterValues,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const std::unique_ptr<cp::WorldStateModification>& pWorldStateModificationPtr1,
                             const std::unique_ptr<cp::WorldStateModification>& pWorldStateModificationPtr2,
                             const Goal& pGoal,
                             const Problem& pProblem,
                             const FactOptional& pFactOptionalToSatisfy,
                             const Domain& pDomain,
                             FactsAlreadyChecked& pFactsAlreadychecked,
                             const std::string& pFromDeductionId)
{
  // Check if the effect satisfy the objective
  auto checkObjectiveCallback = [&](const FactOptional& pFactOptional, std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>& pCheckValidity)
  {
    if (pFactOptionalToSatisfy.isFactNegated != pFactOptional.isFactNegated)
      return pFactOptionalToSatisfy.fact.areEqualWithoutFluentConsideration(pFactOptional.fact) && pFactOptionalToSatisfy.fact.fluent() != pFactOptional.fact.fluent();

    const ConditionNode* objNodePtr = pGoal.objective().fcNodePtr();
    ConditionNodeType objNodeType = objNodePtr != nullptr ? objNodePtr->nodeType : ConditionNodeType::AND;
    bool objIsAComparison = objNodeType == ConditionNodeType::SUPERIOR || objNodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
        objNodeType == ConditionNodeType::INFERIOR || objNodeType == ConditionNodeType::INFERIOR_OR_EQUAL;
    std::map<Parameter, std::set<Entity>> newParameters;
    bool res = pFactOptionalToSatisfy.fact.isInOtherFact(pFactOptional.fact, false, &newParameters, &pParameters,
                                                         pParametersToModifyInPlacePtr, nullptr, objIsAComparison);

    if (res && pParametersToModifyInPlacePtr != nullptr && !pCheckValidity(*pParametersToModifyInPlacePtr))
      res = false;

    if (res && pFactOptional.fact.fluent() && objIsAComparison && objNodePtr != nullptr && objNodePtr->rightOperand)
    {
      const auto* objValPtr = objNodePtr->rightOperand->fcNbPtr();
      if (objValPtr != nullptr)
        res = compIntNb(pFactOptional.fact.fluent()->value, objValPtr->nb,
                        canBeSuperior(objNodeType), canBeEqual(objNodeType));
    }
    applyNewParams(pParameters, newParameters);
    return res;
  };

  if (pWorldStateModificationPtr1 &&
      pWorldStateModificationPtr1->canSatisfyObjective(checkObjectiveCallback, pParameters, pProblem.worldState, pFromDeductionId))
  {
    pSatisfyObjective = true;
    return true;
  }
  if (pWorldStateModificationPtr2 &&
      pWorldStateModificationPtr2->canSatisfyObjective(checkObjectiveCallback, pParameters, pProblem.worldState, pFromDeductionId))
  {
    pSatisfyObjective = true;
    return true;
  }

  // Iterate on possible successions
  auto& setOfEvents = pDomain.getSetOfEvents();
  auto successionsCallback = [&](const Successions& pSuccessions,
                                 const cp::FactOptional& pFactOptional,
                                 std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                                 const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>& pCheckValidity) {
      if ((!pFactOptional.isFactNegated && pFactsAlreadychecked.factsToAdd.count(pFactOptional.fact) == 0) ||
          (pFactOptional.isFactNegated && pFactsAlreadychecked.factsToRemove.count(pFactOptional.fact) == 0))
      {
        FactsAlreadyChecked subFactsAlreadychecked = pFactsAlreadychecked;
        if (!pFactOptional.isFactNegated)
          subFactsAlreadychecked.factsToAdd.insert(pFactOptional.fact);
        else
          subFactsAlreadychecked.factsToRemove.insert(pFactOptional.fact);

        PossibleEffect possibleEffect = PossibleEffect::NOT_SATISFIED;
        if (!pSuccessions.actions.empty())
        {
          possibleEffect = _lookForAPossibleExistingOrNotFactFromActions(pSuccessions.actions, pFactOptional, pParameters, pParametersToModifyInPlacePtr,
                                                                         pTryToGetAllPossibleParentParameterValues, pTreeOfAlreadyDonePath,
                                                                         pGoal, pProblem, pFactOptionalToSatisfy,
                                                                         pDomain, subFactsAlreadychecked);

          if (possibleEffect == PossibleEffect::SATISFIED && pParametersToModifyInPlacePtr != nullptr && !pCheckValidity(*pParametersToModifyInPlacePtr))
            possibleEffect = PossibleEffect::NOT_SATISFIED;
        }

        if (possibleEffect != PossibleEffect::SATISFIED && !pSuccessions.events.empty())
        {
          possibleEffect = _merge(_lookForAPossibleExistingOrNotFactFromEvents(pSuccessions.events, pFactOptional, pParameters, pParametersToModifyInPlacePtr,
                                                                                   pTryToGetAllPossibleParentParameterValues, pTreeOfAlreadyDonePath,
                                                                                   setOfEvents, pGoal, pProblem, pFactOptionalToSatisfy,
                                                                                   pDomain, subFactsAlreadychecked), possibleEffect);
          if (possibleEffect == PossibleEffect::SATISFIED && pParametersToModifyInPlacePtr != nullptr && !pCheckValidity(*pParametersToModifyInPlacePtr))
            possibleEffect = PossibleEffect::NOT_SATISFIED;
        }

        if (possibleEffect != PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD)
          pFactsAlreadychecked.swap(subFactsAlreadychecked);
        return possibleEffect == PossibleEffect::SATISFIED;
      }
      return false;
    };

  if (pWorldStateModificationPtr1)
    if (pWorldStateModificationPtr1->iterateOnSuccessions(successionsCallback, pParameters, pProblem.worldState, pFromDeductionId))
      return true;
  if (pWorldStateModificationPtr2)
    if (pWorldStateModificationPtr2->iterateOnSuccessions(successionsCallback, pParameters, pProblem.worldState, pFromDeductionId))
      return true;
  return false;
}


PlanCost _extractPlanCost(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos& pLookForAnActionOutputInfos,
    const ActionPtrWithGoal& pPreviousAction)
{
  PlanCost res;
  std::set<std::string> actionAlreadyInPlan;
  bool shouldBreak = false;
  while (!pProblem.goalStack.goals().empty())
  {
    if (shouldBreak)
    {
      res.success = false;
      break;
    }
    auto subPlan = _planForMoreImportantGoalPossible(pProblem, pDomain, false,
                                                     pNow, pGlobalHistorical, &pLookForAnActionOutputInfos, &pPreviousAction);
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
      updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical,
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
    const Goal& pCurrentGoal,
    const Historical* pGlobalHistorical)
{
  if (pTryToDoMoreOptimalSolution &&
      pLength == 0 &&
      pNewPotentialNextAction.actionPtr != nullptr &&
      pCurrentNextAction.actionPtr != nullptr &&
      (pNewPotentialNextAction.actionPtr->effect != pCurrentNextAction.actionPtr->effect ||
       pNewPotentialNextAction.parameters != pCurrentNextAction.parameters))
  {
    ActionInvocationWithGoal oneStepOfPlannerResult1(pNewPotentialNextAction.actionId, pNewPotentialNextAction.parameters, {}, 0);
    ActionInvocationWithGoal oneStepOfPlannerResult2(pCurrentNextAction.actionId, pCurrentNextAction.parameters, {}, 0);
    std::unique_ptr<std::chrono::steady_clock::time_point> now;

    PlanCost newCost;
    {
      auto localProblem1 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      updateProblemForNextPotentialPlannerResult(localProblem1, goalChanged, oneStepOfPlannerResult1, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      ActionPtrWithGoal actionPtrWithGoal(pNewPotentialNextAction.actionPtr, pCurrentGoal);
      newCost = _extractPlanCost(localProblem1, pDomain, now, nullptr, lookForAnActionOutputInfos, actionPtrWithGoal);
    }

    if (!pPotentialNextActionComparisonCacheOpt)
    {
      auto localProblem2 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      updateProblemForNextPotentialPlannerResult(localProblem2, goalChanged, oneStepOfPlannerResult2, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      pPotentialNextActionComparisonCacheOpt = PotentialNextActionComparisonCache();
      ActionPtrWithGoal actionPtrWithGoal(pCurrentNextAction.actionPtr, pCurrentGoal);
      pPotentialNextActionComparisonCacheOpt->currentCost = _extractPlanCost(localProblem2, pDomain, now, nullptr, lookForAnActionOutputInfos, actionPtrWithGoal);
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


void _findFirstActionForAGoalAndSetOfActions(PotentialNextAction& pCurrentResult,
                                             std::optional<PotentialNextActionComparisonCache>& pPotentialNextActionComparisonCacheOpt,
                                             std::set<ActionId>& pAlreadyDoneActions,
                                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                                             const FactToConditions::ConstMapOfFactIterator& pActions,
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
    if (pAlreadyDoneActions.count(currAction) > 0)
      continue;
    pAlreadyDoneActions.insert(currAction);

    auto itAction = domainActions.find(currAction);
    if (itAction != domainActions.end())
    {
      const Action& action = itAction->second;
      if (!action.canThisActionBeUsedByThePlanner)
        continue;
      FactsAlreadyChecked factsAlreadyChecked;
      auto newPotRes = PotentialNextAction(currAction, action);
      auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currAction);
      if (newTreePtr != nullptr && // To skip leaf of already seen path
          _lookForAPossibleEffect(newPotRes.satisfyObjective, newPotRes.parameters, pTryToDoMoreOptimalSolution, *newTreePtr,
                                  action.effect.worldStateModification, action.effect.potentialWorldStateModification,
                                  pGoal, pProblem, pFactOptionalToSatisfy,
                                  pDomain, factsAlreadyChecked, currAction) &&
          (!action.precondition || action.precondition->isTrue(pProblem.worldState, {}, {}, &newPotRes.parameters)))
      {
        while (true)
        {
          if (_isMoreOptimalNextAction(pPotentialNextActionComparisonCacheOpt, newPotRes, newPotNextAction, pProblem, pDomain, pTryToDoMoreOptimalSolution, pLength, pGoal, pGlobalHistorical))
          {
            assert(newPotRes.actionPtr != nullptr);
            newPotNextAction = newPotRes;
          }
          if (!newPotRes.removeAPossibility())
            break;
        }
      }
    }
  }

  if (_isMoreOptimalNextAction(pPotentialNextActionComparisonCacheOpt, newPotNextAction, pCurrentResult, pProblem, pDomain, pTryToDoMoreOptimalSolution, pLength, pGoal, pGlobalHistorical))
  {
    assert(newPotNextAction.actionPtr != nullptr);
    pCurrentResult = newPotNextAction;
  }
}


ActionId _findFirstActionForAGoal(
    std::map<Parameter, std::set<Entity>>& pParameters,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const Goal& pGoal,
    const Problem& pProblem,
    const FactOptional& pFactOptionalToSatisfy,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    std::size_t pLength,
    const Historical* pGlobalHistorical,
    const ActionPtrWithGoal* pPreviousActionPtr)
{
  PotentialNextAction res;
  std::set<ActionId> alreadyDoneActions;
  if (pPreviousActionPtr != nullptr &&
      pPreviousActionPtr->goal.objective() == pGoal.objective() &&
      pPreviousActionPtr->actionPtr != nullptr)
    alreadyDoneActions = pPreviousActionPtr->actionPtr->actionsSuccessionsWithoutInterestCache;
  std::optional<PotentialNextActionComparisonCache> potentialNextActionComparisonCacheOpt;
  for (const auto& currFact : pProblem.worldState.facts())
  {
    auto itPrecToActions = pDomain.preconditionToActions().find(currFact.first);
    _findFirstActionForAGoalAndSetOfActions(res, potentialNextActionComparisonCacheOpt, alreadyDoneActions,
                                            pTreeOfAlreadyDonePath,
                                            itPrecToActions, pGoal,
                                            pProblem, pFactOptionalToSatisfy,
                                            pDomain, pTryToDoMoreOptimalSolution,
                                            pLength, pGlobalHistorical);
  }

  auto actionWithoutPrecondition = pDomain.actionsWithoutFactToAddInPrecondition().valuesWithoutFact();
  _findFirstActionForAGoalAndSetOfActions(res, potentialNextActionComparisonCacheOpt, alreadyDoneActions,
                                          pTreeOfAlreadyDonePath,
                                          actionWithoutPrecondition, pGoal,
                                          pProblem, pFactOptionalToSatisfy,
                                          pDomain, pTryToDoMoreOptimalSolution,
                                          pLength, pGlobalHistorical);
  pParameters = std::move(res.parameters);
  return res.actionId;
}

const FactOptional* _getGoalToStatisfy(const Goal& pGoal,
                                       const Problem& pProblem)
{
  const auto& setOfFacts = pProblem.worldState.factsMapping();
  const FactOptional* res = nullptr;
  pGoal.objective().untilFalse(
        [&](const FactOptional& pFactOptional)
  {
    if (!pProblem.worldState.isOptionalFactSatisfied(pFactOptional))
    {
      res = &pFactOptional;
      return false;
    }
    return true;
  }, setOfFacts);
  return res;
}


bool _goalToPlanRec(
    std::list<ActionInvocationWithGoal>& pActionInvocations,
    Problem& pProblem,
    std::map<std::string, std::size_t>& pActionAlreadyInPlan,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    const Historical* pGlobalHistorical,
    const Goal& pGoal,
    int pPriority,
    const ActionPtrWithGoal* pPreviousActionPtr)
{
  pProblem.worldState.refreshCacheIfNeeded(pDomain);
  TreeOfAlreadyDonePath treeOfAlreadyDonePath;

  std::unique_ptr<ActionInvocationWithGoal> potentialRes;
  const FactOptional* factOptionalToSatisfyPtr = _getGoalToStatisfy(pGoal, pProblem);
  if (factOptionalToSatisfyPtr != nullptr)
  {
    std::map<Parameter, std::set<Entity>> parameters;
    auto actionId =
        _findFirstActionForAGoal(parameters, treeOfAlreadyDonePath,
                                 pGoal, pProblem, *factOptionalToSatisfyPtr,
                                 pDomain, pTryToDoMoreOptimalSolution, 0, pGlobalHistorical, pPreviousActionPtr);
    if (!actionId.empty())
      potentialRes = std::make_unique<ActionInvocationWithGoal>(actionId, parameters, pGoal.clone(), pPriority);
  }

  if (potentialRes && potentialRes->fromGoal)
  {
    const auto& actionToDoStr = potentialRes->actionInvocation.toStr();
    auto itAlreadyFoundAction = pActionAlreadyInPlan.find(actionToDoStr);
    if (itAlreadyFoundAction == pActionAlreadyInPlan.end())
    {
      pActionAlreadyInPlan[actionToDoStr] = 1;
    }
    else
    {
      if (itAlreadyFoundAction->second > 1)
        return false;
      ++itAlreadyFoundAction->second;
    }

    auto problemForPlanCost = pProblem;
    bool goalChanged = false;

    auto* potActionPtr = pDomain.getActionPtr(potentialRes->actionInvocation.actionId);
    if (potActionPtr != nullptr)
    {
      updateProblemForNextPotentialPlannerResultWithAction(problemForPlanCost, goalChanged,
                                                           *potentialRes, *potActionPtr,
                                                           pDomain, pNow, nullptr, nullptr);
      ActionPtrWithGoal previousAction(potActionPtr, pGoal);
      if (problemForPlanCost.worldState.isGoalSatisfied(pGoal) ||
          _goalToPlanRec(pActionInvocations, problemForPlanCost, pActionAlreadyInPlan,
                         pDomain, pTryToDoMoreOptimalSolution, pNow, nullptr, pGoal, pPriority, &previousAction))
      {
        potentialRes->fromGoal->notifyActivity();
        pActionInvocations.emplace_front(std::move(*potentialRes));
        return true;
      }
    }
  }
  else
  {
    return false; // Fail to find an next action to do
  }
  return false;
}

std::list<ActionInvocationWithGoal> _planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr,
                                                                     const ActionPtrWithGoal* pPreviousActionPtr)
{
  std::list<ActionInvocationWithGoal> res;
  pProblem.goalStack.iterateOnGoalsAndRemoveNonPersistent(
        [&](const Goal& pGoal, int pPriority){
            std::map<std::string, std::size_t> actionAlreadyInPlan;
            return _goalToPlanRec(res, pProblem, actionAlreadyInPlan,
                                  pDomain, pTryToDoMoreOptimalSolution, pNow, pGlobalHistorical, pGoal, pPriority,
                                  pPreviousActionPtr);
          },
        pProblem.worldState, pNow,
        pLookForAnActionOutputInfosPtr);
  return res;
}

}


std::list<ActionInvocationWithGoal> planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  return _planForMoreImportantGoalPossible(pProblem, pDomain, pTryToDoMoreOptimalSolution, pNow,
                                           pGlobalHistorical, pLookForAnActionOutputInfosPtr, nullptr);
}


std::list<ActionInvocationWithGoal> actionsToDoInParallelNow(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical)
{
  std::list<Goal> goalsDone;
  auto problemForPlanResolution = pProblem;
  auto sequentialPlan = planForEveryGoals(problemForPlanResolution, pDomain, pNow, pGlobalHistorical, &goalsDone);
  auto parallelPlan = toParallelPlan(sequentialPlan, true, pProblem, pDomain, goalsDone, pNow);
  if (!parallelPlan.empty())
    return parallelPlan.front();
  return {};
}


void notifyActionStarted(Problem& pProblem,
                         const Domain& pDomain,
                         const ActionInvocationWithGoal& pActionInvocationWithGoal,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const auto& actions = pDomain.actions();
  auto itAction = actions.find(pActionInvocationWithGoal.actionInvocation.actionId);
  if (itAction != actions.end())
  {
    if (itAction->second.effect.worldStateModificationAtStart)
    {
      auto worldStateModificationAtStart = itAction->second.effect.worldStateModificationAtStart->cloneParamSet(pActionInvocationWithGoal.actionInvocation.parameters);
      auto& setOfEvents = pDomain.getSetOfEvents();
      const auto& ontology = pDomain.getOntology();
      pProblem.worldState.modify(worldStateModificationAtStart, pProblem.goalStack, setOfEvents,
                                 ontology, pProblem.entities, pNow);
    }
  }
}


void notifyActionDone(Problem& pProblem,
                      const Domain& pDomain,
                      const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const auto& actions = pDomain.actions();
  auto itAction = actions.find(pOnStepOfPlannerResult.actionInvocation.actionId);
  if (itAction != actions.end())
  {
    auto& setOfEvents = pDomain.getSetOfEvents();
    bool goalChanged = false;
    const auto& ontology = pDomain.getOntology();
    notifyActionInvocationDone(pProblem, goalChanged, setOfEvents, pOnStepOfPlannerResult, itAction->second.effect.worldStateModification,
                               ontology, pNow,
                               &itAction->second.effect.goalsToAdd, &itAction->second.effect.goalsToAddInCurrentPriority,
                               nullptr);
  }
}



std::list<ActionInvocationWithGoal> planForEveryGoals(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    std::list<Goal>* pGoalsDonePtr)
{
  const bool tryToDoMoreOptimalSolution = true;
  std::map<std::string, std::size_t> actionAlreadyInPlan;
  std::list<ActionInvocationWithGoal> res;
  LookForAnActionOutputInfos lookForAnActionOutputInfos;
  while (!pProblem.goalStack.goals().empty())
  {
    auto subPlan = _planForMoreImportantGoalPossible(pProblem, pDomain, tryToDoMoreOptimalSolution,
                                                     pNow, pGlobalHistorical, &lookForAnActionOutputInfos, nullptr);
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
      updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical,
                                                 &lookForAnActionOutputInfos);
      res.emplace_back(std::move(currActionInSubPlan));
      if (goalChanged)
        break;
    }
  }
  if (pGoalsDonePtr != nullptr)
    lookForAnActionOutputInfos.moveGoalsDone(*pGoalsDonePtr);
  return res;
}



std::string planToStr(const std::list<cp::ActionInvocationWithGoal>& pPlan,
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


std::string goalsToStr(const std::list<cp::Goal>& pGoals,
                       const std::string& pSep)
{
  auto size = pGoals.size();
  if (size == 1)
    return pGoals.front().toStr();
  std::string res;
  bool firstIteration = true;
  for (const auto& currGoal : pGoals)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += pSep;
    res += currGoal.toStr();
  }
  return res;
}



} // !cp
