# core.influence — calculator + territory (types live in core/influence_types/).
set(CORE_INFLUENCE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/InfluenceFieldCalculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TerritoryCalculator.cpp
)

add_library(coreengine_influence OBJECT ${CORE_INFLUENCE_SOURCES})
target_include_directories(coreengine_influence PUBLIC ${CMAKE_SOURCE_DIR})