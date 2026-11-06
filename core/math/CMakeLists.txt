# core.math — explicit source list (Phase 2 pilot). Header-only: Vector2, Vector3, MathConstants.
set(CORE_MATH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/DeterministicRNG.cpp
)

add_library(coreengine_math OBJECT ${CORE_MATH_SOURCES})
target_include_directories(coreengine_math PUBLIC ${CMAKE_SOURCE_DIR})