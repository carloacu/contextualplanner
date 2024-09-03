#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFTYPES_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFTYPES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <string>
#include "type.hpp"

namespace cp
{

struct CONTEXTUALPLANNER_API SetOfTypes
{
  SetOfTypes();

  static SetOfTypes fromStr(const std::string& pStr);

  void addType(const std::string& pTypeToAdd,
               const std::string& pParentType = "");

  std::shared_ptr<Type> nameToType(const std::string& pName) const;
  std::shared_ptr<Type> numberType() const { return _numberType; }

  std::list<std::string> typesToStrs() const;
  std::string toStr() const;
  bool empty() const;

private:
  std::shared_ptr<Type> _numberType;
  std::list<std::shared_ptr<Type>> _types;
  std::map<std::string, std::shared_ptr<Type>> _nameToType;
};

} // namespace cp

#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFTYPES_HPP
