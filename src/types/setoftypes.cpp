#include <contextualplanner/types/setoftypes.hpp>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <contextualplanner/util/util.hpp>


namespace cp
{

SetOfTypes::SetOfTypes()
    : _types(),
      _nameToType()
{
}


SetOfTypes SetOfTypes::fromStr(const std::string& pStr)
{
  SetOfTypes res;
  std::vector<std::string> lineSplitted;
  cp::split(lineSplitted, pStr, "\n");

  for (auto& currLine : lineSplitted)
  {
    std::vector<std::string> typeWithParentType;
    cp::split(typeWithParentType, currLine, "-");

    if (typeWithParentType.empty())
      continue;

    std::string parentType;
    if (typeWithParentType.size() > 1)
    {
      parentType = typeWithParentType[1];
      ltrim(parentType);
      rtrim(parentType);
    }

    auto typesStrs = typeWithParentType[0];
    std::vector<std::string> types;
    cp::split(types, typesStrs, " ");
    for (auto& currType : types)
      if (!currType.empty())
        res.addType(currType, parentType);
  }
  return res;
}


void SetOfTypes::addType(const std::string& pTypeToAdd,
                         const std::string& pParentType)
{
  if (pParentType == "")
  {
    _types.push_back(Type(pTypeToAdd));
    _nameToType[pTypeToAdd] = &_types.back();
    return;
  }

  auto it = _nameToType.find(pParentType);
  if (it == _nameToType.end())
  {
    addType(pParentType);
    it = _nameToType.find(pParentType);
  }

  auto type = Type(pTypeToAdd, it->second);
  it->second->subTypes.push_back(type);
  _nameToType[pTypeToAdd] = &it->second->subTypes.back();
}

Type SetOfTypes::nameToType(const std::string& pName) const
{
  auto it = _nameToType.find(pName);
  if (it != _nameToType.end())
    return *it->second;
  throw std::runtime_error("\"" + pName + "\" isa not a valid type name");
}


std::list<std::string> SetOfTypes::typesToStrs() const
{
  if (_types.empty())
    return {};
  std::list<std::string> res;
  for (auto& currType : _types)
    currType.toStrs(res);
  return res;
}


std::string SetOfTypes::typesToStr() const
{
  std::stringstream ss;
  auto strs = typesToStrs();
  bool firstIteration = true;
  for (auto& currStr : strs)
  {
    if (firstIteration)
      firstIteration = false;
    else
      ss << "\n";
    ss << currStr;
  }
  return ss.str();
}



} // !cp
