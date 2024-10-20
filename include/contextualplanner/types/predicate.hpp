#ifndef INCLUDE_CONTEXTUALPLANNER_PREDICATE_HPP
#define INCLUDE_CONTEXTUALPLANNER_PREDICATE_HPP

#include <optional>
#include <string>
#include <vector>
#include "../util/api.hpp"
#include "parameter.hpp"


namespace cp
{

struct CONTEXTUALPLANNER_API Predicate
{
  Predicate(const std::string& pStr,
            bool pStrPddlFormated,
            const SetOfTypes& pSetOfTypes,
            std::size_t pBeginPos = 0,
            std::size_t* pResPos = nullptr);

  std::string toPddl() const;
  std::string toStr() const;

  bool operator==(const Predicate& pOther) const;
  bool operator!=(const Predicate& pOther) const { return !operator==(pOther); }

  /// Name of the predicate.
  std::string name;
  /// Argument types of the predicate.
  std::vector<Parameter> parameters;
  /// Fluent type of the predicate.
  std::shared_ptr<Type> fluent;
};

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_PREDICATE_HPP
