#include <contextualplanner/types/setofinferences.hpp>
#include <contextualplanner/util/util.hpp>

namespace cp
{

SetOfInferences::SetOfInferences(const Inference& pInference)
 : _inferences(),
   _reachableInferenceLinks(),
   _unreachableInferenceLinks()
{
  addInference(pInference);
}


InferenceId SetOfInferences::addInference(const Inference& pInference,
                                          const InferenceId& pInferenceId)
{
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _inferences.count(pId) == 0;
  };
  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pInferenceId, isIdOkForInsertion);

  _inferences.emplace(newId, pInference);
  auto& links = pInference.isReachable ? _reachableInferenceLinks : _unreachableInferenceLinks;

  if (pInference.condition)
  {
    pInference.condition->forAll(
          [&](const FactOptional& pFactOptional)
    {
      if (pFactOptional.isFactNegated)
        links.notConditionToInferences[pFactOptional.fact.name].insert(newId);
      else
        links.conditionToInferences[pFactOptional.fact.name].insert(newId);
    }
    );
  }
  return newId;
}


void SetOfInferences::removeInference(const InferenceId& pInferenceId)
{
  auto it = _inferences.find(pInferenceId);
  if (it == _inferences.end())
    return;
  auto& inferenceThatWillBeRemoved = it->second;
  auto& links = inferenceThatWillBeRemoved.isReachable ? _reachableInferenceLinks : _unreachableInferenceLinks;

  if (inferenceThatWillBeRemoved.condition)
  {
    inferenceThatWillBeRemoved.condition->forAll(
          [&](const FactOptional& pFactOptional)
    {
      if (pFactOptional.isFactNegated)
        links.notConditionToInferences[pFactOptional.fact.name].erase(pInferenceId);
      else
        links.conditionToInferences[pFactOptional.fact.name].erase(pInferenceId);
    }
    );
  }
  _inferences.erase(it);
}


} // !cp
