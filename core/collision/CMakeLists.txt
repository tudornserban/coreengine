# core.collision — detection, resolution, spatial grid (OBJECT lib for coreengine + dependents).
set(CORE_COLLISION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CollisionDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CollisionEventManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CollisionResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CollisionSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImmunityManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MapCollisionHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NarrowPhaseDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PositionCorrector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SpatialGrid.cpp
)

add_library(coreengine_collision OBJECT ${CORE_COLLISION_SOURCES})
target_include_directories(coreengine_collision PUBLIC ${CMAKE_SOURCE_DIR})
