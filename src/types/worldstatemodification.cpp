#include <contextualplanner/types/worldstatemodification.hpp>
#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/worldstate.hpp>
#include "expressionParsed.hpp"
#include <contextualplanner/util/util.hpp>
#include "worldstatemodificationprivate.hpp"

namespace cp
{

void Successions::addSuccesionsOptFact(const FactOptional& pFactOptional,
                                       const Domain& pDomain,
                                       const WorldStateModificationContainerId& pContainerId,
                                       const std::set<FactOptional>& pOptionalFactsToIgnore)
{
  if ((pFactOptional.fact.fluent() && pFactOptional.fact.fluent()->isAnyValue()) ||
      pOptionalFactsToIgnore.count(pFactOptional) == 0)
  {
    auto& preconditionToActions = !pFactOptional.isFactNegated ? pDomain.preconditionToActions() : pDomain.notPreconditionToActions();
    auto actionsFromPreconditions = preconditionToActions.find(pFactOptional.fact);
    for (const auto& currActionId : actionsFromPreconditions)
      if (!pContainerId.isAction(currActionId))
        actions.insert(currActionId);

    auto& setOfEvents = pDomain.getSetOfEvents();
    for (auto& currSetOfEvents : setOfEvents)
    {
      std::set<EventId>* eventsPtr = nullptr;
      auto& conditionToReachableEvents = !pFactOptional.isFactNegated ?
            currSetOfEvents.second.reachableEventLinks().conditionToEvents :
            currSetOfEvents.second.reachableEventLinks().notConditionToEvents;

      auto eventsFromCondtion = conditionToReachableEvents.find(pFactOptional.fact);
      for (const auto& currEventId : eventsFromCondtion)
      {
        if (!pContainerId.isEvent(currSetOfEvents.first, currEventId))
        {
          if (eventsPtr == nullptr)
            eventsPtr = &events[currSetOfEvents.first];
          eventsPtr->insert(currEventId);
        }
      }
    }
  }
}

void Successions::print(std::string& pRes,
                        const FactOptional& pFactOptional) const
{
  if (empty())
    return;
  if (pRes != "")
    pRes += "\n";

  pRes += "fact: " + pFactOptional.toStr() + "\n";
  for (const auto& currActionId : actions)
    pRes += "action: " + currActionId + "\n";
  for (const auto& currEventSet : events)
    for (const auto& currEventId : currEventSet.second)
      pRes += "event: " + currEventSet.first + "|" + currEventId + "\n";
}


std::unique_ptr<WorldStateModification> WorldStateModification::createByConcatenation(const WorldStateModification& pWsModif1,
                                                                                      const WorldStateModification& pWsModif2)
{
  return std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND,
                                                      pWsModif1.clone(nullptr),
                                                      pWsModif2.clone(nullptr));
}


} // !cp
