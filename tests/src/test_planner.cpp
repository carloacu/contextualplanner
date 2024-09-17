#include <contextualplanner/contextualplanner.hpp>
#include <contextualplanner/types/derivedpredicate.hpp>
#include <contextualplanner/types/predicate.hpp>
#include <contextualplanner/types/setofinferences.hpp>
#include <contextualplanner/util/trackers/goalsremovedtracker.hpp>
#include <contextualplanner/util/print.hpp>
#include <contextualplanner/util/util.hpp>
#include <iostream>
#include <assert.h>
#include "test_arithmeticevaluator.hpp"
#include "test_facttoconditions.hpp"
#include "test_ontology.hpp"
#include "test_setoffacts.hpp"
#include "test_successionscache.hpp"
#include "test_util.hpp"
#include "docexamples/test_planningDummyExample.hpp"
#include "docexamples/test_planningExampleWithAPreconditionSolve.hpp"
#include "test_plannerWithSingleType.hpp"


namespace
{
const auto _now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
const std::map<cp::SetOfInferencesId, cp::SetOfInferences> _emptySetOfInferences;
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



void _simplest_plan_possible()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("type1 type2 - entity");
  ontology.constants = cp::SetOfEntities::fromStr("toto - type1\n"
                                                  "titi - type2", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a(?e - entity)\n"
                                                     "pred_b\n", ontology.types);

  const cp::SetOfEntities entities;

  std::vector<cp::Parameter> parameters(1, cp::Parameter::fromStr("?pa - type1", ontology.types));
  cp::Action actionObj1(cp::Condition::fromStr("pred_a(?pa)", ontology, entities, parameters),
                        cp::WorldStateModification::fromStr("pred_b", ontology, entities, parameters));
  actionObj1.parameters = std::move(parameters);
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_b", ontology, entities)});
  problem.worldState.addFact(cp::Fact("pred_a(toto)", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);

  assert_eq<std::string>("action1(?pa -> toto)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}



void _wrong_condition_type()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity\n"
                                           "type1 - entity\n"
                                           "type2 - entity");
  ontology.constants = cp::SetOfEntities::fromStr("toto - type1\n"
                                                  "titi - type2", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a(?e - entity)\n"
                                                     "pred_b\n", ontology.types);

  const cp::SetOfEntities entities;

  std::vector<cp::Parameter> parameters(1, cp::Parameter::fromStr("?pa - type1", ontology.types));
  cp::Action actionObj1(cp::Condition::fromStr("pred_a(?pa)", ontology, entities, parameters),
                        cp::WorldStateModification::fromStr("pred_b", ontology, entities, parameters));
  actionObj1.parameters = std::move(parameters);
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_b", ontology, entities)});
  problem.worldState.addFact(cp::Fact("pred_a(titi)", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);

  assert_eq<std::string>("", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _number_type()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity");
  ontology.constants = cp::SetOfEntities::fromStr("toto - entity", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a(?e - entity) - number\n"
                                                     "pred_b", ontology.types);

  const cp::SetOfEntities entities;

  cp::Action actionObj1(cp::Condition::fromStr("pred_a(toto)=10", ontology, entities, {}),
                        cp::WorldStateModification::fromStr("pred_b", ontology, entities, {}));
  actions.emplace(action1, actionObj1);

  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_b", ontology, entities)});
  assert_eq<std::string>("", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
  problem.worldState.addFact(cp::Fact("pred_a(toto)=10", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);

  _setGoalsForAPriority(problem, {cp::Goal("pred_b", ontology, entities)});
  assert_eq<std::string>(action1, _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _planWithActionThenInferenceWithFluentParameter()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity");
  ontology.constants = cp::SetOfEntities::fromStr("toto titi - entity", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a - entity\n"
                                                     "pred_b(?e - entity)", ontology.types);

  const cp::SetOfEntities entities;

  cp::Action actionObj1({},
                        cp::WorldStateModification::fromStr("pred_a=toto", ontology, entities, {}));
  actions.emplace(action1, actionObj1);

  cp::SetOfInferences setOfInferences;
  std::vector<cp::Parameter> inferenceParameters{cp::Parameter::fromStr("?e - entity", ontology.types)};
  cp::Inference inference(cp::Condition::fromStr("pred_a=?e", ontology, entities, inferenceParameters),
                          cp::WorldStateModification::fromStr("pred_b(?e)", ontology, entities, inferenceParameters));
  inference.parameters = std::move(inferenceParameters);
  setOfInferences.addInference(inference);

  cp::Domain domain(std::move(actions), {}, std::move(setOfInferences));
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_b(toto)", ontology, entities)});
 assert_eq<std::string>(action1, _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _planWithActionThenInferenceWithAssign()
{
  const std::string action1 = "action1";
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity\n"
                                           "other_type");
  ontology.constants = cp::SetOfEntities::fromStr("toto titi - entity\n"
                                                  "v - other_type", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a - other_type\n"
                                                     "pred_b(?e - entity) - other_type\n"
                                                     "pred_c - other_type\n"
                                                     "pred_d - other_type", ontology.types);

  const cp::SetOfEntities entities;

  std::vector<cp::Parameter> actionParameters{cp::Parameter::fromStr("?e - entity", ontology.types)};
  cp::Action actionObj1({},
                        cp::WorldStateModification::fromStr("assign(pred_a, pred_b(?e))", ontology, entities, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  cp::SetOfInferences setOfInferences;
  std::vector<cp::Parameter> inferenceParameters{cp::Parameter::fromStr("?t - other_type", ontology.types)};
  cp::Inference inference(cp::Condition::fromStr("pred_a=?t", ontology, entities, inferenceParameters),
                          cp::WorldStateModification::fromStr("pred_d=?t", ontology, entities, inferenceParameters));
  inference.parameters = std::move(inferenceParameters);
  setOfInferences.addInference(inference);

  cp::Domain domain(std::move(actions), {}, std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_d=v", ontology, entities)});
  problem.worldState.addFact(cp::Fact("pred_b(toto)=v", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  assert_eq<std::string>(action1 + "(?e -> toto)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _fluentEqualityInPrecoditionOfAnAction()
{
  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity\n"
                                           "other_type\n"
                                           "lol");
  ontology.constants = cp::SetOfEntities::fromStr("toto titi - entity\n"
                                                  "v - other_type\n"
                                                  "lol_val - lol", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("pred_a - other_type\n"
                                                     "pred_b(?e - entity) - other_type\n"
                                                     "pred_c(?l - lol) - other_type\n"
                                                     "pred_d(?l - lol)", ontology.types);
  const cp::SetOfEntities entities;

  const std::string action1 = "action1";
  std::vector<cp::Parameter> actionParameters{cp::Parameter::fromStr("?e - entity", ontology.types)};
  cp::Action actionObj1({},
                        cp::WorldStateModification::fromStr("assign(pred_a, pred_b(?e))", ontology, entities, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  const std::string action2 = "action2";
  std::vector<cp::Parameter> action2Parameters{cp::Parameter::fromStr("?l - lol", ontology.types)};
  cp::Action actionObj2(cp::Condition::fromStr("=(pred_a, pred_c(?l))", ontology, entities, action2Parameters),
                        cp::WorldStateModification::fromStr("pred_d(?l)", ontology, entities, action2Parameters));
  actionObj2.parameters = std::move(action2Parameters);
  actions.emplace(action2, actionObj2);

  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();
  cp::Problem problem;
  _setGoalsForAPriority(problem, {cp::Goal("pred_d(lol_val)", ontology, entities)});
  problem.worldState.addFact(cp::Fact("pred_b(toto)=v", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("pred_c(lol_val)=v", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  assert_eq<std::string>(action1 + "(?e -> toto)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _testIncrementOfVariables()
{
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("");
  ontology.constants = cp::SetOfEntities::fromStr("", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("numberOfQuestion - number\n"
                                                     "maxNumberOfQuestions - number\n"
                                                     "ask_all_the_questions\n"
                                                     "finished_to_ask_questions", ontology.types);
  const cp::SetOfEntities entities;

  const std::string action_askQuestion1 = "ask_question_1";
  const std::string action_askQuestion2 = "ask_question_2";
  const std::string action_finisehdToAskQuestions = "finish_to_ask_questions";
  const std::string action_sayQuestionBilan = "say_question_bilan";

  std::map<std::string, cp::Action> actions;
  const cp::Action actionQ1({}, cp::WorldStateModification::fromStr("ask_all_the_questions & add(numberOfQuestion, 1)", ontology, entities, {}));
  const cp::Action actionFinishToActActions(cp::Condition::fromStr("equals(numberOfQuestion, maxNumberOfQuestions)", ontology, entities, {}),
                                            cp::WorldStateModification::fromStr("ask_all_the_questions", ontology, entities, {}));
  const cp::Action actionSayQuestionBilan(cp::Condition::fromStr("ask_all_the_questions", ontology, entities, {}),
                                          cp::WorldStateModification::fromStr("finished_to_ask_questions", ontology, entities, {}));
  actions.emplace(action_askQuestion1, actionQ1);
  actions.emplace(action_askQuestion2, cp::Action({}, cp::WorldStateModification::fromStr("ask_all_the_questions & add(numberOfQuestion, 1)", ontology, entities, {})));
  actions.emplace(action_finisehdToAskQuestions, actionFinishToActActions);
  actions.emplace(action_sayQuestionBilan, actionSayQuestionBilan);
  cp::Domain domain(std::move(actions), ontology);

  std::string initFactsStr = "numberOfQuestion=0 & maxNumberOfQuestions=3";
  cp::Problem problem;
  problem.worldState.modify(cp::WorldStateModification::fromStr(initFactsStr, ontology, entities, {}), problem.goalStack, _emptySetOfInferences, ontology, entities, _now);
  assert(cp::Condition::fromStr(initFactsStr, ontology, entities, {})->isTrue(problem.worldState));
  assert(!actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  assert(cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 3)", ontology, entities, {})->isTrue(problem.worldState));
  assert(!cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 4)", ontology, entities, {})->isTrue(problem.worldState));
  assert(cp::Condition::fromStr("equals(maxNumberOfQuestions, numberOfQuestion + 4 - 1)", ontology, entities, {})->isTrue(problem.worldState));
  for (std::size_t i = 0; i < 3; ++i)
  {
    _setGoalsForAPriority(problem, {cp::Goal("finished_to_ask_questions", ontology, entities)});
    auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
    if (i == 0 || i == 2)
      assert_eq<std::string>(action_askQuestion1, actionToDo);
    else
      assert_eq<std::string>(action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(itAction->second.effect.worldStateModification, problem.goalStack,
                              _emptySetOfInferences, ontology, entities, _now);
    problem.worldState.modify(cp::WorldStateModification::fromStr("!ask_all_the_questions", ontology, entities, {}),
                              problem.goalStack, _emptySetOfInferences, ontology, entities, _now);
  }
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  _setGoalsForAPriority(problem, {cp::Goal("finished_to_ask_questions", ontology, entities)});
  auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
  assert_eq<std::string>(action_finisehdToAskQuestions, actionToDo);
  problem.historical.notifyActionDone(actionToDo);
  auto itAction = domain.actions().find(actionToDo);
  assert(itAction != domain.actions().end());
  problem.worldState.modify(itAction->second.effect.worldStateModification, problem.goalStack,
                            _emptySetOfInferences, ontology, entities, _now);
  assert_eq<std::string>(action_sayQuestionBilan, _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState));
  assert(actionSayQuestionBilan.precondition->isTrue(problem.worldState));
  problem.worldState.modify(actionSayQuestionBilan.effect.worldStateModification, problem.goalStack,
                            _emptySetOfInferences, ontology, entities, _now);
}


void _actionWithParametersInPreconditionsAndEffects()
{
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("");
  ontology.constants = cp::SetOfEntities::fromStr("", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("isEngaged(?hid - number)\n"
                                                     "isHappy(?hid - number)", ontology.types);
  const cp::SetOfEntities entities;

  std::map<std::string, cp::Action> actions;
  std::vector<cp::Parameter> parameters(1, cp::Parameter::fromStr("?human - number", ontology.types));
  cp::Action joke(cp::Condition::fromStr("isEngaged(?human)", ontology, entities, parameters),
                  cp::WorldStateModification::fromStr("isHappy(?human)", ontology, entities, parameters));
  joke.parameters = std::move(parameters);
  const std::string action1 = "action1";
  actions.emplace(action1, joke);

  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  problem.worldState.addFact(cp::Fact("isEngaged(1)", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);

  _setGoalsForAPriority(problem, {cp::Goal("isHappy(1)", ontology, entities)});
  assert_eq(action1 + "(?human -> 1)", cp::planToStr(cp::planForEveryGoals(problem, domain, _now)));
}


void _testQuiz()
{
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("");
  ontology.constants = cp::SetOfEntities::fromStr("", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("numberOfQuestion - number\n"
                                                     "maxNumberOfQuestions - number\n"
                                                     "ask_all_the_questions\n"
                                                     "finished_to_ask_questions", ontology.types);
  const cp::SetOfEntities entities;

  const std::string action_askQuestion1 = "ask_question_1";
  const std::string action_askQuestion2 = "ask_question_2";
  const std::string action_sayQuestionBilan = "say_question_bilan";

  std::map<std::string, cp::Action> actions;
  cp::ProblemModification questionEffect(cp::WorldStateModification::fromStr("add(numberOfQuestion, 1)", ontology, entities, {}));
  questionEffect.potentialWorldStateModification = cp::WorldStateModification::fromStr("ask_all_the_questions", ontology, entities, {});
  const cp::Action actionQ1({}, questionEffect);
  const cp::Action actionSayQuestionBilan(cp::Condition::fromStr("ask_all_the_questions", ontology, entities, {}),
                                          cp::WorldStateModification::fromStr("finished_to_ask_questions", ontology, entities, {}));
  actions.emplace(action_askQuestion1, actionQ1);
  actions.emplace(action_askQuestion2, cp::Action({}, questionEffect));
  actions.emplace(action_sayQuestionBilan, actionSayQuestionBilan);

  cp::Domain domain(std::move(actions), {},
                    cp::Inference(cp::Condition::fromStr("equals(numberOfQuestion, maxNumberOfQuestions)", ontology, entities, {}),
                                  cp::WorldStateModification::fromStr("ask_all_the_questions", ontology, entities, {})));

  auto initFacts = cp::WorldStateModification::fromStr("numberOfQuestion=0 & maxNumberOfQuestions=3", ontology, entities, {});

  cp::Problem problem;

  _setGoalsForAPriority(problem, {cp::Goal("finished_to_ask_questions", ontology, entities)});
  auto& setOfInferencesMap = domain.getSetOfInferences();
  problem.worldState.modify(initFacts, problem.goalStack, setOfInferencesMap, {}, {}, _now);
  for (std::size_t i = 0; i < 3; ++i)
  {
    auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
    if (i == 0 || i == 2)
      assert_eq<std::string>(action_askQuestion1, actionToDo);
    else
      assert_eq<std::string>(action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(itAction->second.effect.worldStateModification,
                              problem.goalStack, setOfInferencesMap, {}, {}, _now);
  }

  auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
  assert_eq(action_sayQuestionBilan, actionToDo);
}



void _doNextActionThatBringsToTheSmallerCost()
{
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("location\n"
                                           "object\n"
                                           "robot");
  ontology.constants = cp::SetOfEntities::fromStr("me - robot", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("objectGrabable(?o - object)\n"
                                                     "locationOfRobot(?r - robot) - location\n"
                                                     "locationOfObject(?o - object) - location\n"
                                                     "grab(?r - robot) - object", ontology.types);
  const std::string action_navigate = "navigate";
  const std::string action_grab = "grab";
  const std::string action_ungrab = "ungrab";

  std::map<std::string, cp::Action> actions;
  std::vector<cp::Parameter> navParameters{cp::Parameter::fromStr("?targetPlace - location", ontology.types)};
  cp::Action navAction({}, cp::WorldStateModification::fromStr("locationOfRobot(me)=?targetPlace", ontology, {}, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(action_navigate, navAction);

  std::vector<cp::Parameter> grabParameters{cp::Parameter::fromStr("?object - object", ontology.types)};
  cp::Action grabAction(cp::Condition::fromStr("equals(locationOfRobot(me), locationOfObject(?object)) & !grab(me)=*", ontology, {}, grabParameters),
                        cp::WorldStateModification::fromStr("grab(me)=?object", ontology, {}, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(action_grab, grabAction);

  std::vector<cp::Parameter> ungrabParameters{cp::Parameter::fromStr("?object - object", ontology.types)};
  cp::Action ungrabAction({}, cp::WorldStateModification::fromStr("!grab(me)=?object", ontology, {}, ungrabParameters));
  ungrabAction.parameters = std::move(ungrabParameters);
  actions.emplace(action_ungrab, ungrabAction);

  cp::SetOfInferences setOfInferences;
  std::vector<cp::Parameter> inferenceParameters{cp::Parameter::fromStr("?object - object", ontology.types), cp::Parameter::fromStr("?location - location", ontology.types)};
  cp::Inference inference(cp::Condition::fromStr("locationOfRobot(me)=?location & grab(me)=?object & objectGrabable(?object)", ontology, {}, inferenceParameters),
                          cp::WorldStateModification::fromStr("locationOfObject(?object)=?location", ontology, {}, inferenceParameters));
  inference.parameters = std::move(inferenceParameters);
  setOfInferences.addInference(inference);
  cp::Domain domain(std::move(actions), {}, std::move(setOfInferences));
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  auto& entities = problem.entities;
  entities = cp::SetOfEntities::fromStr("obj1 obj2 - object\n"
                                        "livingRoom kitchen bedroom - location", ontology.types);
  problem.worldState.addFact(cp::Fact("objectGrabable(obj1)", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("objectGrabable(obj2)", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("locationOfRobot(me)=livingRoom", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("grab(me)=obj2", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("locationOfObject(obj2)=livingRoom", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("locationOfObject(obj1)=kitchen", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  auto secondProblem = problem;
  // Here it will will be quicker for the second goal if we ungrab the obj2 right away
  _setGoalsForAPriority(problem, {cp::Goal("locationOfObject(obj1)=bedroom & !grab(me)=obj1", ontology, entities),
                                  cp::Goal("locationOfObject(obj2)=livingRoom & !grab(me)=obj2", ontology, entities)});
  assert_eq(action_ungrab + "(?object -> obj2)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());

  // Here it will will be quicker for the second goal if we move the obj2 to the kitchen
  _setGoalsForAPriority(secondProblem, {cp::Goal("locationOfObject(obj1)=bedroom & !grab(me)=obj1", ontology, entities),
                                        cp::Goal("locationOfObject(obj2)=kitchen & !grab(me)=obj2", ontology, entities)});
  assert_eq(action_navigate + "(?targetPlace -> kitchen)", _lookForAnActionToDo(secondProblem, domain, _now).actionInvocation.toStr());
}


void _satisfyGoalWithSuperiorOperator()
{
  const std::string action1 = "action1";
  cp::Ontology ontology;
  ontology.predicates = cp::SetOfPredicates::fromStr("fact_a - number", ontology.types);

  std::map<std::string, cp::Action> actions;
  actions.emplace(action1, cp::Action({}, cp::WorldStateModification::fromStr("fact_a=100", ontology, {}, {})));
  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  auto& entities = problem.entities;
  problem.worldState.addFact(cp::Fact("fact_a=10", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  _setGoalsForAPriority(problem, {cp::Goal("fact_a>50", ontology, entities)});

  assert_eq(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  assert_eq<std::string>("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _parameterToFillFromConditionOfFirstAction()
{
  const std::string action1 = "action1";
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("location\n"
                                           "chargingZone");
  ontology.constants = cp::SetOfEntities::fromStr("cz - chargingZone\n"
                                                  "czLocation - location", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("locationOfRobot - location\n"
                                                     "declaredLocationOfChargingZone(?cz - chargingZone) - location\n"
                                                     "batteryLevel - number", ontology.types);

  std::map<std::string, cp::Action> actions;
  std::vector<cp::Parameter> actionParameters{cp::Parameter::fromStr("?cz - chargingZone", ontology.types)};
  cp::Action action1Obj(cp::Condition::fromStr("=(locationOfRobot, declaredLocationOfChargingZone(?cz))", ontology, {}, actionParameters),
                                        cp::WorldStateModification::fromStr("batteryLevel=100", ontology, {}, actionParameters));
  action1Obj.parameters = std::move(actionParameters);
  actions.emplace(action1, action1Obj);
  cp::Domain domain(std::move(actions), ontology);
  auto& setOfInferencesMap = domain.getSetOfInferences();

  cp::Problem problem;
  auto& entities = problem.entities;
  problem.worldState.addFact(cp::Fact("locationOfRobot=czLocation", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("declaredLocationOfChargingZone(cz)=czLocation", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  problem.worldState.addFact(cp::Fact("batteryLevel=40", ontology, entities, {}), problem.goalStack, setOfInferencesMap,
                             ontology, entities, _now);
  _setGoalsForAPriority(problem, {cp::Goal("batteryLevel=100", ontology, entities)});

  assert_eq(action1 + "(?cz -> cz)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


}




int main(int argc, char *argv[])
{
  cp::CONTEXTUALPLANNER_DEBUG_FOR_TESTS = true;
  test_arithmeticEvaluator();
  test_factToConditions();
  test_setOfFacts();
  test_ontology();
  test_successionsCache();
  test_util();
  planningDummyExample();
  planningExampleWithAPreconditionSolve();

  _simplest_plan_possible();
  _wrong_condition_type();
  _number_type();
  _planWithActionThenInferenceWithFluentParameter();
  _planWithActionThenInferenceWithAssign();
  _fluentEqualityInPrecoditionOfAnAction();
  _testIncrementOfVariables();
  _actionWithParametersInPreconditionsAndEffects();
  _testQuiz();
  _doNextActionThatBringsToTheSmallerCost();
  _satisfyGoalWithSuperiorOperator();
  _parameterToFillFromConditionOfFirstAction();

  test_planWithSingleType();
  std::cout << "chatbot planner is ok !!!!" << std::endl;
  return 0;
}
