#include <contextualplanner/contextualplanner.hpp>
#include <contextualplanner/types/derivedpredicate.hpp>
#include <contextualplanner/types/predicate.hpp>
#include <contextualplanner/types/setofinferences.hpp>
#include <contextualplanner/util/trackers/goalsremovedtracker.hpp>
#include <contextualplanner/util/print.hpp>
#include <iostream>
#include <assert.h>
#include "test_arithmeticevaluator.hpp"
#include "test_ontology.hpp"
#include "test_util.hpp"
#include "docexamples/test_planningDummyExample.hpp"
#include "docexamples/test_planningExampleWithAPreconditionSolve.hpp"
#include "test_plannerWithoutTypes.hpp"


namespace
{
const auto _now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
const std::string _fact_a = "fact_a";
const std::string _fact_b = "fact_b";
const std::string _fact_c = "fact_c";

template <typename TYPE>
void assert_eq(const TYPE& pExpected,
               const TYPE& pValue)
{
  if (pExpected != pValue)
    assert(false);
}

template <typename TYPE>
void assert_true(const TYPE& pValue)
{
  if (!pValue)
    assert(false);
}

template <typename TYPE>
void assert_false(const TYPE& pValue)
{
  if (pValue)
    assert(false);
}

void _setGoalsForAPriority(cp::Problem& pProblem,
                           const std::vector<cp::Goal>& pGoals,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = cp::GoalStack::defaultPriority)
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pNow, pPriority);
}

cp::ActionInvocationWithGoal _lookForAnActionToDo(cp::Problem& pProblem,
                                                  const cp::Domain& pDomain,
                                                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                  const cp::Historical* pGlobalHistorical = nullptr)
{
  auto plan = cp::planForMoreImportantGoalPossible(pProblem, pDomain, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return cp::ActionInvocationWithGoal("", {}, {}, 0);
}




void _simplest_plan_possible()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr(_fact_a),
                        cp::WorldStateModification::fromStr(_fact_b));
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
  problem.worldState.addFact(cp::Fact(_fact_a), problem.goalStack, setOfInferencesMap, _now);

  assert_eq(action1, _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}

}




int main(int argc, char *argv[])
{
  test_arithmeticEvaluator();
  test_ontology();
  test_util();
  planningDummyExample();
  planningExampleWithAPreconditionSolve();

  _simplest_plan_possible();

  test_plannerWithoutTypes();
  std::cout << "chatbot planner is ok !!!!" << std::endl;
  return 0;
}
