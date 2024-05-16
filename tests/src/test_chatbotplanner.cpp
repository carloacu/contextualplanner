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


namespace
{
const std::map<cp::SetOfInferencesId, cp::SetOfInferences> _emptySetOfInferences;
const std::string _sep = ", ";

const std::string _fact_a = "fact_a";
const std::string _fact_b = "fact_b";
const std::string _fact_c = "fact_c";
const std::string _fact_d = "fact_d";
const std::string _fact_e = "fact_e";
const std::string _fact_f = "fact_f";
const std::string _fact_g = "fact_g";
const std::string _fact_punctual_p1 = cp::Fact::punctualPrefix + "fact_p1";
const std::string _fact_punctual_p2 = cp::Fact::punctualPrefix + "fact_p2";
const std::string _fact_punctual_p3 = cp::Fact::punctualPrefix + "fact_p3";
const std::string _fact_punctual_p4 = cp::Fact::punctualPrefix + "fact_p4";
const std::string _fact_punctual_p5 = cp::Fact::punctualPrefix + "fact_p5";
const std::string _fact_advertised = "advertised";
const std::string _fact_beginOfConversation = "begin_of_conversation";
const std::string _fact_presented = "presented";
const std::string _fact_greeted = "greeted";
const std::string _fact_is_close = "is_close";
const std::string _fact_hasQrCode = "has_qrcode";
const std::string _fact_hasCheckInPasword = "has_check_in_password";
const std::string _fact_userWantsToCheckedIn = "user_wants_to_checked_in";
const std::string _fact_checkedIn = "checked_in";
const std::string _fact_beSad = "be_sad";
const std::string _fact_beHappy = "be_happy";
const std::string _fact_askAllTheQuestions = "ask_all_the_questions";
const std::string _fact_finishToAskQuestions = "finished_to_ask_questions";
const std::string _fact_engagedWithUser = "engaged_with_user";
const std::string _fact_userSatisfied = "user_satisfied";
const std::string _fact_robotLearntABehavior = "robot_learnt_a_behavior";
const std::string _fact_headTouched = "head_touched";
const std::string _fact_punctual_headTouched = cp::Fact::punctualPrefix + "head_touched";
const std::string _fact_punctual_checkedIn = cp::Fact::punctualPrefix + "checked_in";

const std::string _action_presentation = "presentation";
const std::string _action_askQuestion1 = "ask_question_1";
const std::string _action_askQuestion2 = "ask_question_2";
const std::string _action_finisehdToAskQuestions = "finish_to_ask_questions";
const std::string _action_sayQuestionBilan = "say_question_bilan";
const std::string _action_greet = "greet";
const std::string _action_advertise = "advertise";
const std::string _action_checkIn = "check_in";
const std::string _action_joke = "joke";
const std::string _action_checkInWithRealPerson = "check_in_with_real_person";
const std::string _action_checkInWithQrCode = "check_in_with_qrcode";
const std::string _action_checkInWithPassword = "check_in_with_password";
const std::string _action_goodBoy = "good_boy";
const std::string _action_navigate = "navigate";
const std::string _action_welcome = "welcome";
const std::string _action_grab = "grab";
const std::string _action_ungrab = "ungrab";
static const std::vector<std::string> _emptyParameters;

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


std::string _solveStrConst(const cp::Problem& pProblem,
                           const std::map<std::string, cp::Action>& pActions,
                           cp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  cp::Domain domain(pActions);
  return cp::planToStr(cp::planForEveryGoals(problem, domain, {}, pGlobalHistorical), _sep);
}

std::string _solveStrConst(const cp::Problem& pProblem,
                           const cp::Domain& pDomain,
                           cp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  return cp::planToStr(cp::planForEveryGoals(problem, pDomain, {}, pGlobalHistorical), _sep);
}

std::string _solveStr(cp::Problem& pProblem,
                      const std::map<std::string, cp::Action>& pActions,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                      cp::Historical* pGlobalHistorical = nullptr)
{
  cp::Domain domain(pActions);
  return cp::planToStr(cp::planForEveryGoals(pProblem, domain, pNow, pGlobalHistorical), _sep);
}

std::string _solveStr(cp::Problem& pProblem,
                      const cp::Domain& pDomain,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                      cp::Historical* pGlobalHistorical = nullptr)
{
  return cp::planToStr(cp::planForEveryGoals(pProblem, pDomain, pNow, pGlobalHistorical), _sep);
}


std::string _getGoalsDoneDuringAPlannificationConst(const cp::Problem& pProblem,
                                                    const cp::Domain& pDomain,
                                                    cp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  std::list<cp::Goal> pGoalsDone;
  cp::planForEveryGoals(problem, pDomain, {}, pGlobalHistorical, &pGoalsDone);
  return cp::goalsToStr(pGoalsDone, _sep);
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

cp::ActionInvocationWithGoal _lookForAnActionToDoConst(const cp::Problem& pProblem,
                                                       const cp::Domain& pDomain,
                                                       const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                       const cp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  auto plan = cp::planForMoreImportantGoalPossible(problem, pDomain, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return cp::ActionInvocationWithGoal("", {}, {}, 0);
}

std::string _lookForAnActionToDoStr(cp::Problem& pProblem,
                                    const cp::Domain& pDomain,
                                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                    const cp::Historical* pGlobalHistorical = nullptr)
{
  return _lookForAnActionToDo(pProblem, pDomain, pNow, pGlobalHistorical).actionInvocation.toStr();
}

std::string _lookForAnActionToDoConstStr(const cp::Problem& pProblem,
                                         const cp::Domain& pDomain,
                                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                         const cp::Historical* pGlobalHistorical = nullptr)
{
  return _lookForAnActionToDoConst(pProblem, pDomain, pNow, pGlobalHistorical).actionInvocation.toStr();
}

cp::ActionInvocationWithGoal _lookForAnActionToDoThenNotify(
    cp::Problem& pProblem,
    const cp::Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto plan = cp::planForMoreImportantGoalPossible(pProblem, pDomain, true, pNow);
  if (!plan.empty())
  {
    auto& firstActionInPlan = plan.front();
    notifyActionStarted(pProblem, pDomain, firstActionInPlan, pNow);
    notifyActionDone(pProblem, pDomain, firstActionInPlan, pNow);
    return firstActionInPlan;
  }
  return cp::ActionInvocationWithGoal("", {}, {}, 0);
}


std::string _lookForAnActionToDoInParallelThenNotifyToStr(
    cp::Problem& pProblem,
    const cp::Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto actions = cp::actionsToDoInParallelNow(pProblem, pDomain, pNow);
  for (auto& currAction : actions)
  {
    notifyActionStarted(pProblem, pDomain, currAction, pNow);
    notifyActionDone(pProblem, pDomain, currAction, pNow);
  }
  return cp::planToStr(actions);
}


void _setGoalsForAPriority(cp::Problem& pProblem,
                           const std::vector<cp::Goal>& pGoals,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = cp::GoalStack::defaultPriority)
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pNow, pPriority);
}


void _test_createEmptyGoal()
{
  cp::Goal("goal_name", -1, "");
}

void _test_goalToStr()
{
  assert_eq<std::string>("persist(a & b)", cp::Goal("persist(a & b)").toStr());
  assert_eq<std::string>("imply(condition, goal_name)", cp::Goal("imply(condition, goal_name)").toStr());
  assert_eq<std::string>("persist(imply(condition, goal_name))", cp::Goal("persist(imply(condition, goal_name))").toStr());
  assert_eq<std::string>("oneStepTowards(goal_name)", cp::Goal("oneStepTowards(goal_name)").toStr());
}

void _test_factToStr()
{
  assert_eq<std::string>("isEngaged(1)", cp::Fact("isEngaged(1)").toStr());
}


void _test_conditionParameters()
{
  assert_false(cp::Condition::fromStr("").operator bool());

  std::map<std::string, std::string> parameters = {{"target", "kitchen"}, {"object", "chair"}};
  assert_eq<std::string>("location(me)=kitchen & grab(me, chair)", cp::Condition::fromStr("location(me)=target & grab(me, object)")->clone(&parameters)->toStr());
  assert_eq<std::string>("location(me)=kitchen & grab(me, chair)", cp::Condition::fromStr("and(location(me)=target, grab(me, object))")->clone(&parameters)->toStr());
  assert_eq<std::string>("location(me)=kitchen & grab(me, chair) & i", cp::Condition::fromStr("and(location(me)=target, grab(me, object), i)")->clone(&parameters)->toStr());
  assert_eq<std::string>("location(me)=kitchen | grab(me, chair) | i", cp::Condition::fromStr("location(me)=target | grab(me, object) | i")->clone(&parameters)->toStr());
  assert_eq<std::string>("location(me)=kitchen | grab(me, chair) | i", cp::Condition::fromStr("or(location(me)=target, grab(me, object), i)")->clone(&parameters)->toStr());
  assert_eq<std::string>("equals(a, b + 3)", cp::Condition::fromStr("equals(a, b + 3)")->toStr());
  assert_eq<std::string>("!a", cp::Condition::fromStr("!a")->toStr());
  assert_eq<std::string>("!a", cp::Condition::fromStr("not(a)")->toStr());
  assert_eq<std::string>("a!=", cp::Condition::fromStr("a!=")->toStr());
  assert_eq<std::string>("a!=b", cp::Condition::fromStr("a!=b")->toStr());
  assert_eq<std::string>("a>3", cp::Condition::fromStr("a>3")->toStr());
  assert_eq<std::string>("a>3", cp::Condition::fromStr(">(a, 3)")->toStr());
  assert_eq<std::string>("a<3", cp::Condition::fromStr("<(a, 3)")->toStr());
  assert_eq<std::string>("!a=*", cp::Condition::fromStr("=(a, undefined)")->toStr());
  assert_eq<std::string>("!a(b)=*", cp::Condition::fromStr("=(a(b), undefined)")->toStr());
  assert_eq<std::string>("!a=*", cp::Condition::fromStr("a=undefined")->toStr());
  assert_eq<std::string>("!a(b)=*", cp::Condition::fromStr("a(b)=undefined")->toStr());
  assert_eq<std::string>("a=c", cp::Condition::fromStr("=(a, c)")->toStr());
  assert_eq<std::string>("a(b)=c", cp::Condition::fromStr("=(a(b), c)")->toStr());
  assert_eq<std::string>("equals(a, c)", cp::Condition::fromStr("=(a, c())")->toStr());
  assert_eq<std::string>("equals(a(b), c)", cp::Condition::fromStr("=(a(b), c())")->toStr());
  assert_eq<std::string>("equals(a, c)", cp::Condition::fromStr("equals(a, c)")->toStr());
  assert_eq<std::string>("equals(a(b), c)", cp::Condition::fromStr("equals(a(b), c)")->toStr());
  assert_eq<std::string>("equals(a, c)", cp::Condition::fromStr("equals(a, c())")->toStr());
  assert_eq<std::string>("equals(a(b), c)", cp::Condition::fromStr("equals(a(b), c())")->toStr());
  assert_eq<std::string>("exists(l, at(self, l))", cp::Condition::fromStr("exists(l, at(self, l))")->toStr());
  assert_eq<std::string>("exists(l, at(self, l) & at(pen, l))", cp::Condition::fromStr("exists(l, at(self, l) & at(pen, l))")->toStr());
  assert_eq<std::string>("!exists(l, at(self, l))", cp::Condition::fromStr("not(exists(l, at(self, l)))")->toStr());
  assert_eq<std::string>("!(equals(at(i, o), at(self, l)))", cp::Condition::fromStr("not(equals(at(i, o), at(self, l)))")->toStr());
  assert_eq<std::string>("!(equals(at(i, o), at(self, l)))", cp::Condition::fromStr("not(=(at(i, o), at(self, l)))")->toStr());
}

void _test_wsModificationToStr()
{
  assert_false(cp::WorldStateModification::fromStr("").operator bool());
  assert_eq<std::string>("location(me)=target", cp::WorldStateModification::fromStr("location(me)=target")->toStr());
  assert_eq<std::string>("location(me)=target & grab(sweets)", cp::WorldStateModification::fromStr("location(me)=target & grab(sweets)")->toStr());
  assert_eq<std::string>("location(me)=target & grab(sweets)", cp::WorldStateModification::fromStr("and(location(me)=target, grab(sweets))")->toStr());
  assert_eq<std::string>("assign(a, b + 3)", cp::WorldStateModification::fromStr("assign(a, b + 3)")->toStr());
  assert_eq<std::string>("assign(a, b + 4 - 1)", cp::WorldStateModification::fromStr("set(a, b + 4 - 1)")->toStr()); // set is depecated
  assert_eq<std::string>("increase(a, 1)", cp::WorldStateModification::fromStr("add(a, 1)")->toStr());
  assert_eq<std::string>("increase(a, 1)", cp::WorldStateModification::fromStr("increase(a, 1)")->toStr());
  assert_eq<std::string>("decrease(a, 2)", cp::WorldStateModification::fromStr("decrease(a, 2)")->toStr());
  assert_eq<std::string>("!a", cp::WorldStateModification::fromStr("!a")->toStr());
  assert_eq<std::string>("!a", cp::WorldStateModification::fromStr("not(a)")->toStr());
  assert_eq<std::string>("!a=*", cp::WorldStateModification::fromStr("a=undefined")->toStr());
  assert_eq<std::string>("!a=*", cp::WorldStateModification::fromStr("assign(a, undefined)")->toStr());
  assert_eq<std::string>("forall(a, f(a), d(a, c))", cp::WorldStateModification::fromStr("forall(a, f(a), d(a, c))")->toStr());
  assert_eq<std::string>("forall(a, f(a), d(a, c))", cp::WorldStateModification::fromStr("forall(a, when(f(a), d(a, c)))")->toStr());
  assert_eq<std::string>("forall(a, f(a), !d(a, c))", cp::WorldStateModification::fromStr("forall(a, when(f(a), not(d(a, c))))")->toStr());
  assert_eq<std::string>("assign(a(b), c(d))", cp::WorldStateModification::fromStr("assign(a(b), c(d))")->toStr());
  assert_eq<std::string>("assign(a(b), c(d))", cp::WorldStateModification::fromStr("set(a(b), c(d))")->toStr()); // set is depecated
  assert_eq<std::string>("assign(a(b), c())", cp::WorldStateModification::fromStr("assign(a(b), c())")->toStr()); // c() means that c is a predicate
  assert_eq<std::string>("a(b)=c", cp::WorldStateModification::fromStr("assign(a(b), c)")->toStr());
  assert_eq<std::string>("assign(a(b), c())", cp::WorldStateModification::fromStr("set(a(b), c())")->toStr()); // set is depecated
  assert_eq<std::string>("assign(a(b), c())", cp::WorldStateModification::fromStr("set(a(b), c)")->toStr()); // set is depecated
}

void _test_invertCondition()
{
  assert_eq<std::string>("!location(me)=kitchen | grab(me, chair) | !i", cp::Condition::fromStr("location(me)=kitchen & !grab(me, chair) & i)")->clone(nullptr, true)->toStr());
  assert_eq<std::string>("!location(me)=kitchen & !grab(me, chair) & i", cp::Condition::fromStr("location(me)=kitchen | grab(me, chair) | !i")->clone(nullptr, true)->toStr());
  assert_eq<std::string>("equals(at(i, o), at(self, l))", cp::Condition::fromStr("not(=(at(i, o), at(self, l)))")->clone(nullptr, true)->toStr());
}


void _test_checkCondition()
{
  cp::WorldState worldState;
  cp::GoalStack goalStack;
  std::map<cp::SetOfInferencesId, cp::SetOfInferences> setOfInferences;
  worldState.addFact(cp::Fact("a=c"), goalStack, setOfInferences, {});
  assert_true(cp::Condition::fromStr("a!=b")->isTrue(worldState));
  assert_false(cp::Condition::fromStr("a!=c")->isTrue(worldState));
  assert_eq<std::size_t>(1, worldState.facts().size());
  worldState.addFact(cp::Fact("a!=c"), goalStack, setOfInferences, {});
  assert_eq<std::size_t>(1, worldState.facts().size());
  assert_false(cp::Condition::fromStr("a!=b")->isTrue(worldState));
  assert_true(cp::Condition::fromStr("a!=c")->isTrue(worldState));
  worldState.addFact(cp::Fact("a!=b"), goalStack, setOfInferences, {});
  assert_eq<std::size_t>(2, worldState.facts().size());
  assert_true(cp::Condition::fromStr("a!=b")->isTrue(worldState));
  assert_true(cp::Condition::fromStr("a!=c")->isTrue(worldState));
  worldState.addFact(cp::Fact("a=d"), goalStack, setOfInferences, {});
  assert_eq<std::size_t>(1, worldState.facts().size());
  assert_true(cp::Condition::fromStr("a!=b")->isTrue(worldState));
  assert_true(cp::Condition::fromStr("a!=c")->isTrue(worldState));
  assert_true(cp::Condition::fromStr("a=d")->isTrue(worldState));
  assert_false(cp::Condition::fromStr("a!=d")->isTrue(worldState));
  worldState.addFact(cp::Fact("a!=c"), goalStack, setOfInferences, {});
  assert_eq<std::size_t>(1, worldState.facts().size());
  assert_true(cp::Condition::fromStr("a=d")->isTrue(worldState));
}


void _automaticallyRemoveGoalsWithAMaxTimeToKeepInactiveEqualTo0()
{
  cp::GoalStack goalStack;
  cp::WorldState worldState;
  assert_eq<std::size_t>(0u, goalStack.goals().size());
  goalStack.pushBackGoal(cp::Goal(_fact_advertised), worldState, {}, 10);
  goalStack.pushBackGoal(cp::Goal(_fact_beHappy), worldState, {}, 9);
  assert_eq<std::size_t>(2u, goalStack.goals().size());
  goalStack.pushBackGoal(cp::Goal(_fact_checkedIn, 0), worldState, {}, 9);
  assert_eq<std::size_t>(2u, goalStack.goals().size());
  assert_eq<std::size_t>(1u, goalStack.goals().find(9)->second.size());
}


void _maxTimeToKeepInactiveEqualTo0UnderAnAlreadySatisfiedGoal()
{
  cp::GoalStack goalStack;
  cp::WorldState worldState;
  assert_eq<std::size_t>(0u, goalStack.goals().size());
  goalStack.pushBackGoal(cp::Goal("persist(!" + _fact_a + ")"), worldState, {}, 10);
  assert_eq<std::size_t>(1u, goalStack.goals().size());
  goalStack.pushBackGoal(cp::Goal(_fact_checkedIn, 0), worldState, {}, 9);
  assert_eq<std::size_t>(2u, goalStack.goals().size());
}

void _noPreconditionGoalImmediatlyReached()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_goodBoy, cp::Action({},
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_goodBoy, _lookForAnActionToDoStr(problem, domain));
  assert_true(!problem.goalStack.goals().empty());
  assert_true(!problem.worldState.hasFact(_fact_beHappy));
  problem.worldState.addFact(_fact_beHappy, problem.goalStack, _emptySetOfInferences, now);
  assert_true(problem.worldState.hasFact(_fact_beHappy));
}


void _removeGoalWhenItIsSatisfiedByAnAction()
{
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_goodBoy, cp::Action({},
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});

  auto plannerResult = _lookForAnActionToDoThenNotify(problem, domain);
  assert_eq(_action_goodBoy, plannerResult.actionInvocation.toStr());
  assert_eq(_fact_beHappy, plannerResult.fromGoal->toStr());
  assert_eq(10, plannerResult.fromGoalPriority);
  assert_true(problem.goalStack.goals().empty());
}


void _removeAnAction()
{
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_goodBoy, cp::Action({},
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  domain.removeAction(_action_goodBoy);
  assert_eq<std::string>("", _lookForAnActionToDoConstStr(problem, domain));
}


void _removeSomeGoals()
{
  const std::string goalGroupId = "greetAndCheckIn";
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action({}, cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  cp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });
  problem.goalStack.pushFrontGoal(cp::Goal(_fact_checkedIn, -1, goalGroupId), problem.worldState, {});
  problem.goalStack.pushFrontGoal(cp::Goal(_fact_greeted, -1, goalGroupId), problem.worldState, {});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq<std::size_t>(0, goalsRemoved.size());
  assert_true(problem.goalStack.removeGoals(goalGroupId, problem.worldState, {}));
  assert_eq<std::size_t>(2, goalsRemoved.size());
  assert_false(problem.goalStack.removeGoals(goalGroupId, problem.worldState, {}));
  assert_eq(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  problem.goalStack.clearGoals(problem.worldState, {});
  assert_eq<std::string>("", _lookForAnActionToDoConstStr(problem, domain));
}


void _notifyGoalRemovedWhenItIsImmediatlyRemoved()
{
  cp::Problem problem;
  cp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });

  problem.goalStack.addGoals({{10, {_fact_a}}}, problem.worldState, {});

  problem.goalStack.addGoals({{9, {cp::Goal(_fact_b, 0)}}}, problem.worldState, {});
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_b, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushBackGoal(cp::Goal(_fact_c, 0), problem.worldState, {}, 9);
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_c, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushFrontGoal(cp::Goal(_fact_d, 0), problem.worldState, {}, 9);
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_d, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushFrontGoal(cp::Goal(_fact_e, 0), problem.worldState, {}, 11);
  assert_eq<std::size_t>(0u, goalsRemoved.size());
  problem.goalStack.changeGoalPriority(_fact_e, 9, true, problem.worldState, {});
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_e, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.setGoals({{10, {cp::Goal(_fact_a, 0)}}, {9, {cp::Goal(_fact_b, 0)}}}, problem.worldState, {});
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_b, *goalsRemoved.begin());
}



void _handlePreconditionWithNegatedFacts()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action(cp::Condition::fromStr("!" + _fact_checkedIn),
                                            cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_joke, cp::Action(cp::Condition::fromStr("!" + _fact_checkedIn),
                                           cp::WorldStateModification::fromStr(_fact_userSatisfied)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_userSatisfied),
                                              cp::WorldStateModification::fromStr(_fact_checkedIn)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_checkedIn});
  auto actionToDo = _lookForAnActionToDoStr(problem, domain);
  assert_true(actionToDo == _action_greet || actionToDo == _action_joke);
}


void _testWithNegatedAccessibleFacts()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};

  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action(cp::Condition::fromStr(_fact_e + " & !" + _fact_b),
                                      cp::WorldStateModification::fromStr("!" + _fact_c)));
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr("!" + _fact_b)));
  actions.emplace(action3, cp::Action(cp::Condition::fromStr(_fact_a + " & !" + _fact_c),
                                      cp::WorldStateModification::fromStr(_fact_d)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.worldState.setFacts({_fact_a, _fact_b, _fact_c}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_d});
  assert_eq<std::string>("", _lookForAnActionToDoConstStr(problem, domain));
  problem.worldState.addFact(_fact_e,  problem.goalStack, _emptySetOfInferences, now);
  assert_eq(action2, _lookForAnActionToDoStr(problem, domain));
}

void _noPlanWithALengthOf2()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _noPlanWithALengthOf3()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action(cp::Condition::fromStr(_fact_greeted),
                                              cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}

void _2preconditions()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}

void _2Goals()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}

void _2UnrelatedGoals()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _impossibleGoal()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy});
  assert_eq(_action_checkIn, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _privigelizeTheActionsThatHaveManyPreferedInContext()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_checkInWithQrCode, cp::Action(cp::Condition::fromStr(_fact_hasQrCode),
                                                        cp::WorldStateModification::fromStr(_fact_checkedIn),
                                                        cp::Condition::fromStr(_fact_hasQrCode)));
  actions.emplace(_action_checkInWithPassword, cp::Action(cp::Condition::fromStr(_fact_hasCheckInPasword),
                                                          cp::WorldStateModification::fromStr(_fact_checkedIn),
                                                          cp::Condition::fromStr(_fact_hasCheckInPasword)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasQrCode}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkInWithQrCode + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasCheckInPasword}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  assert_eq(_action_greet + _sep +
            _action_checkInWithPassword + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));
}

void _preconditionThatCannotBeSolved()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkInWithQrCode, cp::Action(cp::Condition::fromStr(_fact_hasQrCode),
                                                        cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_checkInWithPassword, cp::Action(cp::Condition::fromStr(_fact_hasCheckInPasword),
                                                          cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_true(_lookForAnActionToDoStr(problem, domain).empty());
}


void _preferInContext()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkInWithQrCode, cp::Action({},
                                                        cp::WorldStateModification::fromStr(_fact_checkedIn),
                                                        cp::Condition::fromStr(_fact_hasQrCode)));
  actions.emplace(_action_checkInWithPassword, cp::Action({},
                                                          cp::WorldStateModification::fromStr(_fact_checkedIn),
                                                          cp::Condition::fromStr(_fact_hasCheckInPasword)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_greeted + " & " + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_greet + _sep +
            _action_checkInWithPassword + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasQrCode}, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(_action_checkInWithQrCode + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasCheckInPasword}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_checkInWithPassword + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  problem.worldState.setFacts({}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasQrCode}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_checkInWithQrCode + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_hasCheckInPasword}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_checkInWithPassword + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  actions.emplace(_action_checkInWithRealPerson, cp::Action({},
                                                            cp::WorldStateModification::fromStr(_fact_checkedIn),
                                                            cp::Condition::fromStr("!" + _fact_hasQrCode)));
  problem.worldState.setFacts({}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_checkInWithRealPerson + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));
}


void _preferWhenPreconditionAreCloserToTheRealFacts()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({},
                                            cp::WorldStateModification::fromStr(_fact_greeted + "&" + _fact_presented),
                                            cp::Condition::fromStr(_fact_beginOfConversation)));
  actions.emplace(_action_presentation, cp::Action({}, cp::WorldStateModification::fromStr(_fact_presented)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_presented + "&" + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn + _sep +
            _action_presentation + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  problem.worldState.setFacts({_fact_beginOfConversation}, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _avoidToDo2TimesTheSameActionIfPossble()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({},
                                            cp::WorldStateModification::fromStr(_fact_greeted + "&" + _fact_presented)));
  actions.emplace(_action_presentation, cp::Action({},
                                                   cp::WorldStateModification::fromStr(_fact_presented)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_presented + "&" + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions));

  _setGoalsForAPriority(problem, {_fact_greeted});
  assert_eq(_action_greet, _solveStr(problem, actions));

  problem.worldState.setFacts({}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn + _sep +
            _action_presentation + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _takeHistoricalIntoAccount()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted + "&" + _fact_presented)));
  actions.emplace(_action_presentation, cp::Action({}, cp::WorldStateModification::fromStr(_fact_presented)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_presented + "&" + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, &problem.historical));

  assert_eq(_action_presentation + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, &problem.historical));
}


void _goDoTheActionThatHaveTheMostPreferInContextValidated()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_advertise, cp::Action({}, cp::WorldStateModification::fromStr(_fact_advertised)));
  actions.emplace(_action_checkIn, cp::Action(cp::Condition::fromStr(_fact_is_close),
                                              cp::WorldStateModification::fromStr(_fact_checkedIn),
                                              cp::Condition::fromStr(_fact_is_close)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_advertised + "&" + _fact_checkedIn),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.worldState.setFacts({_fact_is_close}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_checkIn, _lookForAnActionToDoStr(problem, domain));
}


void _checkNotInAPrecondition()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action(cp::Condition::fromStr("!" + _fact_checkedIn),
                                            cp::WorldStateModification::fromStr(_fact_greeted)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted});
  assert_eq(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  problem.worldState.modify(cp::WorldStateModification::fromStr(_fact_checkedIn), problem.goalStack,
                                 _emptySetOfInferences, now);
  assert_eq(std::string(), _lookForAnActionToDoConstStr(problem, domain));
}


void _checkClearGoalsWhenItsAlreadySatisfied()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
 cp::Problem problem;
  problem.worldState.setFacts({_fact_greeted}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_greeted});
  assert_eq<std::size_t>(1, problem.goalStack.goals().size());
  cp::Domain domain;
  _lookForAnActionToDo(problem, domain);
  assert_eq<std::size_t>(0, problem.goalStack.goals().size());
}


void _checkActionHasAFact()
{
  cp::ProblemModification effect(cp::WorldStateModification::fromStr(_fact_a + " & !" + _fact_b));
  effect.potentialWorldStateModification = cp::WorldStateModification::fromStr(_fact_c);
  effect.goalsToAdd[cp::GoalStack::defaultPriority] = {_fact_d};
  const cp::Action action(cp::Condition::fromStr(_fact_e),
                          effect,
                          cp::Condition::fromStr(_fact_f));
  assert_true(action.hasFact(_fact_a));
  assert_true(action.hasFact(_fact_b));
  assert_true(action.hasFact(_fact_c));
  assert_true(action.hasFact(_fact_d));
  assert_true(action.hasFact(_fact_e));
  assert_true(action.hasFact(_fact_f));
  assert_false(action.hasFact(_fact_g));
}

void _checkActionReplacefact()
{
  cp::ProblemModification effect(cp::WorldStateModification::fromStr(_fact_a + " & !" + _fact_b));
  effect.potentialWorldStateModification = cp::WorldStateModification::fromStr(_fact_c);
  effect.goalsToAdd[cp::GoalStack::defaultPriority] = {_fact_d};
  cp::Action action(cp::Condition::fromStr(_fact_e),
                    effect,
                    cp::Condition::fromStr(_fact_f));
  assert_true(action.hasFact(_fact_a));
  assert_false(action.hasFact(_fact_g));
  action.replaceFact(_fact_a, _fact_g);
  assert_false(action.hasFact(_fact_a));
  assert_true(action.hasFact(_fact_g));
}



void _testIncrementOfVariables()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  const cp::Action actionQ1({}, cp::WorldStateModification::fromStr(_fact_askAllTheQuestions + " & add(numberOfQuestion, 1)"));
  const cp::Action actionFinishToActActions(cp::Condition::fromStr("equals(numberOfQuestion, maxNumberOfQuestions)"),
                                            cp::WorldStateModification::fromStr(_fact_askAllTheQuestions));
  const cp::Action actionSayQuestionBilan(cp::Condition::fromStr(_fact_askAllTheQuestions),
                                          cp::WorldStateModification::fromStr(_fact_finishToAskQuestions));
  actions.emplace(_action_askQuestion1, actionQ1);
  actions.emplace(_action_askQuestion2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_askAllTheQuestions + " & add(numberOfQuestion, 1)")));
  actions.emplace(_action_finisehdToAskQuestions, actionFinishToActActions);
  actions.emplace(_action_sayQuestionBilan, actionSayQuestionBilan);
  cp::Domain domain(std::move(actions));

  std::string initFactsStr = "numberOfQuestion=0 & maxNumberOfQuestions=3";
  cp::Problem problem;
  problem.worldState.modify(cp::WorldStateModification::fromStr(initFactsStr), problem.goalStack, _emptySetOfInferences, now);
  assert(cp::Condition::fromStr(initFactsStr)->isTrue(problem.worldState));
  assert(!actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  assert(cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 3)")->isTrue(problem.worldState));
  assert(!cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 4)")->isTrue(problem.worldState));
  assert(cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 4 - 1)")->isTrue(problem.worldState));
  for (std::size_t i = 0; i < 3; ++i)
  {
    _setGoalsForAPriority(problem, {_fact_finishToAskQuestions});
    auto actionToDo = _lookForAnActionToDoStr(problem, domain);
    if (i == 0 || i == 2)
      assert_eq<std::string>(_action_askQuestion1, actionToDo);
    else
      assert_eq<std::string>(_action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(itAction->second.effect.worldStateModification, problem.goalStack,
                                   _emptySetOfInferences, now);
    problem.worldState.modify(cp::WorldStateModification::fromStr("!" + _fact_askAllTheQuestions), problem.goalStack,
                                   _emptySetOfInferences, now);
  }
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  _setGoalsForAPriority(problem, {_fact_finishToAskQuestions});
  auto actionToDo = _lookForAnActionToDoStr(problem, domain);
  assert_eq<std::string>(_action_finisehdToAskQuestions, actionToDo);
  problem.historical.notifyActionDone(actionToDo);
  auto itAction = domain.actions().find(actionToDo);
  assert(itAction != domain.actions().end());
  problem.worldState.modify(itAction->second.effect.worldStateModification, problem.goalStack,
                                 _emptySetOfInferences, now);
  assert_eq<std::string>(_action_sayQuestionBilan, _lookForAnActionToDoStr(problem, domain));
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  problem.worldState.modify(actionSayQuestionBilan.effect.worldStateModification, problem.goalStack,
                                 _emptySetOfInferences, now);
}

void _precoditionEqualEffect()
{
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr(_fact_beHappy),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_true(_lookForAnActionToDoStr(problem, domain).empty());
}


void _addGoalEvenForEmptyAction()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  std::map<cp::ActionId, cp::Action> actions;
  cp::Action act1Obj({}, {});
  act1Obj.effect.goalsToAddInCurrentPriority.push_back(cp::Goal(_fact_a));
  actions.emplace(action1, act1Obj);
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a)));

  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  assert_true(problem.goalStack.goals().empty());
  cp::notifyActionDone(problem, domain, cp::ActionInvocationWithGoal(action1, {}, {}, 0), {});
  assert_false(problem.goalStack.goals().empty());
}

void _circularDependencies()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action(cp::Condition::fromStr(_fact_greeted),
                                              cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace("check-in-pwd", cp::Action(cp::Condition::fromStr(_fact_hasCheckInPasword),
                                             cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace("inverse-of-check-in-pwd", cp::Action(cp::Condition::fromStr(_fact_checkedIn),
                                                        cp::WorldStateModification::fromStr(_fact_hasCheckInPasword)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq<std::string>("", _lookForAnActionToDoConstStr(problem, domain));
}


void _triggerActionThatRemoveAFact()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_joke, cp::Action(cp::Condition::fromStr(_fact_beSad),
                                           cp::WorldStateModification::fromStr("!" + _fact_beSad)));
  actions.emplace(_action_goodBoy, cp::Action(cp::Condition::fromStr("!" + _fact_beSad),
                                              cp::WorldStateModification::fromStr(_fact_beHappy)));

  cp::Historical historical;
  cp::Problem problem;
  problem.worldState.addFact(_fact_beSad, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_beHappy});
  assert_eq(_action_joke + _sep +
            _action_goodBoy, _solveStr(problem, actions, {}, &historical));
}


void _actionWithConstantValue()
{
  std::map<std::string, cp::Action> actions;
  cp::Action navigate({}, cp::WorldStateModification::fromStr("place=kitchen"));
  actions.emplace(_action_navigate, navigate);

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("place=kitchen")});
  assert_eq(_action_navigate, _solveStr(problem, actions));
}


void _actionWithParameterizedValue()
{
  std::map<std::string, cp::Action> actions;
  cp::Action navigate({}, cp::WorldStateModification::fromStr("place=target"));
  navigate.parameters.emplace_back("target");
  actions.emplace(_action_navigate, navigate);

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("place=kitchen")});
  assert_eq(_action_navigate + "(target -> kitchen)", _solveStr(problem, actions));
}


void _actionWithParameterizedParameter()
{
  std::map<std::string, cp::Action> actions;
  cp::Action joke({}, cp::WorldStateModification::fromStr("isHappy(human)"));
  joke.parameters.emplace_back("human");
  actions.emplace(_action_joke, joke);

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("isHappy(1)")});
  assert_eq(_action_joke + "(human -> 1)", _solveStr(problem, actions));
}



void _actionWithParametersInPreconditionsAndEffects()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  cp::Action joke(cp::Condition::fromStr("isEngaged(human)"),
                  cp::WorldStateModification::fromStr("isHappy(human)"));
  joke.parameters.emplace_back("human");
  actions.emplace(_action_joke, joke);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact::fromStr("isEngaged(1)"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal("isHappy(1)")});
  assert_eq(_action_joke + "(human -> 1)", _solveStr(problem, actions));
}


void _actionWithParametersInPreconditionsAndEffectsWithoutSolution()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  cp::Action joke(cp::Condition::fromStr("isEngaged(human)"),
                  cp::WorldStateModification::fromStr("isHappy(human)"));
  joke.parameters.emplace_back("human");
  actions.emplace(_action_joke, joke);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact::fromStr("isEngaged(2)"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal("isHappy(1)")});
  assert_eq<std::string>("", _solveStr(problem, actions));
}

void _actionWithParametersInsideThePath()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  cp::Action navigateAction({},
                            cp::WorldStateModification::fromStr("place=target"));
  navigateAction.parameters.emplace_back("target");
  actions.emplace(_action_navigate, navigateAction);

  actions.emplace(_action_welcome,
                  cp::Action(cp::Condition::fromStr("place=entrance"),
                             cp::WorldStateModification::fromStr("welcomePeople")));

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact::fromStr("place=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal("welcomePeople")});
  assert_eq<std::string>(_action_navigate + "(target -> entrance)" + _sep +
                         _action_welcome, _solveStr(problem, actions));
  assert_true(problem.worldState.hasFact(cp::Fact::fromStr("place=entrance")));
  assert_false(problem.worldState.hasFact(cp::Fact::fromStr("place=kitchen")));
}


void _testPersistGoal()
{
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_welcome, cp::Action({}, cp::WorldStateModification::fromStr("welcomePeople")));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("welcomePeople")});
  assert_eq<std::size_t>(1, problem.goalStack.goals().size());
  assert_eq<std::string>(_action_welcome, _solveStr(problem, actions));
  assert_eq<std::size_t>(0, problem.goalStack.goals().size());
  assert_eq<std::string>("", _solveStr(problem, actions));
  assert_eq<std::size_t>(0, problem.goalStack.goals().size());

  problem = cp::Problem();
  _setGoalsForAPriority(problem, {cp::Goal("persist(welcomePeople)")});
  assert_eq<std::size_t>(1, problem.goalStack.goals().size());
  assert_eq<std::string>(_action_welcome, _solveStr(problem, actions));
  assert_eq<std::size_t>(1, problem.goalStack.goals().size());
  assert_eq<std::string>("", _solveStr(problem, actions));
  assert_eq<std::size_t>(1, problem.goalStack.goals().size());
}


void _testPersistImplyGoal()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("persist(imply(" + _fact_greeted + ", " + _fact_checkedIn + "))")});
  assert_eq<std::string>("", _solveStr(problem, actions));
  problem.worldState.addFact(_fact_greeted, problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>(_action_checkIn, _solveStr(problem, actions));
}


void _testImplyGoal()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("imply(" + _fact_greeted + ", " + _fact_checkedIn + ")")});
  assert_eq<std::string>("", _solveStr(problem, actions));
  // It is not a persistent goal it is removed
  problem.worldState.addFact(_fact_greeted, problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>("", _solveStr(problem, actions));
}


void _checkPreviousBugAboutSelectingAnInappropriateAction()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_askQuestion1, cp::Action(cp::Condition::fromStr(_fact_engagedWithUser),
                                                   cp::WorldStateModification::fromStr(_fact_userSatisfied),
                                                   cp::Condition::fromStr("!" + _fact_robotLearntABehavior)));
  actions.emplace(_action_checkIn, cp::Action({},
                                              cp::WorldStateModification::fromStr("!" + _fact_robotLearntABehavior + " & " + _fact_advertised)));
  cp::Domain domain(std::move(actions));

  cp::Historical historical;
  cp::Problem problem;
  problem.worldState.setFacts({_fact_engagedWithUser}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {"persist(" + _fact_userSatisfied + ")"});
  assert_eq<std::string>(_action_askQuestion1, _solveStr(problem, actions));
  problem.worldState.removeFact(_fact_userSatisfied, problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>(_action_askQuestion1, _solveStr(problem, actions));
}


void _dontLinkActionWithPreferredInContext()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  actions.emplace(_action_askQuestion1, cp::Action({},
                                                   cp::WorldStateModification::fromStr(_fact_userSatisfied),
                                                   cp::Condition::fromStr(_fact_checkedIn)));
  actions.emplace(_action_checkIn, cp::Action(cp::Condition::fromStr(_fact_engagedWithUser),
                                              cp::WorldStateModification::fromStr(_fact_checkedIn)));
  cp::Domain domain(std::move(actions));

  cp::Historical historical;
  cp::Problem problem;
  problem.worldState.setFacts({_fact_engagedWithUser}, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {_fact_userSatisfied});
  assert_eq<std::string>(_action_askQuestion1, _solveStr(problem, actions));
}


void _checkPriorities()
{
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action({}, cp::WorldStateModification::fromStr(_fact_beHappy)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.goalStack.setGoals({{10, {_fact_greeted}}, {9, {_fact_beHappy}}}, problem.worldState, {});
  assert_eq(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions));
}


void _stackablePropertyOfGoals()
{
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action({}, cp::WorldStateModification::fromStr(_fact_beHappy)));
  actions.emplace(_action_presentation, cp::Action({}, cp::WorldStateModification::fromStr(_fact_presented)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.goalStack.setGoals({{10, {cp::Goal(_fact_greeted, 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}}, problem.worldState, {});
  assert_eq(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions));

  cp::Problem problem2;
  problem2.goalStack.setGoals({{10, {cp::Goal(_fact_greeted, 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}}, problem2.worldState, {});
  problem2.goalStack.pushFrontGoal(_fact_presented, problem2.worldState, {}, 10);
  assert_eq(_action_presentation + _sep +
            _action_goodBoy, _solveStr(problem2, actions));
}



void _doNotRemoveAGoalWithMaxTimeToKeepInactiveEqual0BelowAGoalWithACondotionNotSatisfied()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  actions.emplace(_action_goodBoy, cp::Action({}, cp::WorldStateModification::fromStr(_fact_beHappy)));
  actions.emplace(_action_presentation, cp::Action({}, cp::WorldStateModification::fromStr(_fact_presented)));

  // Even if _fact_checkedIn has maxTimeToKeepInactive equal to 0, it is not removed because the goal with a higher priority is inactive.
  cp::Problem problem;
  problem.goalStack.setGoals({{10, {cp::Goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}},
                     problem.worldState, {});
  assert_eq(_action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions));

  cp::Problem problem2;
  problem2.worldState.addFact(_fact_presented, problem2.goalStack, _emptySetOfInferences, now); // The difference here is that the condition of the first goal is satisfied
  problem2.goalStack.setGoals({{10, {cp::Goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}},
                              problem2.worldState, {});
  assert_eq(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem2, actions));


  cp::Problem problem3;
  problem3.goalStack.setGoals({{10, {cp::Goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}},
                      problem3.worldState, {});
  problem3.worldState.addFact(_fact_presented, problem3.goalStack, _emptySetOfInferences, now); // The difference here is that the condition is validated after the add of the goal
  assert_eq(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem3, actions));


  cp::Problem problem4;
  problem4.goalStack.setGoals({{10, {cp::Goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", 0)}}, {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}},
                      problem4.worldState, {});
  problem4.worldState.addFact(_fact_presented, problem4.goalStack, _emptySetOfInferences, now); // Here _fact_checkedIn goal shoud be removed from the stack
  problem4.worldState.removeFact(_fact_presented, problem4.goalStack, _emptySetOfInferences, now); // The difference here is that the condition was validated only punctually
  assert_eq(_action_goodBoy, _solveStr(problem4, actions));
}



void _checkMaxTimeToKeepInactiveForGoals()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));


  cp::Problem problem;
  problem.goalStack.setGoals({{10, {_fact_greeted, cp::Goal(_fact_checkedIn, 60)}}}, problem.worldState, now);
  assert_eq(_action_greet + _sep +
            _action_checkIn, _solveStr(problem, actions, now));


  cp::Problem problem2;
  problem2.goalStack.setGoals({{10, {_fact_greeted, cp::Goal(_fact_checkedIn, 60)}}}, problem2.worldState, now);
  now = std::make_unique<std::chrono::steady_clock::time_point>(*now + std::chrono::seconds(100));
  assert_eq(_action_greet, _solveStr(problem2, actions, now));
}



void _changePriorityOfGoal()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));

  std::map<int, std::vector<cp::Goal>> goalsFromSubscription;
  cp::Problem problem;
  auto onGoalsChangedConnection = problem.goalStack.onGoalsChanged.connectUnsafe([&](const std::map<int, std::vector<cp::Goal>>& pGoals) {
    goalsFromSubscription = pGoals;
  });
  cp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });

  problem.goalStack.setGoals({{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, problem.worldState, now);
  {
    auto& goals = problem.goalStack.goals();
    assert_eq(goalsFromSubscription, goals);
    assert_true(goalsRemoved.empty());
    assert_eq<std::size_t>(1, goals.find(9)->second.size());
    assert_eq<std::size_t>(2, goals.find(10)->second.size());
  }

  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, true, problem.worldState, now);
  {
    auto& goals = problem.goalStack.goals();
    assert_eq(goalsFromSubscription, goals);
    assert_true(goalsRemoved.empty());
    assert_eq<std::size_t>(2, goals.find(9)->second.size());
    assert_eq(_fact_checkedIn, goals.find(9)->second[0].toStr());
    assert_eq(_fact_userSatisfied, goals.find(9)->second[1].toStr());
    assert_eq<std::size_t>(1, goals.find(10)->second.size());
  }

  problem.goalStack.setGoals({{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, problem.worldState, now);
  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, false, problem.worldState, now);
  {
    auto& goals = problem.goalStack.goals();
    assert_eq(goalsFromSubscription, goals);
    assert_true(goalsRemoved.empty());
    assert_eq<std::size_t>(2, goals.find(9)->second.size());
    assert_eq(_fact_userSatisfied, goals.find(9)->second[0].toStr());
    assert_eq(_fact_checkedIn, goals.find(9)->second[1].toStr());
    assert_eq<std::size_t>(1, goals.find(10)->second.size());
  }

  problem.goalStack.setGoals({{10, {_fact_greeted, _fact_checkedIn}}}, problem.worldState, now);
  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, true, problem.worldState, now);
  {
    auto& goals = problem.goalStack.goals();
    assert_eq(goalsFromSubscription, goals);
    assert_eq<std::size_t>(1, goalsRemoved.size());
    assert_eq(_fact_userSatisfied, *goalsRemoved.begin());
    assert_eq<std::size_t>(1, goals.find(9)->second.size());
    assert_eq(_fact_checkedIn, goals.find(9)->second[0].toStr());
    assert_eq<std::size_t>(1, goals.find(10)->second.size());
  }
  onGoalsRemovedConnection.disconnect();
  onGoalsChangedConnection.disconnect();
}


void _factChangedNotification()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_greet, cp::Action({}, cp::WorldStateModification::fromStr(_fact_greeted)));
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn + "&" + _fact_punctual_p1)));
  cp::Domain domain(std::move(actions));

  std::set<cp::Fact> factsChangedFromSubscription;
  cp::Problem problem;
  problem.worldState.addFact(_fact_beginOfConversation, problem.goalStack, _emptySetOfInferences, now);
  auto factsChangedConnection = problem.worldState.onFactsChanged.connectUnsafe([&](const std::set<cp::Fact>& pFacts) {
    factsChangedFromSubscription = pFacts;
  });
  std::list<cp::Fact> punctualFactsAdded;
  auto onPunctualFactsConnection = problem.worldState.onPunctualFacts.connectUnsafe([&](const std::set<cp::Fact>& pFacts) {
    punctualFactsAdded.insert(punctualFactsAdded.end(), pFacts.begin(), pFacts.end());
  });
  std::list<cp::Fact> factsAdded;
  auto onFactsAddedConnection = problem.worldState.onFactsAdded.connectUnsafe([&](const std::set<cp::Fact>& pFacts) {
    factsAdded.insert(factsAdded.end(), pFacts.begin(), pFacts.end());
  });
  std::list<cp::Fact> factsRemoved;
  auto onFactsRemovedConnection = problem.worldState.onFactsRemoved.connectUnsafe([&](const std::set<cp::Fact>& pFacts) {
    factsRemoved.insert(factsRemoved.end(), pFacts.begin(), pFacts.end());
  });

  problem.goalStack.setGoals({{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, problem.worldState, now);
  assert_eq({}, factsChangedFromSubscription);

  auto plannerResult =_lookForAnActionToDoThenNotify(problem, domain);
  assert_eq<std::string>(_action_greet, plannerResult.actionInvocation.actionId);
  assert_eq({_fact_beginOfConversation, _fact_greeted}, factsChangedFromSubscription);
  assert_eq({}, punctualFactsAdded);
  assert_eq({_fact_greeted}, factsAdded);
  factsAdded.clear();
  assert_eq({}, factsRemoved);

  plannerResult =_lookForAnActionToDoThenNotify(problem, domain);
  assert_eq<std::string>(_action_checkIn, plannerResult.actionInvocation.actionId);
  assert_eq({_fact_beginOfConversation, _fact_greeted, _fact_checkedIn}, factsChangedFromSubscription);
  assert_eq({_fact_punctual_p1}, punctualFactsAdded);
  assert_eq({_fact_checkedIn}, factsAdded);
  assert_eq({}, factsRemoved);
  problem.worldState.removeFact(_fact_greeted, problem.goalStack, _emptySetOfInferences, now);
  assert_eq({_fact_beginOfConversation, _fact_checkedIn}, factsChangedFromSubscription);
  assert_eq({_fact_punctual_p1}, punctualFactsAdded);
  assert_eq({_fact_checkedIn}, factsAdded);
  assert_eq({_fact_greeted}, factsRemoved);

  onFactsRemovedConnection.disconnect();
  onPunctualFactsConnection.disconnect();
  onFactsAddedConnection.disconnect();
  factsChangedConnection.disconnect();
}




void _checkInferences()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  assert_eq<std::string>("", _solveStr(problem, domain, now));
  // Inference: if (_fact_headTouched) then remove(_fact_headTouched) and addGoal(_fact_checkedIn)
  domain.addSetOfInferences(cp::Inference(cp::Condition::fromStr(_fact_headTouched),
                                          cp::WorldStateModification::fromStr("!" + _fact_headTouched),
                                          _emptyParameters, {{{9, {_fact_checkedIn}}}}));
  assert_eq<std::string>("", _solveStr(problem, domain, now));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(_fact_headTouched, problem.goalStack, setOfInferencesMap, now);
  assert_true(!problem.worldState.hasFact(_fact_headTouched)); // removed because of the inference
  assert_eq(_action_checkIn, _solveStr(problem, domain, now));
}



void _checkInferencesWithImply()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_checkedIn)));

  // Inference: if (_fact_headTouched) then add(_fact_userWantsToCheckedIn) and remove(_fact_headTouched)
  cp::Domain domain(std::move(actions),
                    cp::Inference(cp::Condition::fromStr(_fact_headTouched),
                                  cp::WorldStateModification::fromStr(_fact_userWantsToCheckedIn + " & !" + _fact_headTouched)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("persist(imply(" + _fact_userWantsToCheckedIn + ", " + _fact_checkedIn + "))")});
  assert_eq<std::string>("", _solveStr(problem, domain, now));
  auto& setOfReferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(_fact_headTouched, problem.goalStack, setOfReferencesMap, now);
  assert_true(!problem.worldState.hasFact(_fact_headTouched)); // removed because of the inference
  assert_eq(_action_checkIn, _solveStr(problem, domain, now));
}


void _checkInferenceWithPunctualCondition()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr("!" + _fact_userWantsToCheckedIn)));

  // Inference: if (_fact_punctual_headTouched) then add(_fact_userWantsToCheckedIn)
  cp::Domain domain(std::move(actions),
                    cp::Inference(cp::Condition::fromStr(_fact_punctual_headTouched),
                                  cp::WorldStateModification::fromStr(_fact_userWantsToCheckedIn)));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("persist(!" + _fact_userWantsToCheckedIn + ")")});
  assert_eq<std::string>("", _solveStr(problem, domain, now));
  auto& setOfReferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(_fact_punctual_headTouched, problem.goalStack, setOfReferencesMap, now);
  assert_true(!problem.worldState.hasFact(_fact_punctual_headTouched)); // because it is a punctual fact
  assert_eq(_action_checkIn, _solveStr(problem, domain, now));
}


void _checkInferenceAtEndOfAPlan()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(_action_checkIn, cp::Action({}, cp::WorldStateModification::fromStr(_fact_punctual_checkedIn)));

  cp::SetOfInferences setOfInferences;
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_headTouched),
                                             cp::WorldStateModification::fromStr(_fact_userWantsToCheckedIn)));
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_checkedIn),
                                             cp::WorldStateModification::fromStr("!" + _fact_userWantsToCheckedIn)));
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("persist(!" + _fact_userWantsToCheckedIn + ")")});
  assert_eq<std::string>("", _solveStr(problem, domain, now));
  auto& setOfReferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(_fact_punctual_headTouched, problem.goalStack, setOfReferencesMap, now);
  assert_true(!problem.worldState.hasFact(_fact_punctual_headTouched)); // because it is a punctual fact
  assert_eq(_action_checkIn, _solveStr(problem, domain, now));
}


void _checkInferenceInsideAPlan()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a)));
  actions.emplace(action2, cp::Action(cp::Condition::fromStr(_fact_c), cp::WorldStateModification::fromStr(_fact_d)));
  cp::Domain domain(std::move(actions));

  {
    cp::SetOfInferences setOfInferences;
    setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_a),
                                               cp::WorldStateModification::fromStr(_fact_b)));
    setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_b + "&" + _fact_d),
                                               cp::WorldStateModification::fromStr(_fact_c)));
    domain.addSetOfInferences(std::move(setOfInferences));
  }

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d)});
  assert_eq<std::string>("", _solveStrConst(problem, domain));

  domain.addSetOfInferences(cp::Inference(cp::Condition::fromStr(_fact_b),
                                          cp::WorldStateModification::fromStr(_fact_c)));

  assert_eq(action1 + _sep + action2, _solveStrConst(problem, domain)); // check with a copy of the problem
  assert_true(!problem.worldState.hasFact(_fact_a));
  assert_true(!problem.worldState.hasFact(_fact_b));
  assert_true(!problem.worldState.hasFact(_fact_c));
  assert_true(!problem.worldState.hasFact(_fact_d));
  assert_eq(action1 + _sep + action2, _solveStr(problem, domain));
  assert_true(problem.worldState.hasFact(_fact_a));
  assert_true(problem.worldState.hasFact(_fact_b));
  assert_true(problem.worldState.hasFact(_fact_c));
  assert_true(problem.worldState.hasFact(_fact_d));
}


void _checkInferenceThatAddAGoal()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";

  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a)));
  actions.emplace(action2, cp::Action(cp::Condition::fromStr(_fact_c),
                                      cp::WorldStateModification::fromStr(_fact_d)));
  actions.emplace(action3, cp::Action(cp::Condition::fromStr(_fact_c + "&" + _fact_f),
                                      cp::WorldStateModification::fromStr(_fact_e)));
  actions.emplace(action4, cp::Action(cp::Condition::fromStr(_fact_b),
                                      cp::WorldStateModification::fromStr(_fact_f)));
  actions.emplace(action5, cp::Action(cp::Condition::fromStr(_fact_b),
                                      cp::WorldStateModification::fromStr(_fact_g)));
  cp::SetOfInferences setOfInferences;
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_a),
                                             cp::WorldStateModification::fromStr(_fact_b),
                                             _emptyParameters, {{cp::GoalStack::defaultPriority, {_fact_e}}}));
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_b),
                                             cp::WorldStateModification::fromStr(_fact_c)));
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("imply(" + _fact_g + ", " + _fact_d + ")")});
  assert_eq<std::string>("", _solveStrConst(problem, domain));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(_fact_g, problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + _sep + action4 + _sep + action3 + _sep + action2, _solveStr(problem, domain));
}



void _testQuiz()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;
  cp::ProblemModification questionEffect(cp::WorldStateModification::fromStr("add(numberOfQuestion, 1)"));
  questionEffect.potentialWorldStateModification = cp::WorldStateModification::fromStr(_fact_askAllTheQuestions);
  const cp::Action actionQ1({}, questionEffect);
  const cp::Action actionSayQuestionBilan(cp::Condition::fromStr(_fact_askAllTheQuestions),
                                          cp::WorldStateModification::fromStr(_fact_finishToAskQuestions));
  actions.emplace(_action_askQuestion1, actionQ1);
  actions.emplace(_action_askQuestion2, cp::Action({}, questionEffect));
  actions.emplace(_action_sayQuestionBilan, actionSayQuestionBilan);

  cp::Domain domain(std::move(actions),
                    cp::Inference(cp::Condition::fromStr("equals(numberOfQuestion, maxNumberOfQuestions)"),
                                  cp::WorldStateModification::fromStr(_fact_askAllTheQuestions)));

  auto initFacts = cp::WorldStateModification::fromStr("numberOfQuestion=0 & maxNumberOfQuestions=3");

  cp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_finishToAskQuestions});
  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.modify(initFacts, problem.goalStack, setOfInferencesMap, now);
  for (std::size_t i = 0; i < 3; ++i)
  {
    auto actionToDo = _lookForAnActionToDoStr(problem, domain);
    if (i == 0 || i == 2)
      assert_eq<std::string>(_action_askQuestion1, actionToDo);
    else
      assert_eq<std::string>(_action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(itAction->second.effect.worldStateModification, problem.goalStack, setOfInferencesMap, now);
  }

  auto actionToDo = _lookForAnActionToDoStr(problem, domain);
  assert_eq(_action_sayQuestionBilan, actionToDo);
}


void _testGetNotSatisfiedGoals()
{
  auto goal1 = "persist(!" + _fact_a + ")";
  auto goal2 = "persist(" + _fact_b + ")";
  auto goal3 = "imply(" + _fact_c + ", " + _fact_d + ")";
  auto goal4 = "persist(imply(!" + _fact_c + ", " + _fact_d + "))";

  cp::Problem problem;
  problem.goalStack.addGoals({goal1}, problem.worldState, {}, cp::GoalStack::defaultPriority + 1);
  problem.goalStack.addGoals({goal2, goal3, goal4}, problem.worldState, {});

  assert_eq(goal1 + ", " + goal2 + ", " + goal3 + ", " + goal4, cp::printGoals(problem.goalStack.goals()));
  assert_eq(goal2 + ", " + goal4, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.addFact(_fact_a, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(goal1 + ", " + goal2 + ", " + goal4, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.addFact(_fact_c, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(goal1 + ", " + goal2 + ", " + goal3, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.addFact(_fact_d, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(goal1 + ", " + goal2, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.removeFact(_fact_a, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(goal2, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.addFact(_fact_b, problem.goalStack, _emptySetOfInferences, {});
  assert_eq<std::string>("", cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
  problem.worldState.removeFact(_fact_d, problem.goalStack, _emptySetOfInferences, {});
  assert_eq(goal3, cp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState)));
}



void _testGoalUnderPersist()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_b)));
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_c)));
  cp::Domain domain(std::move(actions));

  {
    cp::Problem problem;
    problem.goalStack.addGoals({"persist(!" + _fact_a + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.addGoals({"persist(!" + _fact_a + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, {});
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.addGoals({"persist(!" + _fact_a + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    problem.worldState.addFact(_fact_a, problem.goalStack, _emptySetOfInferences, now);
    assert_eq<std::string>("", _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.addGoals({"persist(" + _fact_c + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    assert_eq<std::string>("", _lookForAnActionToDoStr(problem, domain));
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.addGoals({"persist(" + _fact_c + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, {});
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.addGoals({_fact_c}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, {});
    problem.goalStack.addGoals({cp::Goal(_fact_b, 0)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    cp::Problem problem;
    problem.goalStack.pushBackGoal({"persist(!" + _fact_e + ")"}, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    problem.goalStack.pushBackGoal(_fact_c, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, now);
    problem.goalStack.addGoals({cp::Goal(_fact_b, 1)}, problem.worldState, now, cp::GoalStack::defaultPriority);
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain, now));
    assert_eq(action1, _lookForAnActionToDoStr(problem, domain, now));

    problem.worldState.removeFact(_fact_c, problem.goalStack, _emptySetOfInferences, now);
    problem.goalStack.pushBackGoal(_fact_c, problem.worldState, now, cp::GoalStack::defaultPriority + 2);
    auto plannerResult = _lookForAnActionToDo(problem, domain, now);
    assert_eq(action2, plannerResult.actionInvocation.actionId);

    now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now() + std::chrono::minutes(5));
    cp::notifyActionDone(problem, domain, plannerResult, now);

    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, now);
    assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId); // Not action1 because it was inactive for too long
  }

}


void _checkLinkedInferences()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  cp::SetOfInferences setOfInferences;
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("persist(!" + _fact_userWantsToCheckedIn + ")")});
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_p2),
                                             cp::WorldStateModification::fromStr(_fact_a)));
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_p5),
                                             cp::WorldStateModification::fromStr(_fact_punctual_p2 + "&" + _fact_punctual_p3)));
  setOfInferences.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_p4),
                                             cp::WorldStateModification::fromStr(_fact_punctual_p5 + "&" + _fact_punctual_p1)));

  std::map<cp::SetOfInferencesId, cp::SetOfInferences> setOfInferencesMap = {{"soi", setOfInferences}};
  assert_false(problem.worldState.hasFact(_fact_a));
  problem.worldState.addFact(_fact_punctual_p4, problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(_fact_a));
}



void _oneStepTowards()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<cp::ActionId, cp::Action> actions;
  cp::ProblemModification greetPbModification;
  greetPbModification.potentialWorldStateModification = cp::WorldStateModification::fromStr(_fact_greeted);
  actions.emplace(_action_greet, cp::Action({}, greetPbModification));
  actions.emplace(_action_goodBoy, cp::Action({}, cp::WorldStateModification::fromStr(_fact_beHappy)));
  static const std::string actionb = "actionb";
  actions.emplace(actionb, cp::Action({}, cp::WorldStateModification::fromStr(_fact_b)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  cp::Goal implyGoal("persist(imply(" + _fact_a + ", " + _fact_b + "))", 0);
  problem.goalStack.setGoals({{11, {implyGoal}},
                      {10, {cp::Goal("oneStepTowards(" + _fact_greeted + ")", 0)}},
                      {9, {cp::Goal(_fact_checkedIn, 0), _fact_beHappy}}}, problem.worldState, now);
  problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, now);
  assert_eq(_action_greet, _lookForAnActionToDoStr(problem, domain, now));
  assert_eq(_action_greet, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
  assert_eq(_action_goodBoy, _lookForAnActionToDoStr(problem, domain, now));
  assert_eq(_action_goodBoy, _lookForAnActionToDoStr(problem, domain, now));
  problem.worldState.addFact(_fact_a, problem.goalStack, _emptySetOfInferences, now);
  assert_eq(actionb, _lookForAnActionToDoStr(problem, domain, now));
  assert_eq<std::string>(actionb + _sep + _action_goodBoy, _solveStrConst(problem, domain));
  assert_eq<std::string>(implyGoal.toStr() + _sep + _fact_beHappy, _getGoalsDoneDuringAPlannificationConst(problem, domain));
}


void _infrenceLinksFromManyInferencesSets()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  std::map<cp::ActionId, cp::Action> actions;
  cp::ProblemModification pbModification;
  pbModification.potentialWorldStateModification = cp::WorldStateModification::fromStr(_fact_d);
  actions.emplace(action1, cp::Action({}, pbModification));
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_c)));
  cp::Domain domain(std::move(actions));

  assert_true(cp::GoalStack::defaultPriority >= 1);
  auto lowPriority = cp::GoalStack::defaultPriority - 1;
  domain.addSetOfInferences(cp::Inference(cp::Condition::fromStr(_fact_punctual_p2),
                                          {}, _emptyParameters,
                                          {{lowPriority, {"oneStepTowards(" + _fact_d + ")"}}}));
  cp::Problem problem;
  problem.goalStack.setGoals({{lowPriority, {cp::Goal("oneStepTowards(" + _fact_d + ")", 0)}}}, problem.worldState, {});

  {
    cp::SetOfInferences setOfInferences2;
    setOfInferences2.addInference(cp::Inference(cp::Condition::fromStr(_fact_punctual_p1),
                                                cp::WorldStateModification::fromStr(_fact_b + "&" + _fact_punctual_p2)));
    setOfInferences2.addInference(cp::Inference(cp::Condition::fromStr(_fact_b),
                                                {}, _emptyParameters,
                                                {{cp::GoalStack::defaultPriority, {"oneStepTowards(" + _fact_c + ")"}}}));
    domain.addSetOfInferences(setOfInferences2);
  }

  auto& setOfInferencesMap = domain.getSetOfInferences();
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
  problem.worldState.addFact(_fact_punctual_p1, problem.goalStack, setOfInferencesMap, now);
  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
}


void _factValueModification()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const std::string action1 = "action1";

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr("!" + _fact_b)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.goalStack.setGoals({{10, {cp::Goal("persist(imply(" + _fact_a + "=a, " + "!" + _fact_b + "))", 0)}}}, problem.worldState, {});

  problem.worldState.addFact(_fact_b, problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>("", _lookForAnActionToDoStr(problem, domain, now));
  problem.worldState.addFact(_fact_a + "=a", problem.goalStack, _emptySetOfInferences, now);
  assert_eq(action1, _lookForAnActionToDoStr(problem, domain, now));
  problem.worldState.addFact(_fact_a + "=b", problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>("", _lookForAnActionToDoStr(problem, domain, now));
}


void _removeGoaWhenAnActionFinishesByAddingNewGoals()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  std::map<cp::ActionId, cp::Action> actions;
  cp::ProblemModification wm(cp::WorldStateModification::fromStr(_fact_a));
  wm.goalsToAddInCurrentPriority.push_back(cp::Goal(_fact_b, 0));
  actions.emplace(action1, cp::Action({}, wm));
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_b)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  cp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });
  problem.goalStack.setGoals({{27, {_fact_a}}}, problem.worldState, now);

  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
  assert_eq<std::size_t>(1u, goalsRemoved.size());
  assert_eq(_fact_a, *goalsRemoved.begin());
  onGoalsRemovedConnection.disconnect();

  // The only remaning goal is the goal we just added
  assert_eq<std::size_t>(1u, problem.goalStack.goals().size());
  assert_eq(27, problem.goalStack.goals().begin()->first);
  assert_eq<std::size_t>(1u, problem.goalStack.goals().begin()->second.size());
  assert_eq(_fact_b, problem.goalStack.goals().begin()->second.begin()->toStr());
}


void _setWsModification()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("set(location(me), location(object))"));
  actions.emplace(_action_navigate, navAction);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("location(me)=corridor"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("location(object)=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal("location(me)=kitchen")});
  assert_eq(_action_navigate, _solveStr(problem, actions));
}


void _forAllWsModification()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("forAll(obj, grab(me, obj), set(location(obj), location(me)))"));
  actions.emplace(action1, navAction);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("location(me)=corridor"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("location(object1)=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("grab(me, object1)"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("grab(me, object2)"), problem.goalStack, _emptySetOfInferences, now);

  _setGoalsForAPriority(problem, {cp::Goal("location(object2)=corridor")});
  assert_eq(action1, _solveStr(problem, actions));
}


void _actionNavigationAndGrabObjectWithParameters()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("location(me)=targetLocation"));
  navAction.parameters.emplace_back("targetLocation");
  actions.emplace(_action_navigate, navAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(location(me), location(object))"),
                        cp::WorldStateModification::fromStr("grab(me, object)"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("location(me)=corridor"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("location(sweets)=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>("kitchen", *problem.worldState.getFactFluent(cp::Fact("location(sweets)")));
  _setGoalsForAPriority(problem, {cp::Goal("grab(me, sweets)")});
  assert_eq<std::string>(_action_navigate + "(targetLocation -> kitchen), " + _action_grab + "(object -> sweets)", _solveStr(problem, actions));
}


void _actionNavigationAndGrabObjectWithParameters2()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("location(me)=targetLocation"));
  navAction.parameters.emplace_back("targetLocation");
  actions.emplace(_action_navigate, navAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(location(object), location(me))"),
                        cp::WorldStateModification::fromStr("grab(me, object)"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("location(me)=corridor"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("location(sweets)=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  assert_eq<std::string>("kitchen", *problem.worldState.getFactFluent(cp::Fact("location(sweets)")));
  _setGoalsForAPriority(problem, {cp::Goal("grab(me, sweets)")});
  assert_eq<std::string>(_action_navigate + "(targetLocation -> kitchen), " + _action_grab + "(object -> sweets)", _solveStr(problem, actions));
}



void _moveObject()
{
  const std::string actionNavigate2 = "actionNavigate2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("location(me)=targetLocation"));
  navAction.parameters.emplace_back("targetLocation");
  actions.emplace(_action_navigate, navAction);

  cp::Action navAction2(cp::Condition::fromStr("grab(me, object)"),
                        cp::WorldStateModification::fromStr("location(me)=targetLocation & location(object)=targetLocation"));
  navAction2.parameters.emplace_back("targetLocation");
  navAction2.parameters.emplace_back("object");
  actions.emplace(actionNavigate2, navAction2);

  cp::Action grabAction(cp::Condition::fromStr("equals(location(me), location(object))"),
                        cp::WorldStateModification::fromStr("grab(me, object)"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("location(me)=corridor"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact("location(sweets)=kitchen"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal("location(sweets)=bedroom")});
  assert_eq<std::string>(_action_navigate + "(targetLocation -> kitchen), " + _action_grab + "(object -> sweets), " +
                         actionNavigate2 + "(object -> sweets, targetLocation -> bedroom)", _solveStr(problem, actions));
}


void _moveAndUngrabObject()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("locationOfRobot(me)=targetLocation"));
  navAction.parameters.emplace_back("targetLocation");
  actions.emplace(_action_navigate, navAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(locationOfRobot(me), locationOfObj(object))"),
                        cp::WorldStateModification::fromStr("grab(me, object)"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Action ungrabAction({}, cp::WorldStateModification::fromStr("!grab(me, object)"));
  ungrabAction.parameters.emplace_back("object");
  actions.emplace(_action_ungrab, ungrabAction);

  cp::Problem problem;
  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr("locationOfRobot(me)=targetLocation & grab(me, object)"),
                          cp::WorldStateModification::fromStr("locationOfObj(object)=targetLocation"));
  inference.parameters.emplace_back("targetLocation");
  inference.parameters.emplace_back("object");
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(cp::Fact("locationOfObj(sweets)=kitchen"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)")});

  assert_eq(_action_navigate + "(targetLocation -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_grab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_navigate + "(targetLocation -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_ungrab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_false(problem.worldState.hasFact(cp::Fact::fromStr("locationOfObj(sweets)=kitchen")));
}


void _failToMoveAnUnknownObject()
{
  const std::string actionWhereIsObject = "actionWhereIsObject";
  const std::string actionLeavePod = "actionLeavePod";
  const std::string actionRanomWalk = "actionRanomWalk";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction(cp::Condition::fromStr("!isLost & !charging(me)"),
                       cp::WorldStateModification::fromStr("locationOfRobot(me)=targetLocation"));
  navAction.parameters.emplace_back("targetLocation");
  actions.emplace(_action_navigate, navAction);

  cp::Action randomWalkAction(cp::Condition::fromStr("!charging(me)"),
                       cp::WorldStateModification::fromStr("!isLost"));
  actions.emplace(actionRanomWalk, randomWalkAction);

  cp::Action leavePodAction({}, cp::WorldStateModification::fromStr("!charging(me)"));
  actions.emplace(actionLeavePod, leavePodAction);

  cp::Action whereIsObjectAction(cp::Condition::fromStr("!locationOfObj(object)=*"),
                                 cp::WorldStateModification::fromStr("locationOfObj(object)=aLocation"));
  whereIsObjectAction.parameters.emplace_back("object");
  whereIsObjectAction.parameters.emplace_back("aLocation");
  actions.emplace(actionWhereIsObject, whereIsObjectAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(locationOfRobot(me), locationOfObj(object))"),
                        cp::WorldStateModification::fromStr("grab(me, object)"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Action ungrabAction({}, cp::WorldStateModification::fromStr("!grab(me, object)"));
  ungrabAction.parameters.emplace_back("object");
  actions.emplace(_action_ungrab, ungrabAction);

  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr("locationOfRobot(me)=targetLocation & grab(me, object)"),
                          cp::WorldStateModification::fromStr("locationOfObj(object)=targetLocation"));
  inference.parameters.emplace_back("targetLocation");
  inference.parameters.emplace_back("object");
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)")});

  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(cp::Fact("charging(me)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(actionWhereIsObject + "(aLocation -> bedroom, object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact("locationOfObj(sweets)=kitchen"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)")});
  assert_eq(actionLeavePod, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_navigate + "(targetLocation -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_grab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_navigate + "(targetLocation -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_ungrab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _completeMovingObjectScenario()
{
  const std::string actionWhereIsObject = "actionWhereIsObject";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction(cp::Condition::fromStr("!lost(me) & !pathIsBlocked"),
                       cp::WorldStateModification::fromStr("locationOfRobot(me)=targetPlace"));
  navAction.parameters.emplace_back("targetPlace");
  actions.emplace(_action_navigate, navAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(locationOfRobot(me), locationOfObject(object)) & not(exists(o, grab(me)=o))"),
                        cp::WorldStateModification::fromStr("grab(me)=object"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Action ungrabAction({}, cp::WorldStateModification::fromStr("!grab(me)=object"));
  ungrabAction.parameters.emplace_back("object");
  actions.emplace(_action_ungrab, ungrabAction);

  cp::Action whereIsObjectAction(cp::Condition::fromStr("!locationOfObject(object)=*"),
                                 cp::WorldStateModification::fromStr("locationOfObject(object)=aLocation"));
  whereIsObjectAction.parameters.emplace_back("object");
  whereIsObjectAction.parameters.emplace_back("aLocation");
  actions.emplace(actionWhereIsObject, whereIsObjectAction);

  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr("locationOfRobot(me)=location & grab(me)=object"),
                          cp::WorldStateModification::fromStr("locationOfObject(object)=location"));
  inference.parameters.emplace_back("object");
  inference.parameters.emplace_back("location");
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObject(sweets)=bedroom & !grab(me)=sweets")});

  assert_eq(actionWhereIsObject + "(aLocation -> bedroom, object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.addFact(cp::Fact("locationOfObject(sweets)=kitchen"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObject(sweets)=bedroom & !grab(me)=sweets")});
  assert_eq(_action_navigate + "(targetPlace -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_grab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_navigate + "(targetPlace -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(_action_ungrab + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObject(bottle)=entrance & !grab(me)=bottle")});
  assert_eq(actionWhereIsObject + "(aLocation -> entrance, object -> bottle)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}

void _inferenceWithANegatedFactWithParameter()
{
  const std::string actionUngrabLeftHand = "actionUngrabLeftHand";
  const std::string actionUngrabRightHand = "actionUngrabRightHand";
  const std::string actionUngrabBothHands = "actionUngrabBothHands";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action ungrabLeftAction(cp::Condition::fromStr("!hasTwoHandles(object)"),
                              cp::WorldStateModification::fromStr("!grabLeftHand(me)=object"));
  ungrabLeftAction.parameters.emplace_back("object");
  actions.emplace(actionUngrabLeftHand, ungrabLeftAction);

  cp::Action ungrabRightAction(cp::Condition::fromStr("!hasTwoHandles(object)"),
                               cp::WorldStateModification::fromStr("!grabRightHand(me)=object"));
  ungrabRightAction.parameters.emplace_back("object");
  actions.emplace(actionUngrabRightHand, ungrabRightAction);

  cp::Action ungrabBothAction(cp::Condition::fromStr("hasTwoHandles(object)"),
                              cp::WorldStateModification::fromStr("!grabLeftHand(me)=object & !grabRightHand(me)=object"));
  ungrabBothAction.parameters.emplace_back("object");
  actions.emplace(actionUngrabBothHands, ungrabBothAction);

  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr("!grabLeftHand(me)=object & !grabRightHand(me)=object"),
                          cp::WorldStateModification::fromStr("!grab(me, object)"));
  inference.parameters.emplace_back("object");
  setOfInferences.addInference(inference);

  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("hasTwoHandles(sweets)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grabLeftHand(me)=sweets"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grabRightHand(me)=sweets"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grab(me, sweets)"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("!grab(me, sweets)")});

  assert_eq(actionUngrabBothHands + "(object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_false(problem.worldState.hasFact(cp::Fact::fromStr("grab(me, sweets)")));

  problem.worldState.addFact(cp::Fact("grabLeftHand(me)=bottle"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grab(me, bottle)"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("!grab(me, bottle)")});
  assert_eq(actionUngrabLeftHand + "(object -> bottle)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());

  problem.worldState.addFact(cp::Fact("grabLeftHand(me)=bottle"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grab(me, bottle)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grabRightHand(me)=glass"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grab(me, glass)"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal("!grab(me, glass)")});
  assert_eq(actionUngrabRightHand + "(object -> glass)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_false(problem.worldState.hasFact(cp::Fact::fromStr("grab(me, glass)")));
}


void _actionWithANegatedFactNotTriggeredIfNotNecessary()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  actions.emplace(action1, cp::Action({},
                                      cp::WorldStateModification::fromStr("!" + _fact_a),
                                      cp::Condition::fromStr(_fact_c + " & " + _fact_d)));

  actions.emplace(action2, cp::Action(cp::Condition::fromStr("!" + _fact_a + " & " + _fact_e),
                                      cp::WorldStateModification::fromStr(_fact_b)));

  actions.emplace(action3, cp::Action({},
                                      cp::WorldStateModification::fromStr(_fact_e)));

  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_c), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact(_fact_d), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

  assert_eq(action3, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _useTwoTimesAnInference()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr(_fact_a + " & " + _fact_b + "(object)"),
                          cp::WorldStateModification::fromStr(_fact_c + "(object)"));
  inference.parameters.emplace_back("object");
  setOfInferences.addInference(inference);

  std::map<std::string, cp::Action> actions;
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj1)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj2)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_a), problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(cp::Fact::fromStr(_fact_c + "(obj1)")));
  assert_true(problem.worldState.hasFact(cp::Fact::fromStr(_fact_c + "(obj2)")));
}


void _linkWithAnyValueInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  actions.emplace(action1, cp::Action(cp::Condition::fromStr("!" + _fact_a + "=*"),
                                      cp::WorldStateModification::fromStr(_fact_b)));
  cp::Action act2({}, cp::WorldStateModification::fromStr("!" + _fact_a + "=aVal"));
  act2.parameters.emplace_back("aVal");
  actions.emplace(action2, act2);

  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_a + "=toto"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
  assert_eq(action2 + "(aVal -> toto)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _removeAFactWithAnyValue()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a + " & !" + _fact_b + "=*")));
  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_b + "=toto"), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_a)});
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_false(problem.worldState.hasFact(_fact_b + "=toto"));
}



void _notDeducePathIfTheParametersOfAFactAreDifferents()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  std::map<cp::ActionId, cp::Action> actions;
  actions.emplace(action1, cp::Action({},
                                      cp::WorldStateModification::fromStr(_fact_a + "(1)"),
                                      cp::Condition::fromStr(_fact_c)));
  actions.emplace(action2, cp::Action(cp::Condition::fromStr(_fact_a + "(2)"),
                                      cp::WorldStateModification::fromStr(_fact_b)));
  actions.emplace(action3, cp::Action(cp::Condition::fromStr(_fact_b),
                                      cp::WorldStateModification::fromStr(_fact_d)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_a + "(2)"), problem.goalStack, _emptySetOfInferences, now);
  problem.worldState.addFact(cp::Fact(_fact_c), problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d)});

  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
}


void _checkPreferInContext()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a), cp::Condition::fromStr(_fact_b)));
  actions.emplace(action2, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a)));

  cp::Problem problem;
  problem.worldState.addFact(_fact_b, problem.goalStack, _emptySetOfInferences, {});
  _setGoalsForAPriority(problem, {_fact_a});
  assert_eq(action1, _solveStrConst(problem, actions, &problem.historical));
  assert_eq(action1, _solveStrConst(problem, actions, &problem.historical));
}


void _checkPreferHighImportanceOfNotRepeatingIt()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  std::map<std::string, cp::Action> actions;
  auto action1Obj = cp::Action({}, cp::WorldStateModification::fromStr(_fact_a), cp::Condition::fromStr(_fact_b));
  action1Obj.highImportanceOfNotRepeatingIt = true;
  actions.emplace(action1, action1Obj);
  auto action2Obj = cp::Action({}, cp::WorldStateModification::fromStr(_fact_a));
  action2Obj.highImportanceOfNotRepeatingIt = true;
  actions.emplace(action2, action2Obj);

  cp::Problem problem;
  problem.worldState.addFact(_fact_b, problem.goalStack, _emptySetOfInferences, {});
  _setGoalsForAPriority(problem, {_fact_a});
  assert_eq(action1, _solveStrConst(problem, actions, &problem.historical));
  assert_eq(action2, _solveStrConst(problem, actions, &problem.historical));
}


void _actionWithFactWithANegatedFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  actions.emplace(action1, cp::Action({},
                                      cp::WorldStateModification::fromStr(_fact_a + "=a")));

  actions.emplace(action2, cp::Action(cp::Condition::fromStr(_fact_a + "!=b & " + _fact_d),
                                      cp::WorldStateModification::fromStr(_fact_c)));

  actions.emplace(action3, cp::Action({},
                                      cp::WorldStateModification::fromStr(_fact_d),
                                      cp::Condition::fromStr(_fact_e)));

  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  problem.worldState.addFact(_fact_e, problem.goalStack, _emptySetOfInferences, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});

  assert_eq(action3, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _negatedFactValueInWorldState()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  actions.emplace(action1, cp::Action(cp::Condition::fromStr(_fact_a + "!=b"),
                                      cp::WorldStateModification::fromStr(_fact_b)));
  cp::Domain domain(std::move(actions));

  {
    cp::Problem problem;
    _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
    assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  }

  {
    cp::Problem problem;
    problem.worldState.addFact(_fact_a + "=b", problem.goalStack, _emptySetOfInferences, now);
    _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
    assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  }

  {
    cp::Problem problem;
    problem.worldState.addFact(_fact_a + "=c", problem.goalStack, _emptySetOfInferences, now);
    _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

    assert_eq<std::string>(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
    assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  }

  {
    cp::Problem problem;
    problem.worldState.addFact(_fact_a + "!=b", problem.goalStack, _emptySetOfInferences, now);
    _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

    assert_eq<std::string>(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
    assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  }
}


void _problemThatUseADomainThatChangedSinceLastUsage()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  actions.emplace(action1, cp::Action(cp::Condition::fromStr(_fact_a),
                                      cp::WorldStateModification::fromStr(_fact_b)));
  cp::Domain domain(std::move(actions));

  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
  assert_eq<std::string>("", _lookForAnActionToDoStr(problem, domain, now)); // set problem cache about domain

  domain.addAction(action2, cp::Action({},
                                       cp::WorldStateModification::fromStr(_fact_a)));

  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
  assert_eq<std::string>(action2, _lookForAnActionToDoStr(problem, domain, now)); // as domain as changed since last time the problem cache should be regenerated
}


void _doNextActionThatBringsToTheSmallerCost()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action navAction({}, cp::WorldStateModification::fromStr("locationOfRobot(me)=targetPlace"));
  navAction.parameters.emplace_back("targetPlace");
  actions.emplace(_action_navigate, navAction);

  cp::Action grabAction(cp::Condition::fromStr("equals(locationOfRobot(me), locationOfObject(object)) & !grab(me)=*"),
                        cp::WorldStateModification::fromStr("grab(me)=object"));
  grabAction.parameters.emplace_back("object");
  actions.emplace(_action_grab, grabAction);

  cp::Action ungrabAction({}, cp::WorldStateModification::fromStr("!grab(me)=object"));
  ungrabAction.parameters.emplace_back("object");
  actions.emplace(_action_ungrab, ungrabAction);

  cp::SetOfInferences setOfInferences;
  cp::Inference inference(cp::Condition::fromStr("locationOfRobot(me)=location & grab(me)=object & objectGrabable(object)"),
                          cp::WorldStateModification::fromStr("locationOfObject(object)=location"));
  inference.parameters.emplace_back("object");
  inference.parameters.emplace_back("location");
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("objectGrabable(obj1)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("objectGrabable(obj2)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=livingRoom"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("grab(me)=obj2"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("locationOfObject(obj2)=livingRoom"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("locationOfObject(obj1)=kitchen"), problem.goalStack, setOfInferencesMap, now);
  auto secondProblem = problem;
  // Here it will will be quicker for the second goal if we ungrab the obj2 right away
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObject(obj1)=bedroom & !grab(me)=obj1"), cp::Goal("locationOfObject(obj2)=livingRoom & !grab(me)=obj2")});
  assert_eq(_action_ungrab + "(object -> obj2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());

  // Here it will will be quicker for the second goal if we move the obj2 to the kitchen
  _setGoalsForAPriority(secondProblem, {cp::Goal("locationOfObject(obj1)=bedroom & !grab(me)=obj1"), cp::Goal("locationOfObject(obj2)=kitchen & !grab(me)=obj2")});
  assert_eq(_action_navigate + "(targetPlace -> kitchen)", _lookForAnActionToDoThenNotify(secondProblem, domain, now).actionInvocation.toStr());
}


void _checkFilterFactInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1({}, cp::WorldStateModification::fromStr(_fact_a + "(obj)"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr(_fact_b + "(obj, loc) & " + _fact_a + "(obj)"),
                        cp::WorldStateModification::fromStr(_fact_c));
  actionObj2.parameters.emplace_back("obj");
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);
  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj1, loc1)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj1, loc2)"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});
  assert_eq<std::string>(action1 + "(obj -> obj1)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>(action2 + "(loc -> loc1, obj -> obj1)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _checkFilterFactInConditionAndThenPropagate()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1({}, cp::WorldStateModification::fromStr(_fact_a + "=obj"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr(_fact_b + "(obj, loc) & " + _fact_a + "=obj"),
                        cp::WorldStateModification::fromStr(_fact_c + "(loc)"));
  actionObj2.parameters.emplace_back("obj");
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);


  cp::Action actionObj3(cp::Condition::fromStr(_fact_c + "(loc)"),
                        cp::WorldStateModification::fromStr(_fact_d + "(loc)"));
  actionObj3.parameters.emplace_back("loc");
  actions.emplace(action3, actionObj3);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj1, loc1)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(obj2, loc2)"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d + "(loc2)")});
  assert_eq(action1 + "(obj -> obj2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2 + "(loc -> loc2, obj -> obj2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action3 + "(loc -> loc2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _satisfyGoalWithSuperiorOperator()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a + "=100")));

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_a + "=10"), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_a + ">50")});
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _checkOutputValueOfLookForAnActionToDo()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a)));

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_a)});

  {
    cp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = cp::planForMoreImportantGoalPossible(problem, domain, true, now, nullptr, &lookForAnActionOutputInfos);
    assert(!res.empty());
    assert_eq(cp::PlannerStepType::IN_PROGRESS, lookForAnActionOutputInfos.getType());
    assert_eq<std::size_t>(0, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }

  {
    problem.worldState.addFact(cp::Fact(_fact_a), problem.goalStack, setOfInferencesMap, now);
    cp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = cp::planForMoreImportantGoalPossible(problem, domain, true, now, nullptr, &lookForAnActionOutputInfos);
    assert(res.empty());
    assert_eq(cp::PlannerStepType::FINISHED_ON_SUCCESS, lookForAnActionOutputInfos.getType());
    assert_eq<std::size_t>(1, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }

  {
    _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
    cp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = cp::planForMoreImportantGoalPossible(problem, domain, true, now, nullptr, &lookForAnActionOutputInfos);
    assert(res.empty());
    assert_eq(cp::PlannerStepType::FINISEHD_ON_FAILURE, lookForAnActionOutputInfos.getType());
    assert_eq<std::size_t>(0, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }
}


void _hardProbleThatNeedsToBeSmart()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";
  const std::string action6 = "action6";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1({}, cp::WorldStateModification::fromStr(_fact_a + "=obj"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr(_fact_a + "=val1"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action2, actionObj2);

  cp::Action actionObj3(cp::Condition::fromStr(_fact_a + "=val2"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action3, actionObj3);

  cp::Action actionObj4(cp::Condition::fromStr(_fact_a + "=val3 & !" + _fact_c),
                        cp::WorldStateModification::fromStr("!" + _fact_d + " & " + _fact_f));
  actions.emplace(action4, actionObj4);

  cp::Action actionObj5(cp::Condition::fromStr(_fact_a + "=val4"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action5, actionObj5);

  cp::Action actionObj6(cp::Condition::fromStr(_fact_b + " & !" + _fact_d),
                        cp::WorldStateModification::fromStr(_fact_e));
  actions.emplace(action6, actionObj6);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_d), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_e)});

  assert_eq(action1 + "(obj -> val3)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action4, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action1 + "(obj -> val1)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action6, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _goalsToDoInParallel()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";
  const std::string action6 = "action6";
  const std::string action7 = "action7";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1({}, cp::WorldStateModification::fromStr(_fact_a + "=obj"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr(_fact_a + "=val1"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action2, actionObj2);

  cp::Action actionObj3(cp::Condition::fromStr(_fact_a + "=val2"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action3, actionObj3);

  cp::Action actionObj4(cp::Condition::fromStr(_fact_a + "=val3 & !" + _fact_c),
                        cp::WorldStateModification::fromStr("!" + _fact_d + " & " + _fact_f));
  actions.emplace(action4, actionObj4);

  cp::Action actionObj5(cp::Condition::fromStr(_fact_a + "=val4"),
                        cp::WorldStateModification::fromStr(_fact_b + " & " + _fact_c));
  actions.emplace(action5, actionObj5);

  cp::Action actionObj6(cp::Condition::fromStr(_fact_b + " & !" + _fact_d + " & " + _fact_g),
                        cp::WorldStateModification::fromStr(_fact_e));
  actions.emplace(action6, actionObj6);

  cp::Action actionObj7(cp::Condition::fromStr(_fact_f),
                        cp::WorldStateModification::fromStr(_fact_g));
  actions.emplace(action7, actionObj7);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_d), problem.goalStack, setOfInferencesMap, now);
  _setGoalsForAPriority(problem, {cp::Goal(_fact_e)});

  assert_eq(action1 + "(obj -> val3)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());

  assert_eq<std::string>(action1 + "(obj -> val3)", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_eq<std::string>(action4, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_eq<std::string>(action7 + _sep + action1 + "(obj -> val1)", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_eq<std::string>(action2, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_eq<std::string>(action6, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_eq<std::string>("", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
}



void _checkOverallEffectDuringParallelisation()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("!" + _fact_d),
                        cp::WorldStateModification::fromStr(_fact_a + " & !" + _fact_d));
  actionObj1.effect.worldStateModificationAtStart = cp::WorldStateModification::fromStr(_fact_d + " & " + _fact_e);
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr("!" + _fact_d),
                        cp::WorldStateModification::fromStr(_fact_b + " & !" + _fact_d));
  actionObj2.effect.worldStateModificationAtStart = cp::WorldStateModification::fromStr(_fact_d);
  actions.emplace(action2, actionObj2);

  cp::Action actionObj3(cp::Condition::fromStr(_fact_a + " & " + _fact_b),
                        cp::WorldStateModification::fromStr(_fact_c));
  actions.emplace(action3, actionObj3);

  cp::Domain domain(std::move(actions));
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});

  assert_eq<std::string>(action1, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, now));
  assert_true(problem.worldState.hasFact(_fact_e));
}

void _checkSimpleExists()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(l))"),
                        cp::WorldStateModification::fromStr(_fact_b));
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "(kitchen)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1, _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _checkExistsWithActionParameterInvolved()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(l, obj))"),
                        cp::WorldStateModification::fromStr(_fact_b));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "(kitchen, pen)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(obj -> pen)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _checkExistsWithManyFactsInvolved()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(self, l) & " + _fact_b + "(obj, l))"),
                        cp::WorldStateModification::fromStr(_fact_c));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});

  problem.worldState.addFact(cp::Fact(_fact_b + "(mouse, bedroom)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(bottle, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(pen, livingroom)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "(self, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(obj -> bottle)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _doAnActionToSatisfyAnExists()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(self, l) & " + _fact_b + "(obj, l))"),
                        cp::WorldStateModification::fromStr(_fact_c + "(obj)"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2({}, cp::WorldStateModification::fromStr(_fact_a + "(self, loc)"));
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c + "(mouse)")});

  problem.worldState.addFact(cp::Fact(_fact_b + "(bottle, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(mouse, bedroom)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(pen, livingroom)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action2 + "(loc -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action1 + "(obj -> mouse)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _checkForAllEffectAtStart()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(self, l) & " + _fact_b + "(obj, l))"),
                        cp::WorldStateModification::fromStr(_fact_c + "(obj)"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2({}, cp::WorldStateModification::fromStr(_fact_a + "(self, loc)"));
  actionObj2.effect.worldStateModificationAtStart = cp::WorldStateModification::fromStr("forall(l, when(" + _fact_a + "(self, l), not(" + _fact_a + "(self, l))))");
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c + "(mouse)")});

  problem.worldState.addFact(cp::Fact(_fact_a + "(self, entrance)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(bottle, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(mouse, bedroom)"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(pen, livingroom)"), problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_a + "(self, entrance)")));
  assert_eq(action2 + "(loc -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_a + "(self, entrance)"))); // removed by the effect at start of action2
  assert_eq(action1 + "(obj -> mouse)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _existsWithValue()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("exists(l, " + _fact_a + "(self)=l & " + _fact_b + "(obj)=l)"),
                        cp::WorldStateModification::fromStr(_fact_c + "(obj)"));
  actionObj1.parameters.emplace_back("obj");
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2({}, cp::WorldStateModification::fromStr(_fact_a + "(self)=loc"));
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c + "(pen)")});

  problem.worldState.addFact(cp::Fact(_fact_b + "(pen)=livingroom"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action2 + "(loc -> livingroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action1 + "(obj -> pen)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _notExists()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("not(exists(l, " + _fact_a + "(self, l)))"),
                        cp::WorldStateModification::fromStr(_fact_b));
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});

  assert_eq(action1, _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "(self, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq<std::string>("", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _actionToSatisfyANotExists()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr("not(busy(spec_rec)) & not(exists(l, " + _fact_a + "(self, l)))"),
                        cp::WorldStateModification::fromStr("not(busy(spec_rec)) & " + _fact_b));
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2({}, cp::WorldStateModification::fromStr("!" + _fact_a + "(self, loc)"));
  actionObj2.parameters.emplace_back("loc");
  actions.emplace(action2, actionObj2);

  cp::Action actionObj3({}, cp::WorldStateModification::fromStr("not(busy(r))"));
  actionObj3.parameters.emplace_back("r");
  actions.emplace(action3, actionObj3);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_b)});
  problem.worldState.addFact(cp::Fact("busy(spec_rec)"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action3 + "(r -> spec_rec)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "(self, kitchen)"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action2 + "(loc -> kitchen)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _orInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::Action actionObj1(cp::Condition::fromStr(_fact_a + " & " + _fact_b),
                        cp::WorldStateModification::fromStr(_fact_c));
  actions.emplace(action1, actionObj1);

  cp::Action actionObj2(cp::Condition::fromStr(_fact_a + " | " + _fact_b),
                        cp::WorldStateModification::fromStr(_fact_c));
  actions.emplace(action2, actionObj2);

  cp::Action actionObj3(cp::Condition::fromStr(_fact_a + " & " + _fact_b),
                        cp::WorldStateModification::fromStr(_fact_c));
  actions.emplace(action3, actionObj3);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});
  problem.worldState.addFact(cp::Fact(_fact_a), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _derivedPredicate()
{
  std::unique_ptr<std::chrono::steady_clock::time_point> now = {};
  std::map<std::string, cp::Action> actions;

  cp::SetOfInferences setOfInferences;
  cp::DerivedPredicate derivedPredicate1(cp::Condition::fromStr(_fact_a + "(?o)" + " & " + _fact_b + "(?o)"), _fact_c + "(?o)", {"?o"});
  for (auto& currInference : derivedPredicate1.toInferences())
    setOfInferences.addInference(currInference);

  cp::DerivedPredicate derivedPredicate2(cp::Condition::fromStr(_fact_a + "(?o)" + " | " + _fact_b + "(?o)"), _fact_d + "(?o)", {"?o"});
  for (auto& currInference : derivedPredicate2.toInferences())
    setOfInferences.addInference(currInference);

  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact(_fact_e), problem.goalStack, setOfInferencesMap, now);

  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(book)")));
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_d + "(book)")));
  problem.worldState.addFact(cp::Fact(_fact_a + "(book)"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(book)")));
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_d + "(book)")));
  problem.worldState.addFact(cp::Fact(_fact_b + "(book)"), problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_c + "(book)")));
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_d + "(book)")));
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(titi)")));
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_d + "(titi)")));
  problem.worldState.removeFact(cp::Fact(_fact_a + "(book)"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(book)")));
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_d + "(book)")));
  problem.worldState.addFact(cp::Fact(_fact_a + "(titi)"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(book)")));
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_d + "(book)")));
  assert_false(problem.worldState.hasFact(cp::Fact(_fact_c + "(titi)")));
  assert_true(problem.worldState.hasFact(cp::Fact(_fact_d + "(titi)")));
}


void _assignAnotherValueToSatisfyNotGoal()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr(_fact_a + "=toto")));

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("!" + _fact_a + "=titi")});

  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "=titi"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _assignUndefined()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", undefined)")));

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("!" + _fact_a + "=titi")});

  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_a + "=titi"), problem.goalStack, setOfInferencesMap, now);
  assert_eq<std::size_t>(1u, problem.worldState.facts().size());
  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::size_t>(0u, problem.worldState.facts().size()); // because assign undefined is done like a fact removal
}


void _assignAFact()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_a + "=valGoal")});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _assignAFactToAction()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);
  actions.emplace(action2, cp::Action(cp::Condition::fromStr(_fact_a + "=valGoal"),
                                      cp::WorldStateModification::fromStr(_fact_c)));

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_c)});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}


void _assignAFactThenCheckEqualityWithAnotherFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);
  cp::Action action2Obj(cp::Condition::fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))"),
                        cp::WorldStateModification::fromStr(_fact_d));
  action2Obj.parameters.emplace_back("?pc");
  actions.emplace(action2, action2Obj);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d)});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _assignAFactThenCheckExistWithAnotherFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);
  cp::Action action2Obj(cp::Condition::fromStr("exists(pc, =(" + _fact_a + ", " + _fact_c + "(pc)))"),
                        cp::WorldStateModification::fromStr(_fact_d));
  actions.emplace(action2, action2Obj);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d)});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _existWithEqualityInInference()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj(cp::Condition::fromStr(_fact_b + "(?o)"),
                        cp::WorldStateModification::fromStr(_fact_d + "(?o)"));
  action1Obj.parameters.emplace_back("?o");
  actions.emplace(action1, action1Obj);

  cp::SetOfInferences setOfInferences;
  cp::DerivedPredicate derivedPredicate1(cp::Condition::fromStr("exists(?pc, =(" + _fact_c + "(?pc), " + _fact_a + "(?o)))"), _fact_b + "(?o)", {"?o"});
  for (auto& currInference : derivedPredicate1.toInferences())
    setOfInferences.addInference(currInference);


  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d + "(p2)")});
  problem.worldState.addFact(cp::Fact(_fact_a + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_a + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_a + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.removeFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.removeFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d + "(p1)")});
  assert_eq(action1 + "(?o -> p1)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
}


void _existWithEqualityInInference_withEqualityInverted()
{
  const std::string action1 = "action1";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj(cp::Condition::fromStr(_fact_b + "(?o)"),
                        cp::WorldStateModification::fromStr(_fact_d + "(?o)"));
  action1Obj.parameters.emplace_back("?o");
  actions.emplace(action1, action1Obj);

  cp::SetOfInferences setOfInferences;
  cp::DerivedPredicate derivedPredicate1(cp::Condition::fromStr("exists(?pc, =(" + _fact_a + "(?o), " + _fact_c + "(?pc)))"), _fact_b + "(?o)", {"?o"});
  for (auto& currInference : derivedPredicate1.toInferences())
    setOfInferences.addInference(currInference);


  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d + "(p2)")});
  problem.worldState.addFact(cp::Fact(_fact_a + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_a + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_a + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.removeFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.removeFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  assert_eq<std::string>("", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  assert_eq(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d + "(p1)")});
  assert_eq(action1 + "(?o -> p1)", _lookForAnActionToDoConst(problem, domain, now).actionInvocation.toStr());
}



void _fixInferenceWithFluentInParameter()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + "(titi), " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);
  cp::Action action2Obj(cp::Condition::fromStr("=(" + _fact_e + "(titi), " + _fact_c + "(?pc))"),
                        cp::WorldStateModification::fromStr(_fact_d));
  action2Obj.parameters.emplace_back("?pc");
  actions.emplace(action2, action2Obj);

  cp::SetOfInferences setOfInferences;
  cp::DerivedPredicate derivedPredicate1(cp::Condition::fromStr(_fact_a + "(?a)=?v"), _fact_e + "(?a)=?v", {"?a", "?v"});
  for (auto& currInference : derivedPredicate1.toInferences())
    setOfInferences.addInference(currInference);


  cp::Domain domain(std::move(actions), std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_d)});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, now).actionInvocation.toStr());
}



void _assignAFactTwoTimesInTheSamePlan()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");

  actions.emplace(action1, action1Obj);
  cp::Action action2Obj(cp::Condition::fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))"),
                        cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_d + "(?pc))"));
  action2Obj.parameters.emplace_back("?pc");
  actions.emplace(action2, action2Obj);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_a + "=kitchen")});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_d + "(pc2)=kitchen"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}


void _checkTwoTimesTheEqualityOfAFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;
  cp::Action action1Obj({}, cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))"));
  action1Obj.parameters.emplace_back("?p");
  actions.emplace(action1, action1Obj);

  cp::Action action2Obj(cp::Condition::fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))"),
                        cp::WorldStateModification::fromStr("assign(" + _fact_a + ", " + _fact_d + "(?pc))"));
  action2Obj.parameters.emplace_back("?pc");
  actions.emplace(action2, action2Obj);

  cp::Action action3Obj(cp::Condition::fromStr("=(" + _fact_a + ", " + _fact_e + "(?tt))"),
                        cp::WorldStateModification::fromStr("assign(" + _fact_f + ", ?tt)"));
  action3Obj.parameters.emplace_back("?tt");
  actions.emplace(action3, action3Obj);

  cp::Domain domain(std::move(actions));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal(_fact_f + "=bedroom")});
  problem.worldState.addFact(cp::Fact(_fact_b + "(p1)=aVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_b + "(p3)=anotherVal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc1)=v1"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc2)=valGoal"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_c + "(pc3)=v3"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_d + "(pc2)=kitchen"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact(_fact_e + "(bedroom)=kitchen"), problem.goalStack, setOfInferencesMap, now);

  assert_eq(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq(action3 + "(?tt -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.toStr());
}



void _inferenceToRemoveAFactWithoutFluent()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, cp::Action> actions;

  cp::SetOfInferences setOfInferences;
  cp::Inference inference1(cp::Condition::fromStr("locationOfRobot(me)=?l"),
                          cp::WorldStateModification::fromStr("robotAt(me, ?l)"));
  inference1.parameters.emplace_back("?l");
  setOfInferences.addInference(inference1);

  cp::Inference inference2(cp::Condition::fromStr("exists(loc, locationOfRobot(me)=loc & within(loc)=?l)"),
                          cp::WorldStateModification::fromStr("robotAt(me, ?l)"));
  inference2.parameters.emplace_back("?l");
  setOfInferences.addInference(inference2);

  cp::Inference inference(cp::Condition::fromStr("!locationOfRobot(me)=?l"),
                          cp::WorldStateModification::fromStr("forall(ll, robotAt(me, ll), !robotAt(me, ll))"));

  inference.parameters.emplace_back("?l");
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), std::move(setOfInferences));

  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("within(house1)=city"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("within(house2)=city"), problem.goalStack, setOfInferencesMap, now);
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=house1"), problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(cp::Fact("robotAt(me, house1)")));
  assert_true(problem.worldState.hasFact(cp::Fact("robotAt(me, city)")));
  problem.worldState.removeFact(cp::Fact("locationOfRobot(me)=house1"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact("robotAt(me, house1)")));
  assert_false(problem.worldState.hasFact(cp::Fact("robotAt(me, city)")));
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=house1"), problem.goalStack, setOfInferencesMap, now);
  assert_true(problem.worldState.hasFact(cp::Fact("robotAt(me, house1)")));
  assert_true(problem.worldState.hasFact(cp::Fact("robotAt(me, city)")));
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=city"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact("robotAt(me, house1)")));
  assert_true(problem.worldState.hasFact(cp::Fact("robotAt(me, city)")));
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=anotherCity"), problem.goalStack, setOfInferencesMap, now);
  assert_false(problem.worldState.hasFact(cp::Fact("robotAt(me, house1)")));
  assert_false(problem.worldState.hasFact(cp::Fact("robotAt(me, city)")));
}



}




int main(int argc, char *argv[])
{
  test_arithmeticEvaluator();
  test_ontology();
  test_util();
  planningDummyExample();
  planningExampleWithAPreconditionSolve();
  _test_createEmptyGoal();
  _test_goalToStr();
  _test_factToStr();
  _test_conditionParameters();
  _test_wsModificationToStr();
  _test_invertCondition();
  _test_checkCondition();
  _automaticallyRemoveGoalsWithAMaxTimeToKeepInactiveEqualTo0();
  _maxTimeToKeepInactiveEqualTo0UnderAnAlreadySatisfiedGoal();
  _noPreconditionGoalImmediatlyReached();
  _removeGoalWhenItIsSatisfiedByAnAction();
  _removeAnAction();
  _removeSomeGoals();
  _notifyGoalRemovedWhenItIsImmediatlyRemoved();
  _handlePreconditionWithNegatedFacts();
  _testWithNegatedAccessibleFacts();
  _noPlanWithALengthOf2();
  _noPlanWithALengthOf3();
  _2preconditions();
  _2Goals();
  _2UnrelatedGoals();
  _impossibleGoal();
  _privigelizeTheActionsThatHaveManyPreferedInContext();
  _preconditionThatCannotBeSolved();
  _preferInContext();
  _preferWhenPreconditionAreCloserToTheRealFacts();
  _avoidToDo2TimesTheSameActionIfPossble();
  _takeHistoricalIntoAccount();
  _goDoTheActionThatHaveTheMostPreferInContextValidated();
  _checkNotInAPrecondition();
  _checkClearGoalsWhenItsAlreadySatisfied();
  _checkActionHasAFact();
  _checkActionReplacefact();
  _testIncrementOfVariables();
  _precoditionEqualEffect();
  _addGoalEvenForEmptyAction();
  _circularDependencies();
  _triggerActionThatRemoveAFact();
  _actionWithConstantValue();
  _actionWithParameterizedValue();
  _actionWithParameterizedParameter();
  _actionWithParametersInPreconditionsAndEffects();
  _actionWithParametersInPreconditionsAndEffectsWithoutSolution();
  _actionWithParametersInsideThePath();
  _testPersistGoal();
  _testPersistImplyGoal();
  _testImplyGoal();
  _checkPreviousBugAboutSelectingAnInappropriateAction();
  _dontLinkActionWithPreferredInContext();
  _checkPriorities();
  _stackablePropertyOfGoals();
  _doNotRemoveAGoalWithMaxTimeToKeepInactiveEqual0BelowAGoalWithACondotionNotSatisfied();
  _checkMaxTimeToKeepInactiveForGoals();
  _changePriorityOfGoal();
  _factChangedNotification();
  _checkInferences();
  _checkInferencesWithImply();
  _checkInferenceWithPunctualCondition();
  _checkInferenceAtEndOfAPlan();
  _checkInferenceInsideAPlan();
  _checkInferenceThatAddAGoal();
  _testQuiz();
  _testGetNotSatisfiedGoals();
  _testGoalUnderPersist();
  _checkLinkedInferences();
  _oneStepTowards();
  _infrenceLinksFromManyInferencesSets();
  _factValueModification();
  _removeGoaWhenAnActionFinishesByAddingNewGoals();
  _setWsModification();
  _forAllWsModification();
  _actionNavigationAndGrabObjectWithParameters();
  _actionNavigationAndGrabObjectWithParameters2();
  _moveObject();
  _moveAndUngrabObject();
  _failToMoveAnUnknownObject();
  _completeMovingObjectScenario();
  _inferenceWithANegatedFactWithParameter();
  _actionWithANegatedFactNotTriggeredIfNotNecessary();
  _useTwoTimesAnInference();
  _linkWithAnyValueInCondition();
  _removeAFactWithAnyValue();
  _notDeducePathIfTheParametersOfAFactAreDifferents();
  _checkPreferInContext();
  _checkPreferHighImportanceOfNotRepeatingIt();
  _actionWithFactWithANegatedFact();
  _negatedFactValueInWorldState();
  _problemThatUseADomainThatChangedSinceLastUsage();
  _doNextActionThatBringsToTheSmallerCost();
  _checkFilterFactInCondition();
  _checkFilterFactInConditionAndThenPropagate();
  _satisfyGoalWithSuperiorOperator();
  _checkOutputValueOfLookForAnActionToDo();
  _hardProbleThatNeedsToBeSmart();
  _goalsToDoInParallel();
  _checkOverallEffectDuringParallelisation();
  _checkSimpleExists();
  _checkExistsWithActionParameterInvolved();
  _checkExistsWithManyFactsInvolved();
  _doAnActionToSatisfyAnExists();
  _checkForAllEffectAtStart();
  _existsWithValue();
  _notExists();
  _actionToSatisfyANotExists();
  _orInCondition();
  _derivedPredicate();
  _assignAnotherValueToSatisfyNotGoal();
  _assignUndefined();
  _assignAFact();
  _assignAFactToAction();
  _assignAFactThenCheckEqualityWithAnotherFact();
  _assignAFactThenCheckExistWithAnotherFact();
  _existWithEqualityInInference();
  _existWithEqualityInInference_withEqualityInverted();
  _fixInferenceWithFluentInParameter();
  _assignAFactTwoTimesInTheSamePlan();
  _checkTwoTimesTheEqualityOfAFact();
  _inferenceToRemoveAFactWithoutFluent();

  std::cout << "chatbot planner is ok !!!!" << std::endl;
  return 0;
}
