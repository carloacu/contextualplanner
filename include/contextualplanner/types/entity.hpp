#ifndef INCLUDE_CONTEXTUALPLANNER_ENTITY_HPP
#define INCLUDE_CONTEXTUALPLANNER_ENTITY_HPP

#include <string>
#include "../util/api.hpp"
#include "type.hpp"

namespace cp
{
struct SetOfTypes;


struct CONTEXTUALPLANNER_API Entity
{
  Entity(const std::string& pValue,
         const Type& pType = Type(""));

  static Entity createAnyEntity();
  static Entity fromStr(const std::string& pStr,
                        const SetOfTypes& pSetOfTypes);
  std::string toStr() const;

  std::string value;
  Type type;
};

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_ENTITY_HPP
