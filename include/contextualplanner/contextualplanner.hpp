#ifndef INCLUDE_CONTEXTUALPLANNER_CONTEXTUALPLANNER_HPP
#define INCLUDE_CONTEXTUALPLANNER_CONTEXTUALPLANNER_HPP

#include <map>
#include <set>
#include <list>
#include <vector>
#include <assert.h>
#include "api.hpp"
#include <contextualplanner/alias.hpp>
#include <contextualplanner/domain.hpp>
#include <contextualplanner/problem.hpp>



namespace cp
{

CONTEXTUALPLANNER_API
void replaceVariables(std::string& pStr,
                      const Problem& pProblem);


CONTEXTUALPLANNER_API
void fillReachableFacts(Problem& pProblem,
                        const Domain& pDomain);


CONTEXTUALPLANNER_API
bool areFactsTrue(const SetOfFacts& pSetOfFacts,
                  const Problem& pProblem);

CONTEXTUALPLANNER_API
ActionId lookForAnActionToDo(std::map<std::string, std::string>& pParameters,
                             Problem& pProblem,
                             const Domain& pDomain,
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                             const Goal** pGoalOfTheAction = nullptr,
                             int* pGoalPriority = nullptr,
                             const Historical* pGlobalHistorical = nullptr);

CONTEXTUALPLANNER_API
std::string printActionIdWithParameters(
    const std::string& pActionId,
    const std::map<std::string, std::string>& pParameters);

CONTEXTUALPLANNER_API
std::list<ActionId> solve(Problem& pProblem,
                          const Domain& pDomain,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                          Historical* pGlobalHistorical = nullptr);
} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_CONTEXTUALPLANNER_HPP
