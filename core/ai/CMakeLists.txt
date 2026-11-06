# core.ai — behavior FSM, behaviors, and conditions (OBJECT lib for coreengine + standalone tests).
set(CORE_AI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/BehaviorFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StateMachine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Behaviors/CohesionBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Behaviors/FleeBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Behaviors/SeekBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Behaviors/SeparationBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Behaviors/WanderBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Conditions/AlliesNearbyCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Conditions/EnemyInRangeCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Conditions/TimeElapsedCondition.cpp
)

add_library(coreengine_ai OBJECT ${CORE_AI_SOURCES})
target_include_directories(coreengine_ai PUBLIC ${CMAKE_SOURCE_DIR})
