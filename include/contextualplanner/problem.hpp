#ifndef INCLUDE_CONTEXTUALPLANNER_PROBLEM_HPP
#define INCLUDE_CONTEXTUALPLANNER_PROBLEM_HPP

#include <set>
#include "api.hpp"
#include <contextualplanner/historical.hpp>
#include <contextualplanner/observableunsafe.hpp>
#include <contextualplanner/fact.hpp>
#include <contextualplanner/goal.hpp>
#include <contextualplanner/setoffacts.hpp>


namespace cp
{

struct CONTEXTUALPLANNER_API Problem
{
  Problem() = default;
  Problem(const Problem& pOther);
  Historical historical{};
  cpstd::observable::ObservableUnsafe<void (const std::map<std::string, std::string>&)> onVariablesToValueChanged{};
  cpstd::observable::ObservableUnsafe<void (const std::set<Fact>&)> onFactsChanged{};
  cpstd::observable::ObservableUnsafe<void (const std::map<int, std::vector<Goal>>&)> onGoalsChanged{};

  std::string getCurrentGoal() const;
  void addVariablesToValue(const std::map<std::string, std::string>& pVariablesToValue);
  bool addFact(const Fact& pFact);
  template<typename FACTS>
  bool addFacts(const FACTS& pFacts);

  bool hasFact(const Fact& pFact) const;

  bool removeFact(const Fact& pFact);
  template<typename FACTS>
  bool removeFacts(const FACTS& pFacts);

  bool modifyFacts(const SetOfFacts& pSetOfFacts);
  void setFacts(const std::set<Fact>& pFacts);
  void addReachableFacts(const std::set<Fact>& pFacts);
  void addReachableFactsWithAnyValues(const std::vector<Fact>& pFacts);
  void addRemovableFacts(const std::set<Fact>& pFacts);
  void noNeedToAddReachableFacts() { _needToAddReachableFacts = false; }
  bool needToAddReachableFacts() const { return _needToAddReachableFacts; }
  void iterateOnGoalAndRemoveNonPersistent(const std::function<bool (Goal&, int)>& pManageGoal,
                                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  static const int defaultPriority;
  void setGoals(const std::map<int, std::vector<Goal>>& pGoals,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  void setGoalsForAPriority(const std::vector<Goal>& pGoals,
                            const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                            int pPriority = defaultPriority);
  void addGoals(const std::map<int, std::vector<Goal>>& pGoals,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  void pushFrontGoal(const Goal& pGoal,
                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                     int pPriority = defaultPriority);
  void pushBackGoal(const Goal& pGoal,
                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                    int pPriority = defaultPriority);
  void setGoalPriority(const std::string& pGoalStr,
                       int pPriority,
                       bool pPushFrontOrBttomInCaseOfConflictWithAnotherGoal);
  void removeGoals(const std::string& pGoalGroupId,
                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  ActionId removeFirstGoalsThatAreAlreadySatisfied();
  const std::map<int, std::vector<Goal>>& goals() const { return _goals; }

  void notifyActionDone(const std::string& pActionId,
                        const std::map<std::string, std::string>& pParameters,
                        const SetOfFacts& pEffect,
                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                        const std::map<int, std::vector<Goal>>* pGoalsToAdd);
  const std::set<Fact>& facts() const { return _facts; }
  const std::map<std::string, std::size_t>& factNamesToNbOfFactOccurences() const { return _factNamesToNbOfFactOccurences; }
  const std::map<std::string, std::string>& variablesToValue() const { return _variablesToValue; }
  const std::set<Fact>& reachableFacts() const { return _reachableFacts; }
  const std::set<Fact>& reachableFactsWithAnyValues() const { return _reachableFactsWithAnyValues; }
  const std::set<Fact>& removableFacts() const { return _removableFacts; }

  std::string printGoals(std::size_t pGoalNameMaxSize, const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow) const;

private:
  /// Map of priority to goals
  std::map<int, std::vector<Goal>> _goals{};
  std::map<std::string, std::string> _variablesToValue{};
  std::set<Fact> _facts{};
  std::map<std::string, std::size_t> _factNamesToNbOfFactOccurences{};
  std::set<Fact> _reachableFacts{};
  std::set<Fact> _reachableFactsWithAnyValues{};
  std::set<Fact> _removableFacts{};
  bool _needToAddReachableFacts = true;

  template<typename FACTS>
  bool _addFactsWithoutFactNotification(const FACTS& pFacts);

  template<typename FACTS>
  bool _removeFactsWithoutFactNotification(const FACTS& pFacts);

  void _clearRechableAndRemovableFacts();
  void _addFactNameRef(const std::string& pFactName);

  void _removeNoStackableGoals(bool pCheckOnlyForSecondGoal,
                               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
};

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_PROBLEM_HPP
