# core.gamestate — simulation hub (OBJECT lib for coreengine + serialization standalone).
set(CORE_GAMESTATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/BehaviorContextAssembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GameStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RTSGameState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RTSUnit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SimulationRecord.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TimedActionClock.cpp
)

add_library(coreengine_gamestate OBJECT ${CORE_GAMESTATE_SOURCES})
target_include_directories(coreengine_gamestate PUBLIC ${PROJECT_SOURCE_DIR})
