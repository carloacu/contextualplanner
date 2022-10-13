#include <contextualplanner/types/domain.hpp>


namespace cp
{

Domain::Domain(const std::map<ActionId, Action>& pActions)
{
  for (const auto& currAction : pActions)
    addAction(currAction.first, currAction.second);
}


void Domain::addAction(const ActionId& pActionId,
                       const Action& pAction)
{
  if ((pAction.effect.factsModifications.isIncludedIn(pAction.preconditions) &&
       pAction.effect.potentialFactsModifications.isIncludedIn(pAction.preconditions)) ||
      pAction.effect.empty() ||
      _actions.count(pActionId) > 0)
    return;
  _actions.emplace(pActionId, pAction);
  for (const auto& currPrecondition : pAction.preconditions.facts)
    _preconditionToActions[currPrecondition.name].insert(pActionId);
  for (const auto& currPrecondition : pAction.preconditions.notFacts)
    _notPreconditionToActions[currPrecondition.name].insert(pActionId);

  for (auto& currExp : pAction.preconditions.exps)
    for (auto& currElt : currExp.elts)
      if (currElt.type == ExpressionElementType::FACT)
        _preconditionToActionsExps[currElt.value].insert(pActionId);
  if (pAction.preconditions.facts.empty() && pAction.preconditions.exps.empty())
    _actionsWithoutFactToAddInPrecondition.insert(pActionId);
}


void Domain::removeAction(const ActionId& pActionId)
{
  auto it = _actions.find(pActionId);
  if (it == _actions.end())
    return;
  auto& actionThatWillBeRemoved = it->second;
  for (const auto& currPrecondition : actionThatWillBeRemoved.preconditions.facts)
    _preconditionToActions[currPrecondition.name].erase(pActionId);
  for (const auto& currPrecondition : actionThatWillBeRemoved.preconditions.notFacts)
    _notPreconditionToActions[currPrecondition.name].erase(pActionId);

  for (auto& currExp : actionThatWillBeRemoved.preconditions.exps)
    for (auto& currElt : currExp.elts)
      if (currElt.type == ExpressionElementType::FACT)
        _preconditionToActionsExps[currElt.value].erase(pActionId);
  if (actionThatWillBeRemoved.preconditions.facts.empty() && actionThatWillBeRemoved.preconditions.exps.empty())
    _actionsWithoutFactToAddInPrecondition.erase(pActionId);
  _actions.erase(it);
}


} // !cp
