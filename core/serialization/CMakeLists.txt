# core.serialization — game state snapshot format (OBJECT lib for coreengine + standalone tests).
set(CORE_SERIALIZATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/GameStateSerializer.cpp
)

add_library(coreengine_serialization OBJECT ${CORE_SERIALIZATION_SOURCES})
target_include_directories(coreengine_serialization PUBLIC ${CMAKE_SOURCE_DIR})
