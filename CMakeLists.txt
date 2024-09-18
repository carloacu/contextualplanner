project(contextualplanner)
cmake_minimum_required(VERSION 2.8)

include(contextualplanner-config.cmake)

# Making a lib
set(CHATBOTPLANNER_HPPS
    include/contextualplanner/types/action.hpp
    include/contextualplanner/types/actioninvocation.hpp
    include/contextualplanner/types/actioninvocationwithgoal.hpp
    include/contextualplanner/types/condition.hpp
    include/contextualplanner/types/derivedpredicate.hpp
    include/contextualplanner/types/domain.hpp
    include/contextualplanner/types/entity.hpp
    include/contextualplanner/types/fact.hpp

    include/contextualplanner/types/factoptional.hpp
    include/contextualplanner/types/goal.hpp
    include/contextualplanner/types/goalstack.hpp
    include/contextualplanner/types/historical.hpp
    include/contextualplanner/types/inference.hpp
    include/contextualplanner/types/lookforanactionoutputinfos.hpp
    include/contextualplanner/types/facttoconditions.hpp
    include/contextualplanner/types/ontology.hpp
    include/contextualplanner/types/parameter.hpp
    include/contextualplanner/types/predicate.hpp
    include/contextualplanner/types/problem.hpp
    include/contextualplanner/types/problemmodification.hpp
    include/contextualplanner/types/setofentities.hpp
    include/contextualplanner/types/setoffacts.hpp
    include/contextualplanner/types/setofinferences.hpp
    include/contextualplanner/types/setofpredicates.hpp
    include/contextualplanner/types/setoftypes.hpp
    include/contextualplanner/types/type.hpp
    include/contextualplanner/types/worldstate.hpp
    include/contextualplanner/types/worldstatemodification.hpp
    include/contextualplanner/util/exportsymbols/macro.hpp
    include/contextualplanner/util/trackers/goalsremovedtracker.hpp
    include/contextualplanner/util/api.hpp
    include/contextualplanner/util/alias.hpp
    include/contextualplanner/util/arithmeticevaluator.hpp
    include/contextualplanner/util/print.hpp
    include/contextualplanner/util/observableunsafe.hpp
    include/contextualplanner/util/replacevariables.hpp
    include/contextualplanner/util/util.hpp
    include/contextualplanner/contextualplanner.hpp
)


set(CHATBOTPLANNER_SRCS
    src/algo/converttoparallelplan.hpp
    src/algo/converttoparallelplan.cpp
    src/algo/notifyactiondone.hpp
    src/algo/notifyactiondone.cpp
    src/types/action.cpp
    src/types/actioninvocation.cpp
    src/types/actioninvocationwithgoal.cpp
    src/types/condition.cpp
    src/types/derivedpredicate.cpp
    src/types/domain.cpp
    src/types/entity.cpp
    src/types/expressionParsed.hpp
    src/types/expressionParsed.cpp
    src/types/fact.cpp

    src/types/factoptional.cpp
    src/types/factsalreadychecked.hpp
    src/types/goal.cpp
    src/types/goalstack.cpp
    src/types/historical.cpp
    src/types/inference.cpp
    src/types/lookforanactionoutputinfos.cpp
    src/types/facttoconditions.cpp
    src/types/parameter.cpp
    src/types/predicate.cpp
    src/types/problemmodification.cpp
    src/types/setofentities.cpp
    src/types/setoffacts.cpp
    src/types/setofinferences.cpp
    src/types/setofpredicates.cpp
    src/types/setoftypes.cpp
    src/types/treeofalreadydonepaths.hpp
    src/types/treeofalreadydonepaths.cpp
    src/types/type.cpp
    src/types/worldstate.cpp
    src/types/worldstatecache.hpp
    src/types/worldstatecache.cpp
    src/types/worldstatemodification.cpp
    src/util/trackers/goalsremovedtracker.cpp
    src/util/arithmeticevaluator.cpp
    src/util/print.cpp
    src/util/replacevariables.cpp
    src/util/util.cpp
    src/util/uuid.hpp
    src/util/uuid.cpp
    src/contextualplanner.cpp
)


include_directories(include)


add_library(contextualplanner SHARED
  ${CHATBOTPLANNER_HPPS}
  ${CHATBOTPLANNER_SRCS}
)
target_compile_features(contextualplanner PRIVATE cxx_std_14)

install(TARGETS contextualplanner DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)


add_subdirectory(tests)

