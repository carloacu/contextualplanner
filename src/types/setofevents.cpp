#include <contextualplanner/types/setofevents.hpp>
#include <contextualplanner/util/util.hpp>

namespace cp
{

SetOfEvents::SetOfEvents(const Event& pEvent)
  : _events(),
    _reachableEventLinks()
{
  add(pEvent);
}


EventId SetOfEvents::add(const Event& pEvent,
                         const EventId& pEventId)
{
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _events.count(pId) == 0;
  };
  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pEventId, isIdOkForInsertion);

  _events.emplace(newId, pEvent);

  if (pEvent.precondition)
  {
    pEvent.precondition->forAll(
          [&](const FactOptional& pFactOptional,
          bool pIgnoreFluent)
    {
      if (pFactOptional.isFactNegated)
        _reachableEventLinks.notConditionToEvents.add(pFactOptional.fact, newId, pIgnoreFluent);
      else
        _reachableEventLinks.conditionToEvents.add(pFactOptional.fact, newId, pIgnoreFluent);
    }
    );
  }
  return newId;
}


void SetOfEvents::remove(const EventId& pEventId)
{
  auto it = _events.find(pEventId);
  if (it == _events.end())
    return;
  auto& eventThatWillBeRemoved = it->second;

  if (eventThatWillBeRemoved.precondition)
  {
    _reachableEventLinks.notConditionToEvents.erase(pEventId);
    _reachableEventLinks.conditionToEvents.erase(pEventId);
  }
  _events.erase(it);
}


} // !cp
