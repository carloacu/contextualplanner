#include <contextualplanner/types/action.hpp>
#include <contextualplanner/util/util.hpp>

namespace cp
{


bool Action::operator==(const Action& pOther) const
{
  return parameters == pOther.parameters &&
      areUPtrEqual(precondition, pOther.precondition) &&
      areUPtrEqual(preferInContext, pOther.preferInContext) &&
      effect == pOther.effect &&
      highImportanceOfNotRepeatingIt == pOther.highImportanceOfNotRepeatingIt;
}


bool Action::hasFact(const cp::Fact& pFact) const
{
  return (precondition && precondition->hasFact(pFact)) ||
      (preferInContext && preferInContext->hasFact(pFact)) ||
      effect.hasFact(pFact);
}

void Action::replaceArgument(const Entity& pOld,
                             const Entity& pNew)
{
  effect.replaceArgument(pOld, pNew);
}


void Action::updateSuccessionCache(const Domain& pDomain,
                                   const ActionId& pIdOfThisAction)
{
  WorldStateModificationContainerId containerId;
  containerId.actionIdToExclude.emplace(pIdOfThisAction);

  auto optionalFactsToIgnore = precondition ? precondition->getFactToIgnoreInCorrespondingEffect() : std::set<FactOptional>();
  if (effect.worldStateModification)
    effect.worldStateModification->updateSuccesions(pDomain, containerId, optionalFactsToIgnore);
  if (effect.potentialWorldStateModification)
    effect.potentialWorldStateModification->updateSuccesions(pDomain, containerId, optionalFactsToIgnore);
}

std::string Action::printSuccessionCache() const
{
  std::string res;
  if (effect.worldStateModification)
    effect.worldStateModification->printSuccesions(res);
  if (effect.potentialWorldStateModification)
    effect.potentialWorldStateModification->printSuccesions(res);
  return res;
}

void Action::throwIfNotValid(const WorldState& pWorldState)
{
  _throwIfNotValidForACondition(precondition);
  _throwIfNotValidForACondition(preferInContext);
  _throwIfNotValidForAnWordStateModif(effect.worldStateModification, pWorldState);
  _throwIfNotValidForAnWordStateModif(effect.potentialWorldStateModification, pWorldState);
  _throwIfNotValidForAnWordStateModif(effect.worldStateModificationAtStart, pWorldState);
}


void Action::_throwIfNotValidForACondition(const std::unique_ptr<Condition>& pPrecondition)
{
  if (pPrecondition)
    pPrecondition->forAll([&](const FactOptional& pFactOptional, bool) {
      _throwIfNotValidForAFact(pFactOptional.fact);
    });
}


void Action::_throwIfNotValidForAnWordStateModif(const std::unique_ptr<WorldStateModification>& pWs,
                                                 const WorldState& pWorldState)
{
  if (pWs)
    pWs->forAll([&](const FactOptional& pFactOptional) {
      _throwIfNotValidForAFact(pFactOptional.fact);
    }, pWorldState);
}


void Action::_throwIfNotValidForAFact(const Fact& pFact)
{
  for (auto& currArgument : pFact.arguments())
    if (currArgument.isAParameterToFill() && !currArgument.isValidParameterAccordingToPossiblities(parameters))
      throw std::runtime_error("\"" + currArgument.value + "\" is missing in action parameters");

  if (pFact.fluent() && pFact.fluent()->isAParameterToFill() && !pFact.fluent()->isValidParameterAccordingToPossiblities(parameters))
    throw std::runtime_error("\"" + pFact.fluent()->value + "\" fluent is missing in action parameters");
}



} // !cp
