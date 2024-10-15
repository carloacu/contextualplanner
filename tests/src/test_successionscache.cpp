#include "test_successionscache.hpp"
#include <contextualplanner/types/action.hpp>
#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/setoffacts.hpp>

using namespace cp;

namespace
{
template <typename TYPE>
void assert_eq(const TYPE& pExpected,
               const TYPE& pValue)
{
  if (pExpected != pValue)
    assert(false);
}

void _test_actionSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";

  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("e1 e2 - entity\n"
                                           "resource");
  ontology.constants = cp::SetOfEntities::fromStr("a - entity\n"
                                                  "moves - resource", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b(?e - entity)\n"
                                                     "fact_c\n"
                                                     "fact_d\n"
                                                     "fact_e\n"
                                                     "fact_f\n"
                                                     "locked(?r - resource)",
                                                     ontology.types);

  {
    std::vector<cp::Parameter> parameters1(1, cp::Parameter::fromStr("?e - e1", ontology.types));
    cp::Action actionObj1(cp::Condition::fromStr("not(locked(moves)) & fact_f", ontology, {}, parameters1),
                          cp::WorldStateModification::fromStr("not(locked(moves)) & fact_f & not(fact_a) & fact_b(?e)", ontology, {}, parameters1));
    actionObj1.parameters = std::move(parameters1);
    actions.emplace(action1, actionObj1);
  }

  {
    std::vector<cp::Parameter> parameters2(1, cp::Parameter::fromStr("?e - e1", ontology.types));
    cp::Action actionObj2(cp::Condition::fromStr("not(locked(moves)) & fact_b(?e) & fact_f", ontology, {}, parameters2),
                          cp::WorldStateModification::fromStr("not(locked(moves)) & fact_c", ontology, {}, parameters2));
    actionObj2.parameters = std::move(parameters2);
    actions.emplace(action2, actionObj2);
  }

  {
    std::vector<cp::Parameter> parameters3(1, cp::Parameter::fromStr("?e - e2", ontology.types));
    cp::Action actionObj3(cp::Condition::fromStr("not(locked(moves)) & fact_b(?e)", ontology, {}, parameters3),
                          cp::WorldStateModification::fromStr("not(locked(moves)) & fact_d", ontology, {}, parameters3));
    actionObj3.parameters = std::move(parameters3);
    actions.emplace(action3, actionObj3);
  }

  {
    cp::Action actionObj4(cp::Condition::fromStr("not(locked(moves)) & not(fact_a)", ontology, {}, {}),
                          cp::WorldStateModification::fromStr("not(locked(moves)) & fact_e", ontology, {}, {}));
    actions.emplace(action4, actionObj4);
  }

  {
    std::vector<cp::Parameter> parameters5(1, cp::Parameter::fromStr("?e - entity", ontology.types));
    cp::Action actionObj5(cp::Condition::fromStr("not(locked(moves)) & fact_b(?e)", ontology, {}, parameters5),
                          cp::WorldStateModification::fromStr("not(locked(moves)) & fact_d", ontology, {}, parameters5));
    actionObj5.parameters = std::move(parameters5);
    actions.emplace(action5, actionObj5);
  }

  Domain domain(actions, ontology);

  assert_eq<std::string>("action: action1\n"
                         "----------------------------------\n"
                         "\n"
                         "fact: !fact_a\n"
                         "action: action4\n"
                         "\n"
                         "fact: fact_b(?e)\n"
                         "action: action2\n"
                         "action: action5\n"
                         "\n"
                         "\n"
                         "action: action3\n"
                         "----------------------------------\n"
                         "\n"
                         "not action: action5\n"
                         "\n"
                         "\n"
                         "action: action5\n"
                         "----------------------------------\n"
                         "\n"
                         "not action: action3\n", domain.printSuccessionCache());
}


void _test_notActionSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  std::map<std::string, cp::Action> actions;

  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("entity\n"
                                           "location");
  ontology.constants = cp::SetOfEntities::fromStr("e1 e2 - entity", ontology.types);
  ontology.predicates = cp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b(?e - entity)\n"
                                                     "fact_c(?e - entity) - location\n"
                                                     "fact_d - location",
                                                     ontology.types);

  {
    cp::Action actionObj1({},
                          cp::WorldStateModification::fromStr("fact_a & assign(fact_c(e1), fact_d())", ontology, {}, {}));
    actions.emplace(action1, actionObj1);
  }

  {
    std::vector<cp::Parameter> parameters2{
      cp::Parameter::fromStr("?e - entity", ontology.types),
      cp::Parameter::fromStr("?l - location", ontology.types)};
    cp::Action actionObj2(cp::Condition::fromStr("fact_a", ontology, {}, parameters2),
                          cp::WorldStateModification::fromStr("fact_a & fact_b(?e) & fact_c(e1)=?l", ontology, {}, parameters2));
    actionObj2.parameters = std::move(parameters2);
    actions.emplace(action2, actionObj2);
  }

  Domain domain(actions, ontology);
  assert_eq<std::string>("action: action1\n"
                         "----------------------------------\n"
                         "\n"
                         "not action: action2\n", domain.printSuccessionCache());
}

void _test_impossibleSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  std::map<std::string, cp::Action> actions;
  cp::Ontology ontology;
  cp::SetOfConstFacts timelessFacts;
  ontology.predicates = cp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b\n"
                                                     "fact_c\n"
                                                     "fact_d",
                                                     ontology.types);
  timelessFacts.add(cp::Fact("fact_d", false, ontology, {}, {}));

  {
    cp::Action actionObj1({},
                          cp::WorldStateModification::fromStr("fact_a & fact_b", ontology, {}, {}));
    actions.emplace(action1, actionObj1);
  }

  {
    cp::Action actionObj2(cp::Condition::fromStr("fact_a & not(fact_b)", ontology, {}, {}),
                          cp::WorldStateModification::fromStr("fact_c", ontology, {}, {}));
    actions.emplace(action2, actionObj2);
  }

  // Do not consider action that can never be true

  {
    cp::Action actionObj3(cp::Condition::fromStr("fact_a & !fact_d", ontology, {}, {}),
                          cp::WorldStateModification::fromStr("fact_c", ontology, {}, {}));
    actions.emplace(action3, actionObj3);
  }

  Domain domain(actions, ontology, {}, timelessFacts);
  assert_eq<std::string>("action: action1\n"
                         "----------------------------------\n"
                         "\n"
                         "not action: action2\n", domain.printSuccessionCache());
}



}

void test_successionsCache()
{
  _test_actionSuccessions();
  _test_notActionSuccessions();
  _test_impossibleSuccessions();
}
