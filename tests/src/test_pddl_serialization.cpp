#include "test_pddl_serialization.hpp"
#include <assert.h>
#include <iostream>
#include <contextualplanner/types/domain.hpp>
#include <contextualplanner/types/ontology.hpp>
#include <contextualplanner/types/predicate.hpp>
#include <contextualplanner/types/setofentities.hpp>
#include <contextualplanner/types/setofpredicates.hpp>
#include <contextualplanner/types/worldstatemodification.hpp>
#include <contextualplanner/util/serializer/deserializefrompddl.hpp>
#include <contextualplanner/util/serializer/serializeinpddl.hpp>

namespace
{
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


void _test_pddlSerializationParts()
{
  cp::Ontology ontology;
  ontology.types = cp::SetOfTypes::fromStr("type1 type2 - entity");
  ontology.constants = cp::SetOfEntities::fromStr("toto - type1\n"
                                                  "titi - type2", ontology.types);

  cp::Predicate pred("(pred_a ?e - entity)", true, ontology.types);
  assert_eq<std::string>("pred_a(?e - entity)", pred.toStr());

  {
    std::size_t pos = 0;
    ontology.predicates = cp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                        "pred_b\n"
                                                        "(battery-amount ?t - type1) - number", pos, ontology.types);
    assert_eq<std::string>("battery-amount(?t - type1) - number\n"
                           "pred_a(?e - entity)\n"
                           "pred_b()", ontology.predicates.toStr());
  }

  {
    std::size_t pos = 0;
    cp::Fact fact = cp::Fact::fromPddl("(pred_a toto)", ontology, {}, {}, pos, &pos);
    assert_eq<std::string>("(pred_a toto)", fact.toPddl(true));
    assert_eq<std::string>("pred_a(toto)", fact.toStr());
  }

  {
    std::size_t pos = 0;
    std::unique_ptr<cp::WorldStateModification> ws = cp::pddlToWsModification("(decrease (battery-amount toto) 4)", pos, ontology, {}, {});
    if (!ws)
      assert_true(false);
    assert_eq<std::string>("decrease(battery-amount(toto), 4)", ws->toStr());
  }
}


void _test_loadPddlDomain()
{
  auto domain = cp::pddlToDomain(R"(
(define
    (domain construction)
    (:extends building)
    (:requirements :strips :typing)
    (:types
        site material - object
        bricks cables windows - material
        car
        ball
    )
    (:constants mainsite - site
      maincar - car)

    ;(:domain-variables ) ;deprecated

    (:predicates
        (walls-built ?s - site)
        (windows-fitted ?s - site)
        (foundations-set ?s - site)
        (cables-installed ?s - site)
        (site-built ?s - site)
        (on-site ?m - material ?s - site)
        (material-used ?m - material)
        (held ?b - ball)
    )

    (:functions
        (battery-amount ?r - car)
        (distance-to-floor ?b - ball)
        (velocity ?b - ball)
    )

    (:timeless (foundations-set mainsite))

    ;(:safety
        ;(forall
        ;    (?s - site) (walls-built ?s)))
        ;deprecated

    (:axiom
        :vars (?s - site)
        :context (and
            (walls-built ?s)
            (windows-fitted ?s)
            (cables-installed ?s)
        )
        :implies (site-built ?s)
    )

    (:event HIT-GROUND
        :parameters (?b - ball)
        :precondition (and
            (not (held ?b))
            (<= (distance-to-floor ?b) 0)
            (> (velocity ?b) 0)
        )
        :effect (
            (assign (velocity ?b) (* -0.8 (velocity ?b)))
        )
    )

    (:action BUILD-WALL
        :parameters (?s - site ?b - bricks)
        :precondition (and
            (on-site ?b ?s)
            (foundations-set ?s)
            (not (walls-built ?s))
            (not (material-used ?b))
        )
        :effect (and
            (walls-built ?s)
            (material-used ?b)
        )
        ; :expansion ;deprecated
    )

    (:durative-action BUILD-WALL-DURATIVE
        :parameters
          (?s - site ?b - bricks)

        :duration
            (= ?duration 1)

        :condition
          (and
            (at start (on-site ?b ?s))
            (at start (foundations-set ?s))
            (over all (not (walls-built ?s)))
            (at start (not (material-used ?b)))
          )

        :effect
          (and
            (at start (walls-built ?s))
            (at end (material-used ?b))
            (at end (decrease (battery-amount maincar) 4))
          )
    )

)
)");

  auto pddout1 = cp::domainToPddl(domain);
  std::cout << pddout1 << std::endl;
  assert_eq<std::string>(R"((define
    (domain construction)

    (:types
        site material - object
        bricks cables windows - material
        car
        ball
    )

    (:constants
        maincar - car
        mainsite - site
    )

    (:predicates
        (cables-installed ?s - site)
        (foundations-set ?s - site)
        (held ?b - ball)
        (material-used ?m - material)
        (on-site ?m - material ?s - site)
        (site-built ?s - site)
        (walls-built ?s - site)
        (windows-fitted ?s - site)
    )

    (:functions
        (battery-amount ?r - car) - number
        (distance-to-floor ?b - ball) - number
        (velocity ?b - ball) - number
    )

    (:event
))", pddout1);
}

}




void test_pddlSerialization()
{
  _test_pddlSerializationParts();
  _test_loadPddlDomain();

  std::cout << "PDDL serialization is ok !!!!" << std::endl;
}
