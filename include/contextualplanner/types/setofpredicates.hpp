#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFPREDICATES_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFPREDICATES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <string>
#include "predicate.hpp"

namespace cp
{
struct SetOfTypes;


struct CONTEXTUALPLANNER_API SetOfPredicates
{
  SetOfPredicates();

  static SetOfPredicates fromStr(const std::string& pStr,
                                 const SetOfTypes& pSetOfTypes);

  void addAll(const SetOfPredicates& pOther);
  void addPredicate(const Predicate& pPredicate);

  const Predicate* nameToPredicatePtr(const std::string& pName) const;
  Predicate nameToPredicate(const std::string& pName) const;

  std::string toStr() const;

private:
  std::map<std::string, Predicate> _nameToPredicate;
};

} // namespace cp

#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFPREDICATES_HPP
