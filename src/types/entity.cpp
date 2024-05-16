#include <contextualplanner/types/entity.hpp>
#include <vector>
#include <contextualplanner/types/setoftypes.hpp>
#include <contextualplanner/util/util.hpp>


namespace cp
{
namespace
{
const std::string _anyValue = "*";
}

Entity::Entity(const std::string& pValue,
               const Type& pType)
 : value(pValue),
   type(pType)
{
}

Entity Entity::createAnyEntity()
{
  return Entity(_anyValue);
}


Entity Entity::fromStr(const std::string& pStr,
                       const SetOfTypes& pSetOfTypes)
{
  std::vector<std::string> nameWithType;
  cp::split(nameWithType, pStr, "-");

  if (nameWithType.empty())
    throw std::runtime_error("\"" + pStr + "\" is not a valid entity");

  if (nameWithType.size() > 1)
  {
    auto typeStr = nameWithType[1];
    trim(typeStr);
    return Entity(nameWithType[0], pSetOfTypes.nameToType(typeStr));
  }

  return Entity(nameWithType[0]);
}


std::string Entity::toStr() const
{
  if (type.name.empty())
    return value;
  return value + " - " + type.name;
}


} // !cp
