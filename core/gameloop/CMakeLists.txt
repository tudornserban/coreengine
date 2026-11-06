# core.gameloop — deterministic timing / match loop helpers (OBJECT lib for coreengine + standalone tests).
set(CORE_GAMELOOP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/DeploymentTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogicEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlaybackController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThinkTimer.cpp
)

add_library(coreengine_gameloop OBJECT ${CORE_GAMELOOP_SOURCES})
target_include_directories(coreengine_gameloop PUBLIC ${CMAKE_SOURCE_DIR})