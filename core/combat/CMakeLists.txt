# core.combat — selection, commands, explosions (OBJECT lib for coreengine + dependents).
set(CORE_COMBAT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CombatManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Explosion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SelectionSystem.cpp
)

add_library(coreengine_combat OBJECT ${CORE_COMBAT_SOURCES})
target_include_directories(coreengine_combat PUBLIC ${CMAKE_SOURCE_DIR})
