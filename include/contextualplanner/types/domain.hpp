#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP

#include <map>
#include <set>
#include "../util/api.hpp"
#include <contextualplanner/util/alias.hpp>
#include <contextualplanner/types/action.hpp>
#include <contextualplanner/types/facttoconditions.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/setofinferences.hpp>

namespace cp
{

/// Set of all the actions that the bot can do with accessors to optimize the search of a action.
struct CONTEXTUALPLANNER_API Domain
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
         const SetOfInferences& pSetOfInferences = {});

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

  /// All action identifiers to action.
  const std::map<ActionId, Action>& actions() const { return _actions; }
  /// All facts in precondition to action identifiers.
  const FactToConditions& preconditionToActions() const { return _preconditionToActions; }
  /// All negationed facts in precondition to action identifiers.
  const FactToConditions& notPreconditionToActions() const { return _notPreconditionToActions; }
  /// All action identifiers of the actions without precondtion.
  const FactToConditions& actionsWithoutFactToAddInPrecondition() const { return _actionsWithoutFactToAddInPrecondition; }



  // Inferences
  // ----------

  /**
   * @brief Add a set of inferences.
   * @param pSetOfInferences Set of inferences to add.
   * @param pSetOfInferencesId Identifier of the set of inferences to add.
   *
   * If the identifier is already used, the addition will not be done.
   */
  SetOfInferencesId addSetOfInferences(const SetOfInferences& pSetOfInferences,
                                       const SetOfInferencesId& pSetOfInferencesId = "soi");

  /**
   * @brief Remove a set of inferences.
   * @param pSetOfInferencesId Identifier of the set of inferences to remove.
   *
   * If the inference is not found, this function will have no effect.
   * No exception will be raised.
   */
  void removeSetOfInferences(const SetOfInferencesId& pSetOfInferencesId);

  /// Remove all the inferences.
  void clearInferences();

  /// Get the set of inferences.
  const std::map<SetOfInferencesId, SetOfInferences>& getSetOfInferences() const { return _setOfInferences; }

  /// Get the universal unique identifier regenerated each time this object is modified.
  const std::string& getUuid() const { return _uuid; }

  static const SetOfInferencesId setOfInferencesIdFromConstructor;

  const Ontology& getOntology() const { return _ontology; }

private:
  /// Universal unique identifier regenerated each time this object is modified.
  std::string _uuid;
  Ontology _ontology;
  /// Map of action identifiers to action.
  std::map<ActionId, Action> _actions;
  /// Map of facts in precondition to action identifiers.
  FactToConditions _preconditionToActions;
  /// Map of negationed facts in precondition to action identifiers.
  FactToConditions _notPreconditionToActions;
  /// Set of action identifiers of the actions without precondtion.
  FactToConditions _actionsWithoutFactToAddInPrecondition;
  /// Map set of inferences identifiers to the set of inferences.
  std::map<SetOfInferencesId, SetOfInferences> _setOfInferences;
};

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP
