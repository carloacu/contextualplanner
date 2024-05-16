#include <contextualplanner/types/predicate.hpp>
#include "expressionParsed.hpp"

namespace cp
{
namespace
{
void _parametersToStr(std::string& pStr,
                      const std::vector<Entity>& pParameters)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += ", ";
    pStr += param.toStr();
  }
}

}

Predicate::Predicate(const std::string& pStr,
                     const SetOfTypes& pSetOfTypes)
  : name(),
    parameters(),
    fluent()
{
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);

  name = expressionParsed.name;
  for (auto& currArg : expressionParsed.arguments)
    if (currArg.followingExpression)
      parameters.emplace_back(Entity::fromStr(currArg.followingExpression->name, pSetOfTypes));
  if (expressionParsed.followingExpression)
    fluent.emplace(Entity::fromStr(expressionParsed.followingExpression->name, pSetOfTypes));
}


 std::string Predicate::toStr() const
 {
   auto res = name + "(";
   _parametersToStr(res, parameters);
   res += ")";
   if (fluent)
     res += " - " + fluent->toStr();
   return res;
 }

} // !cp
