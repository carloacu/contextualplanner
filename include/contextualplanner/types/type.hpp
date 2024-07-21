#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_TYPE_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_TYPE_HPP

#include "../util/api.hpp"
#include <string>
#include <list>
#include <memory>

namespace cp
{

struct CONTEXTUALPLANNER_API Type
{
  Type(const std::string& pName,
       const std::shared_ptr<Type>& pParent = {});

  Type(const Type& pOther) = default;
  Type(Type&& pOther) noexcept = delete;
  Type& operator=(const Type& pOther) = default;
  Type& operator=(Type&& pOther) noexcept = delete;

  void toStrs(std::list<std::string>& pStrs) const;

  bool operator<(const Type& pOther) const;

  const std::string name;
  const std::shared_ptr<Type> parent;
  std::list<std::shared_ptr<Type>> subTypes;
};

} // namespace cp

#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_TYPE_HPP
