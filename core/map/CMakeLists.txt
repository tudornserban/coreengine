# core.map — placement, entities, map components (OBJECT lib for coreengine + standalone tests).
set(CORE_MAP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapEntity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlacementValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapComponents/MapObstacleComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapComponents/MapResourceComponent.cpp
)

add_library(coreengine_map OBJECT ${CORE_MAP_SOURCES})
target_include_directories(coreengine_map PUBLIC ${CMAKE_SOURCE_DIR})