#include <contextualplanner/types/setoftypes.hpp>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <contextualplanner/util/util.hpp>


namespace cp
{
namespace
{
const std::string _numberTypeName = "number";
const std::shared_ptr<Type> _numberType = std::make_shared<Type>(_numberTypeName);
}

SetOfTypes::SetOfTypes()
    : _types(),
      _nameToType()
{
  _nameToType[_numberTypeName] = _numberType;
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
    _types.push_back(std::make_shared<Type>(pTypeToAdd));
    _nameToType[pTypeToAdd] = _types.back();
    return;
  }

  auto it = _nameToType.find(pParentType);
  if (it == _nameToType.end())
  {
    addType(pParentType);
    it = _nameToType.find(pParentType);
  }

  auto type = std::make_shared<Type>(pTypeToAdd, it->second);
  it->second->subTypes.push_back(type);
  _nameToType[pTypeToAdd] = it->second->subTypes.back();
}

std::shared_ptr<Type> SetOfTypes::nameToType(const std::string& pName) const
{
  auto it = _nameToType.find(pName);
  if (it != _nameToType.end())
    return it->second;
  throw std::runtime_error("\"" + pName + "\" is not a valid type name");
}

std::shared_ptr<Type> SetOfTypes::numberType()
{
  return _numberType;
}


std::list<std::string> SetOfTypes::typesToStrs() const
{
  if (_types.empty())
    return {};
  std::list<std::string> res;
  for (auto& currType : _types)
    currType->toStrs(res);
  return res;
}


std::string SetOfTypes::toStr(std::size_t pIdentation) const
{
  std::string res;
  auto strs = typesToStrs();
  bool firstIteration = true;
  for (auto& currStr : strs)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += std::string(pIdentation, ' ') + currStr;
  }
  return res;
}

bool SetOfTypes::empty() const
{
  return _types.empty();
}


} // !cp
