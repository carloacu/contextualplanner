#include <contextualplanner/util/serializer/serializeinpddl.hpp>
#include <contextualplanner/types/domain.hpp>

namespace cp
{
namespace
{
std::size_t _identationOffset = 4;

std::string _parametersToPddl(const std::vector<Parameter>& pParameters)
{
  std::string res = "(";
  bool firstIteraction = true;
  for (const auto& currParam : pParameters)
  {
    if (firstIteraction)
      firstIteraction = false;
    else
      res +=  " ";
    res += currParam.toStr();
  }
  return res + ")";
}


}


std::string domainToPddl(const Domain& pDomain)
{
  std::string res = "(define\n";

  std::size_t identation = _identationOffset;


  res += std::string(identation, ' ') + "(domain " + pDomain.getName() + ")\n\n";

  const auto& ontology = pDomain.getOntology();

  if (!ontology.types.empty())
  {
    res += std::string(identation, ' ') + "(:types\n";
    res += ontology.types.toStr(_identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (!ontology.constants.empty())
  {
    res += std::string(identation, ' ') + "(:constants\n";
    res += ontology.constants.toStr(_identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (ontology.predicates.hasPredicateOfPddlType(PredicatePddlType::PDDL_PREDICATE))
  {
    res += std::string(identation, ' ') + "(:predicates\n";
    res += ontology.predicates.toPddl(PredicatePddlType::PDDL_PREDICATE, _identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (ontology.predicates.hasPredicateOfPddlType(PredicatePddlType::PDDL_FUNCTION))
  {
    res += std::string(identation, ' ') + "(:functions\n";
    res += ontology.predicates.toPddl(PredicatePddlType::PDDL_FUNCTION, _identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  auto setOfEvents = pDomain.getSetOfEvents();
  if (!setOfEvents.empty())
  {
    for (const auto& currSetOfEvent : setOfEvents)
    {
      for (const auto& currEventIdToEvent : currSetOfEvent.second.events())
      {
        const Event& currEvent = currEventIdToEvent.second;
        res += std::string(identation, ' ') + "(:event ";
        if (setOfEvents.size() == 1)
          res += currEventIdToEvent.first + "\n";
        else
          res += currSetOfEvent.first + "-" + currEventIdToEvent.first + "\n";
        res += "\n";
        std::size_t subIdentation = identation + _identationOffset;
        std::size_t subSubIdentation = subIdentation + _identationOffset;

        std::string eventContent;
        if (!currEvent.parameters.empty())
        {
          eventContent += std::string(subIdentation, ' ') + ":parameters\n";
          eventContent += std::string(subSubIdentation, ' ') + _parametersToPddl(currEvent.parameters) + "\n";
        }

        if (currEvent.precondition)
        {
          if (!eventContent.empty())
            eventContent += "\n";
          eventContent += std::string(subIdentation, ' ') + ":precondition\n";
          eventContent += std::string(subSubIdentation, ' ') +
              currEvent.precondition->toPddl(subSubIdentation, _identationOffset) + "\n";
        }

        if (currEvent.factsToModify)
        {
          if (!eventContent.empty())
            eventContent += "\n";
          eventContent += std::string(subIdentation, ' ') + ":effect\n";
          eventContent += std::string(subSubIdentation, ' ') + "\n";
        }

        res += eventContent;
        res += "\n" + std::string(identation, ' ') + ")\n\n";
      }
    }
  }

  return res + ")";
}

}
