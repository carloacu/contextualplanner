cmake_minimum_required(VERSION 2.8)

project(contextualplanner_tests)

include_directories(".")

if(NOT CONTEXTUALPLANNER_FOUND)
  find_package(contextualplanner)
endif()


add_executable(
  contextualplanner_tests

  src/test_planner.cpp
  src/test_facttoconditions.hpp
  src/test_facttoconditions.cpp
  src/test_plannerWithSingleType.hpp
  src/test_plannerWithSingleType.cpp
  src/test_ontology.hpp
  src/test_ontology.cpp
  src/test_setoffacts.hpp
  src/test_setoffacts.cpp
  src/test_arithmeticevaluator.hpp
  src/test_arithmeticevaluator.cpp
  src/test_successionscache.hpp
  src/test_successionscache.cpp
  src/docexamples/test_planningDummyExample.hpp
  src/docexamples/test_planningDummyExample.cpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.hpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.cpp
  src/test_util.hpp
  src/test_util.cpp
)


target_link_libraries(contextualplanner_tests
  ${CONTEXTUALPLANNER_LIBRARIES}
)
