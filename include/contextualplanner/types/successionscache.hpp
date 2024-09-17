#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_SUCCESSIONSCACHE_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_SUCCESSIONSCACHE_HPP

#include <map>
#include <memory>
#include <set>
#include "../util/api.hpp"
#include <contextualplanner/util/alias.hpp>


namespace cp
{
struct Action;
struct Condition;
struct Domain;
struct FactOptional;
struct WorldStateModification;


struct CONTEXTUALPLANNER_API SuccessionsForAFact
{

  std::set<ActionId> actions;

  std::map<SetOfInferencesId, std::set<InferenceId>> inferences;
};


struct CONTEXTUALPLANNER_API SuccessionsCache
{
  void clear();
  void update(const Domain& pDomain,
              const ActionId& pActionIdToExclude,
              const SetOfInferencesId& pSetOfInferencesIdToExclude,
              const InferenceId& pInferenceIdToExclude,
              const cp::Condition& pPrecondition,
              const std::unique_ptr<cp::WorldStateModification>& pWorldModifPtr1,
              const std::unique_ptr<cp::WorldStateModification>& pWorldModifPtr2);

  std::string print() const;

  std::map<FactOptional, SuccessionsForAFact> factsToSuccessions;

private:
  void _updateForAWsm(const Domain& pDomain,
                      const ActionId& pActionIdToExclude,
                      const SetOfInferencesId& pSetOfInferencesIdToExclude,
                      const InferenceId& pInferenceIdToExclude,
                      const std::set<FactOptional>& pOptionalFactsToIgnore,
                      const cp::WorldStateModification& pWorldModif);
};


} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_SUCCESSIONSCACHE_HPP
