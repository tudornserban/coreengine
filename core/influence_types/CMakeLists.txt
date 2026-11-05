# core.influence.types — value types (Phase 2 leaf). Implementation: InfluenceFieldItem.cpp only.
set(CORE_INFLUENCE_TYPES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/InfluenceFieldItem.cpp
)

add_library(coreengine_influence_types OBJECT ${CORE_INFLUENCE_TYPES_SOURCES})
target_include_directories(coreengine_influence_types PUBLIC ${CMAKE_SOURCE_DIR})
