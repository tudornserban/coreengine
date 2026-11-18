# cmake -P probe. Confirms coreengine_detect_target_platform without configuring the engine.
if(NOT DEFINED PROBE_SYSTEM_NAME OR NOT DEFINED PROBE_EXPECT)
    message(FATAL_ERROR "PROBE_SYSTEM_NAME and PROBE_EXPECT are required")
endif()

set(CMAKE_SYSTEM_NAME "${PROBE_SYSTEM_NAME}")
if(NOT DEFINED PROBE_ANDROID)
    set(PROBE_ANDROID OFF)
endif()
if(NOT DEFINED PROBE_EMSCRIPTEN)
    set(PROBE_EMSCRIPTEN OFF)
endif()
set(ANDROID "${PROBE_ANDROID}")
set(EMSCRIPTEN "${PROBE_EMSCRIPTEN}")

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/CoreEnginePlatform.cmake")
coreengine_detect_target_platform(_detected)
if(NOT _detected STREQUAL "${PROBE_EXPECT}")
    message(FATAL_ERROR
        "platform probe mismatch: system='${PROBE_SYSTEM_NAME}' android='${PROBE_ANDROID}' "
        "emscripten='${PROBE_EMSCRIPTEN}' detected='${_detected}' expected='${PROBE_EXPECT}'")
endif()
message(STATUS "platform probe ok: ${PROBE_EXPECT}")
