#include "test_arithmeticevaluator.hpp"
#include <assert.h>
#include <iostream>
#include <contextualplanner/types/setoftypes.hpp>

namespace
{
template <typename TYPE>
void assert_eq(const TYPE& pExpected,
               const TYPE& pValue)
{
  if (pExpected != pValue)
    assert(false);
}


void _test_setOfTypes()
{
  cp::SetOfTypes setOfTypes;
  assert_eq<std::string>("", setOfTypes.typesToStr());
  setOfTypes.addType("object");
  setOfTypes.addType("voiture", "object");
  setOfTypes.addType("maison", "object");
  setOfTypes.addType("citroen", "voiture");
  setOfTypes.addType("ferrari", "voiture");
  setOfTypes.addType("peugeot", "voiture");
  setOfTypes.addType("c3", "citroen");
  setOfTypes.addType("location");

  assert_eq<std::string>("voiture maison - object\n"
                         "citroen ferrari peugeot - voiture\n"
                         "c3 - citroen\n"
                         "location",
                         setOfTypes.typesToStr());
}


void _test_setOfTypes_fromStr()
{
  std::string typesStr = "voiture maison - object\n"
                         "citroen ferrari peugeot - voiture\n"
                         "c3 - citroen\n"
                         "location";
  auto setOfTypes = cp::SetOfTypes::fromStr(typesStr + " ");
  assert_eq<std::string>(typesStr, setOfTypes.typesToStr());
}


void _test_setOfTypes_fromStr()
{
  auto setOfTypes = cp::SetOfTypes::fromStr("my_type");
}


void _test_predicateToStr()
{
  assert_eq<std::string>("pred_name(my_type)", cp::Predicate("pred_name(?v - my_type)").toStr());
  assert_eq<std::string>("pred_name(my_type, my_type2)", cp::Predicate("pred_name(?v - my_type, ?o - my_type2)").toStr());
  assert_eq<std::string>("pred_name(my_type, my_type2) - return_type", cp::Predicate("pred_name(?v - my_type, ?o - my_type2) - return_type").toStr());
}

}




void test_ontology()
{
  _test_setOfTypes();
  _test_setOfTypes_fromStr();
  _test_predicateToStr();

  std::cout << "ontology is ok !!!!" << std::endl;
}
