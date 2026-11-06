# core.logging — singleton logger implementation (headers are header-only).
set(CORE_LOGGING_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.cpp
)

add_library(coreengine_logging OBJECT ${CORE_LOGGING_SOURCES})
target_include_directories(coreengine_logging PUBLIC ${CMAKE_SOURCE_DIR})
