#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFDERIVEDPREDICATES_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFDERIVEDPREDICATES_HPP

#include "../util/api.hpp"
#include <map>
#include <memory>
#include <string>
#include <orderedgoalsplanner/types/derivedpredicate.hpp>

namespace ogp
{
struct Condition;
struct Predicate;


struct ORDEREDGOALSPLANNER_API SetOfDerivedPredicates
{
  SetOfDerivedPredicates();

  void addAll(const SetOfDerivedPredicates& pOther);
  void addDerivedPredicate(const DerivedPredicate& pDerivedPredicate);

  const Predicate* nameToPredicatePtr(const std::string& pPredicateName) const;

  std::unique_ptr<Condition> optFactToConditionPtr(const FactOptional& pFactOptional) const;


private:
  std::map<std::string, DerivedPredicate> _nameToDerivedPredicate;
};

} // namespace ogp

#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFDERIVEDPREDICATES_HPP
