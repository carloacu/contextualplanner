#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFENTITIES_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFENTITIES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <string>
#include "entity.hpp"

namespace cp
{
struct SetOfTypes;


struct CONTEXTUALPLANNER_API SetOfEntities
{
  SetOfEntities();

  static SetOfEntities fromStr(const std::string& pStr,
                               const SetOfTypes& pSetOfTypes);

  void add(const Entity& pEntity);

  const Entity* valueToEntity(const std::string& pValue) const;

  std::string toStr(std::size_t pIdentation = 0) const;

  bool empty() const { return _valueToEntity.empty(); }

private:
  std::map<std::string, Entity> _valueToEntity;
};

} // namespace cp

#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFENTITIES_HPP
