#ifndef INCLUDE_PRIORITIZEDGOALSPLANNER_TYPES_DOMAIN_HPP
#define INCLUDE_PRIORITIZEDGOALSPLANNER_TYPES_DOMAIN_HPP

#include <map>
#include <set>
#include "../util/api.hpp"
#include <prioritizedgoalsplanner/util/alias.hpp>
#include <prioritizedgoalsplanner/types/action.hpp>
#include <prioritizedgoalsplanner/types/condtionstovalue.hpp>
#include <prioritizedgoalsplanner/types/ontology.hpp>
#include <prioritizedgoalsplanner/types/setofevents.hpp>
#include <prioritizedgoalsplanner/types/setofconstfacts.hpp>

namespace cp
{

/// Set of all the actions that the bot can do with accessors to optimize the search of a action.
struct PRIORITIZEDGOALSPLANNER_API Domain
{
  /// Construct an empty domain.
  Domain();

  // Do not permit to copy an action
  Domain(const Domain& pOther) = default;
  Domain& operator=(const Domain& pOther) = default;

  /**
   * @brief Construct a domain.
   * @param[in] pActions Map of action identifiers to action.
   */
  Domain(const std::map<ActionId, Action>& pActions,
         const Ontology& pOntology,
         const SetOfEvents& pSetOfEvents = {},
         const std::map<SetOfEventsId, SetOfEvents>& pIdToSetOfEvents = {},
         const SetOfConstFacts& pTimelessFacts = {},
         const std::string& pName = "domain");


  // Actions
  // -------

  /**
   * @brief Add an action.
   * @param pActionId[in] Identifier of the action to add.
   * @param pAction[in] Action to add.
   *
   * If the identifier is already used, the addition will not be done.
   */
  void addAction(const ActionId& pActionId,
                 const Action& pAction);

  /**
   * @brief Remove an action.
   * @param pActionId[in] Identifier of the action to remove.
   *
   * If the action is not found, this function will have no effect.<br/>
   * No exception will be raised.
   */
  void removeAction(const ActionId& pActionId);

  const Action* getActionPtr(const ActionId& pActionId) const;

  /// All action identifiers to action.
  const std::map<ActionId, Action>& actions() const { return _actions; }
  /// All facts in precondition to action identifiers.
  const FactsToValue& preconditionToActions() const { return _conditionsToActions.factsToValue(); }
  /// All negationed facts in precondition to action identifiers.
  const FactsToValue& notPreconditionToActions() const { return _conditionsToActions.notFactsToValue(); }
  /// All action identifiers of the actions without precondtion.
  const FactsToValue& actionsWithoutFactToAddInPrecondition() const { return _actionsWithoutFactToAddInPrecondition; }



  // Events
  // ------

  /**
   * @brief Add a set of events.
   * @param pSetOfEvents Set of events to add.
   * @param pSetOfEventsId Identifier of the set of events to add.
   *
   * If the identifier is already used, the addition will not be done.
   */
  SetOfEventsId addSetOfEvents(const SetOfEvents& pSetOfEvents,
                               const SetOfEventsId& pSetOfEventsId = "soe");

  /**
   * @brief Remove a set of events.
   * @param pSetOfEventsId Identifier of the set of events to remove.
   *
   * If the event is not found, this function will have no effect.
   * No exception will be raised.
   */
  void removeSetOfEvents(const SetOfEventsId& pSetOfEventsId);

  /// Remove all the events.
  void clearEvents();

  /// Get the set of events.
  const std::map<SetOfEventsId, SetOfEvents>& getSetOfEvents() const { return _setOfEvents; }

  /// Get the universal unique identifier regenerated each time this object is modified.
  const std::string& getUuid() const { return _uuid; }

  const std::string& getName() const { return _name; }

  static const SetOfEventsId setOfEventsIdFromConstructor;

  const Ontology& getOntology() const { return _ontology; }

  const std::map<ActionId, Action>& getActions() const { return _actions; }

  const SetOfConstFacts& getTimelessFacts() const { return _timelessFacts; }

  std::string printSuccessionCache() const;

  void addRequirement(const std::string& pRequirement);

  const std::set<std::string>& requirements() const { return _requirements; }

private:
  /// Universal unique identifier regenerated each time this object is modified.
  std::string _uuid;
  std::string _name;
  Ontology _ontology;
  SetOfConstFacts _timelessFacts;
  /// Map of action identifiers to action.
  std::map<ActionId, Action> _actions;
  /// Conditions to action identifiers.
  ConditionsToValue _conditionsToActions;
  /// Set of action identifiers of the actions without precondtion.
  FactsToValue _actionsWithoutFactToAddInPrecondition;
  /// Map set of events identifiers to the set of events.
  std::map<SetOfEventsId, SetOfEvents> _setOfEvents;
  std::set<std::string> _requirements;

  void _addAction(const ActionId& pActionId,
                  const Action& pAction);

  void _updateSuccessions();
};

} // !cp


#endif // INCLUDE_PRIORITIZEDGOALSPLANNER_TYPES_DOMAIN_HPP
