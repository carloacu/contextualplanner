#include <contextualplanner/types/factoptional.hpp>


namespace cp
{

FactOptional::FactOptional(const Fact& pFact,
                           bool pIsFactNegated)
  : isFactNegated(pIsFactNegated),
    fact(pFact)
{
}

FactOptional::FactOptional(bool pIsFactNegated,
                           const std::string& pName,
                           const std::vector<std::string>& pArgumentStrs,
                           const std::string& pFluentStr,
                           const Ontology& pOntology,
                           const SetOfEntities& pEntities,
                           const std::vector<Parameter>& pParameters,
                           bool pIsOkIfFluentIsMissing)
  : isFactNegated(pIsFactNegated),
    fact(Fact(pName, pArgumentStrs, pFluentStr, pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing))
{
}


FactOptional::FactOptional(const FactOptional& pOther,
                           const std::map<Parameter, Entity>* pParametersPtr)
  : isFactNegated(pOther.isFactNegated),
    fact(pOther.fact)
{
  if (pParametersPtr != nullptr)
    fact.replaceArguments(*pParametersPtr);
}

FactOptional::FactOptional(const std::string& pStr,
                           const Ontology& pOntology,
                           const SetOfEntities& pEntities,
                           const std::vector<Parameter>& pParameters,
                           const char* pSeparatorPtr,
                           std::size_t pBeginPos,
                           std::size_t* pResPos)
  : isFactNegated(false),
    fact(pStr, pOntology, pEntities, pParameters, pSeparatorPtr, &isFactNegated, pBeginPos, pResPos)
{
}

bool FactOptional::operator<(const FactOptional& pOther) const
{
  if (isFactNegated != pOther.isFactNegated)
    return isFactNegated < pOther.isFactNegated;
  return fact < pOther.fact;
}

void FactOptional::operator=(const FactOptional& pOther)
{
  isFactNegated = pOther.isFactNegated;
  fact = pOther.fact;
}


bool FactOptional::operator==(const FactOptional& pOther) const
{
  return isFactNegated == pOther.isFactNegated &&
      fact == pOther.fact;
}

std::string FactOptional::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                bool pPrintAnyFluent) const
{
  auto polarityStr = isFactNegated ? "!" : "";
  if (pFactWriterPtr)
    return polarityStr + (*pFactWriterPtr)(fact);
  return polarityStr + fact.toStr(pPrintAnyFluent);
}


} // !cp
