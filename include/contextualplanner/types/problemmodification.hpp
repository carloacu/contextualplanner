#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_PROBLEMMODIFICATION_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_PROBLEMMODIFICATION_HPP

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "../util/api.hpp"
#include <contextualplanner/types/worldstatemodification.hpp>
#include <contextualplanner/types/goal.hpp>


namespace cp
{


// Specification of a problem modification.
struct CONTEXTUALPLANNER_API ProblemModification
{
  /// Construct a problem modification without arguments.
  ProblemModification()
    : worldStateModification(),
      potentialWorldStateModification(),
      worldStateModificationAtStart(),
      goalsToAdd(),
      goalsToAddInCurrentPriority()
  {
  }

  /**
   * @brief Construct a problem modification from some world state modifications.
   * @param[in] pWorldStateModification Modification of the world used both for planification and to actually apply when wanted.
   * @param[in] pPotentialWorldStateModification Modification of the world used only for planification.<br/>
   * We supposed it will be potentially applied inderectly by extractors.
   */
  ProblemModification(std::unique_ptr<cp::WorldStateModification> pWorldStateModification,
                      std::unique_ptr<cp::WorldStateModification> pPotentialWorldStateModification = std::unique_ptr<cp::WorldStateModification>())
    : worldStateModification(pWorldStateModification ? std::move(pWorldStateModification): std::unique_ptr<cp::WorldStateModification>()),
      potentialWorldStateModification(pPotentialWorldStateModification ? std::move(pPotentialWorldStateModification) : std::unique_ptr<cp::WorldStateModification>()),
      worldStateModificationAtStart(),
      goalsToAdd(),
      goalsToAddInCurrentPriority()
  {
  }

  /// Copy constructor.
  ProblemModification(const ProblemModification& pOther)
    : worldStateModification(pOther.worldStateModification ? pOther.worldStateModification->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>()),
      potentialWorldStateModification(pOther.potentialWorldStateModification ? pOther.potentialWorldStateModification->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>()),
      worldStateModificationAtStart(pOther.worldStateModificationAtStart ? pOther.worldStateModificationAtStart->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>()),
      goalsToAdd(pOther.goalsToAdd),
      goalsToAddInCurrentPriority(pOther.goalsToAddInCurrentPriority)
  {
  }

  /// Copy operator.
  void operator=(const ProblemModification& pOther)
  {
    worldStateModification = pOther.worldStateModification ? pOther.worldStateModification->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>();
    potentialWorldStateModification = pOther.potentialWorldStateModification ? pOther.potentialWorldStateModification->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>();
    worldStateModificationAtStart = pOther.worldStateModificationAtStart ? pOther.worldStateModificationAtStart->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>();
    goalsToAdd = pOther.goalsToAdd;
    goalsToAddInCurrentPriority = pOther.goalsToAddInCurrentPriority;
  }

  /// Is the problem modification empty.
  bool empty() const { return !worldStateModification && !potentialWorldStateModification && !worldStateModificationAtStart && goalsToAdd.empty() && goalsToAddInCurrentPriority.empty(); }

  /// Check equality with another problem modification.
  bool operator==(const ProblemModification& pOther) const;
  bool operator!=(const ProblemModification& pOther) const { return !operator==(pOther); }

  /// Check if this object contains a fact or the negation of the fact.
  bool hasFact(const cp::Fact& pFact) const;

  /**
   * @brief Add the content of another problem modifiation.
   * @param[in] pOther The other problem modification to add.
   */
  void add(const ProblemModification& pOther);

  /**
   * @brief Replace a fact by another inside this object.
   * @param[in] pOldFact Fact to replace.
   * @param[in] pNewFact New fact to set.
   */
  void replaceArgument(const Entity& pOldFact,
                       const Entity& pNewFact);

  std::set<FactOptional> getAllOptFactsThatCanBeModified() const;

  /// Convert the worldStateModification to a string or to an empty string if it is not defined.
  std::string worldStateModification_str() const { return worldStateModification ? worldStateModification->toStr() : ""; }
  /// Convert the potentialWorldStateModification to a string or to an empty string if it is not defined.
  std::string potentialWorldStateModification_str() const { return potentialWorldStateModification ? potentialWorldStateModification->toStr() : ""; }

  /// World modifications declared and that will be applied to the world.
  std::unique_ptr<cp::WorldStateModification> worldStateModification;
  /// World modifications declared but that will not be applied to the world.
  std::unique_ptr<cp::WorldStateModification> potentialWorldStateModification;
  /// World modifications declared and that will be applied to the world when the action starts.
  std::unique_ptr<cp::WorldStateModification> worldStateModificationAtStart;
  /// Goal priorities to goals to add in the goal stack.
  std::map<int, std::vector<cp::Goal>> goalsToAdd;
  /// Goals to add in the goal stack in current priority of the goal stack.
  std::vector<cp::Goal> goalsToAddInCurrentPriority;
};




// Implemenation

inline bool ProblemModification::hasFact(const cp::Fact& pFact) const
{
  if (worldStateModification && worldStateModification->hasFact(pFact) ||
      potentialWorldStateModification && potentialWorldStateModification->hasFact(pFact) ||
      worldStateModificationAtStart && worldStateModificationAtStart->hasFact(pFact))
    return true;
  for (const auto& currGoalWithPriority : goalsToAdd)
    for (const auto& currGoal : currGoalWithPriority.second)
      if (currGoal.objective().hasFact(pFact))
        return true;
  for (const auto& currGoal : goalsToAddInCurrentPriority)
    if (currGoal.objective().hasFact(pFact))
      return true;
  return false;
}

inline void ProblemModification::add(const ProblemModification& pOther)
{
  if (pOther.worldStateModification)
  {
    if (worldStateModification)
      worldStateModification = WorldStateModification::createByConcatenation(*worldStateModification, *pOther.worldStateModification);
    else
      worldStateModification = pOther.worldStateModification->clone(nullptr);
  }
  if (pOther.potentialWorldStateModification)
  {
    if (potentialWorldStateModification)
      potentialWorldStateModification = WorldStateModification::createByConcatenation(*potentialWorldStateModification, *pOther.potentialWorldStateModification);
    else
      potentialWorldStateModification = pOther.potentialWorldStateModification->clone(nullptr);
  }
  if (pOther.worldStateModificationAtStart)
  {
    if (worldStateModificationAtStart)
      worldStateModificationAtStart = WorldStateModification::createByConcatenation(*worldStateModificationAtStart, *pOther.worldStateModificationAtStart);
    else
      worldStateModificationAtStart = pOther.worldStateModificationAtStart->clone(nullptr);
  }

  goalsToAdd.insert(pOther.goalsToAdd.begin(), pOther.goalsToAdd.end());
  goalsToAddInCurrentPriority.insert(goalsToAddInCurrentPriority.end(), pOther.goalsToAddInCurrentPriority.begin(), pOther.goalsToAddInCurrentPriority.end());
}

inline void ProblemModification::replaceArgument(const Entity& pOld,
                                                 const Entity& pNew)
{
  if (worldStateModification)
    worldStateModification->replaceArgument(pOld, pNew);
  if (potentialWorldStateModification)
    potentialWorldStateModification->replaceArgument(pOld, pNew);
  if (worldStateModificationAtStart)
    worldStateModificationAtStart->replaceArgument(pOld, pNew);
}

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_PROBLEMMODIFICATION_HPP
