#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/worldstate.hpp>
#include <contextualplanner/util/util.hpp>
#include "../util/uuid.hpp"

namespace cp
{
const SetOfInferencesId Domain::setOfInferencesIdFromConstructor = "soi_from_constructor";

namespace
{
static const WorldState _emptyWorldState;
static const std::map<Parameter, std::set<Entity>> _emptyParametersWithValues;
static const std::vector<Parameter> _emptyParameters;

/**
 * @brief Check if a world state modification can do some modification if we assume the world already satisfies a condition.
 * @param[in] pWorldStateModification World state modification to check.
 * @param[in] pSatisfiedConditionPtr Condition that is already satisfied.
 * @return True if the world state modification can do some modification in the world.
 */
bool _canWmDoSomething(const std::unique_ptr<cp::WorldStateModification>& pWorldStateModification,
                       const std::unique_ptr<Condition>& pSatisfiedConditionPtr)
{
  if (!pWorldStateModification)
    return false;
  if (!pWorldStateModification->isOnlyASetOfFacts())
    return true;

  if (pWorldStateModification->forAllUntilTrue(
        [&](const FactOptional& pFactOptional)
  {
      return !pSatisfiedConditionPtr ||
        !pSatisfiedConditionPtr->containsFactOpt(pFactOptional,
                                                 _emptyParametersWithValues, nullptr,
                                                 _emptyParameters);
}, _emptyWorldState))
  {
    return true;
  }

  return false;
}

}

Domain::Domain()
  : _uuid(),
    _ontology(),
    _actions(),
    _preconditionToActions(),
    _notPreconditionToActions(),
    _actionsWithoutFactToAddInPrecondition(),
    _setOfInferences()
{
}


Domain::Domain(const std::map<ActionId, Action>& pActions,
               const Ontology& pOntology,
               const SetOfInferences& pSetOfInferences)
  : _uuid(generateUuid()),
    _ontology(pOntology),
    _actions(),
    _preconditionToActions(),
    _notPreconditionToActions(),
    _actionsWithoutFactToAddInPrecondition(),
    _setOfInferences()
{
  for (const auto& currAction : pActions)
    addAction(currAction.first, currAction.second);

  if (!pSetOfInferences.empty())
    _setOfInferences.emplace(setOfInferencesIdFromConstructor, pSetOfInferences);

  for (auto& currAction: _actions)
    currAction.second.updateSuccessionCache(*this, currAction.first);
}



void Domain::addAction(const ActionId& pActionId,
                       const Action& pAction)
{
  if (_actions.count(pActionId) > 0 ||
      pAction.effect.empty())
    return;
  _actions.emplace(pActionId, pAction);

  if (!_canWmDoSomething(pAction.effect.worldStateModification, pAction.precondition) &&
      !_canWmDoSomething(pAction.effect.potentialWorldStateModification, pAction.precondition))
    return;

  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object

  bool hasAddedAFact = false;
  if (pAction.precondition)
  {
    pAction.precondition->forAll(
          [&](const FactOptional& pFactOptional,
              bool pIgnoreFluent)
    {
      if (pFactOptional.isFactNegated)
      {
        _notPreconditionToActions.add(pFactOptional.fact, pActionId, pIgnoreFluent);
      }
      else
      {
        _preconditionToActions.add(pFactOptional.fact, pActionId, pIgnoreFluent);
        hasAddedAFact = true;
      }
    }
    );
  }

  if (!hasAddedAFact)
    _actionsWithoutFactToAddInPrecondition.addValueWithoutFact(pActionId);
}


void Domain::removeAction(const ActionId& pActionId)
{
  auto it = _actions.find(pActionId);
  if (it == _actions.end())
    return;
  auto& actionThatWillBeRemoved = it->second;
  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object

  if (actionThatWillBeRemoved.precondition)
  {
    _notPreconditionToActions.erase(pActionId);
    _preconditionToActions.erase(pActionId);
  }
  else
  {
    _actionsWithoutFactToAddInPrecondition.erase(pActionId);
  }

  _actions.erase(it);
}



SetOfInferencesId Domain::addSetOfInferences(const SetOfInferences& pSetOfInferences,
                                             const SetOfInferencesId& pSetOfInferencesId)
{
  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _setOfInferences.count(pId) == 0;
  };

  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pSetOfInferencesId, isIdOkForInsertion);
  _setOfInferences.emplace(newId, pSetOfInferences);
  return newId;
}


void Domain::removeSetOfInferences(const SetOfInferencesId& pSetOfInferencesId)
{
  auto it = _setOfInferences.find(pSetOfInferencesId);
  if (it != _setOfInferences.end())
  {
    _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
    _setOfInferences.erase(it);
  }
}

void Domain::clearInferences()
{
  if (!_setOfInferences.empty())
  {
    _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
    _setOfInferences.clear();
  }
}



} // !cp
