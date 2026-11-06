cmake_minimum_required(VERSION 3.17)
project(CoreEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_FFI      "Build shared library with C API"        OFF)
option(BUILD_TESTS    "Build unit tests with GoogleTest"        OFF)
option(BUILD_SDL2     "Build SDL2 platform adapter"            OFF)
option(BUILD_RAYLIB   "Build Raylib platform adapter"          OFF)
option(COREENGINE_MATH_STANDALONE "Build only core.math tests (no full coreengine)" OFF)
option(COREENGINE_INFLUENCE_STANDALONE "Build only core.influence tests (no full coreengine)" OFF)
option(COREENGINE_INFLUENCE_TYPES_STANDALONE "Build only core.influence.types tests" OFF)
option(COREENGINE_LOGGING_STANDALONE "Build only core.logging tests (no full coreengine)" OFF)
option(COREENGINE_INTERFACES_STANDALONE "Build only core.interfaces header compile tests" OFF)

set(_standalone_modes 0)
if(COREENGINE_MATH_STANDALONE)
    math(EXPR _standalone_modes "${_standalone_modes}+1")
endif()
if(COREENGINE_INFLUENCE_STANDALONE)
    math(EXPR _standalone_modes "${_standalone_modes}+1")
endif()
if(COREENGINE_INFLUENCE_TYPES_STANDALONE)
    math(EXPR _standalone_modes "${_standalone_modes}+1")
endif()
if(COREENGINE_LOGGING_STANDALONE)
    math(EXPR _standalone_modes "${_standalone_modes}+1")
endif()
if(COREENGINE_INTERFACES_STANDALONE)
    math(EXPR _standalone_modes "${_standalone_modes}+1")
endif()
if(_standalone_modes GREATER 1)
    message(FATAL_ERROR "At most one COREENGINE_*_STANDALONE option may be ON")
endif()

# Static archive and OBJECT-library .o files linked into coreengine_ffi must be PIC/PIE-ready.
# Scoped to BUILD_FFI so default static-only builds keep non-PIC objects (smaller .a on some toolchains).
if(BUILD_FFI)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(COREENGINE_MATH_STANDALONE)
    add_subdirectory(core/math)
    if(NOT BUILD_TESTS)
        message(FATAL_ERROR "COREENGINE_MATH_STANDALONE requires BUILD_TESTS=ON")
    endif()
    enable_testing()
    find_package(GTest REQUIRED)

    set(CORE_MATH_TEST_SOURCES
        tests/UnitTests/Math/DeterministicRNGTest.cpp
        tests/UnitTests/Math/MathConstantsTest.cpp
        tests/UnitTests/Math/Vector2Test.cpp
        tests/UnitTests/Math/Vector3Test.cpp
    )

    add_executable(coreengine_math_tests ${CORE_MATH_TEST_SOURCES})
    target_include_directories(coreengine_math_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(coreengine_math_tests
        coreengine_math
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_math_tests)
elseif(COREENGINE_INFLUENCE_TYPES_STANDALONE)
    add_subdirectory(core/math)
    add_subdirectory(core/influence_types)
    if(NOT BUILD_TESTS)
        message(FATAL_ERROR "COREENGINE_INFLUENCE_TYPES_STANDALONE requires BUILD_TESTS=ON")
    endif()
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(coreengine_influence_types_tests
        tests/UnitTests/Influence/InfluenceTypesCanonicalHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceLegacyItemHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceLegacyParametersHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceTypesModuleTest.cpp
    )
    target_include_directories(coreengine_influence_types_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(coreengine_influence_types_tests
        coreengine_influence_types
        coreengine_math
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_influence_types_tests)
elseif(COREENGINE_INFLUENCE_STANDALONE)
    add_subdirectory(core/math)
    add_subdirectory(core/influence_types)
    add_subdirectory(core/influence)
    if(NOT BUILD_TESTS)
        message(FATAL_ERROR "COREENGINE_INFLUENCE_STANDALONE requires BUILD_TESTS=ON")
    endif()
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(coreengine_influence_tests
        tests/UnitTests/Influence/InfluenceFieldTest.cpp
        tests/UnitTests/Influence/InfluenceTypesCanonicalHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceLegacyItemHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceLegacyParametersHeaderCompileTest.cpp
        tests/UnitTests/Influence/InfluenceTypesModuleTest.cpp
    )
    target_include_directories(coreengine_influence_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(coreengine_influence_tests
        coreengine_influence
        coreengine_influence_types
        coreengine_math
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_influence_tests)
elseif(COREENGINE_LOGGING_STANDALONE)
    add_subdirectory(core/logging)
    if(NOT BUILD_TESTS)
        message(FATAL_ERROR "COREENGINE_LOGGING_STANDALONE requires BUILD_TESTS=ON")
    endif()
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(coreengine_logging_tests
        tests/UnitTests/Logging/LoggerTest.cpp
        tests/UnitTests/Logging/LoggingLogLevelHeaderCompileTest.cpp
        tests/UnitTests/Logging/LoggingCanonicalHeaderCompileTest.cpp
        tests/UnitTests/Logging/LoggingIntegrationHeaderCompileTest.cpp
    )
    target_include_directories(coreengine_logging_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(coreengine_logging_tests
        coreengine_logging
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_logging_tests)
elseif(COREENGINE_INTERFACES_STANDALONE)
    add_subdirectory(core/math)
    add_subdirectory(core/influence_types)
    if(NOT BUILD_TESTS)
        message(FATAL_ERROR "COREENGINE_INTERFACES_STANDALONE requires BUILD_TESTS=ON")
    endif()
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(coreengine_interfaces_tests
        tests/UnitTests/Interfaces/InterfacesModuleHeaderCompileTest.cpp
    )
    target_include_directories(coreengine_interfaces_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(coreengine_interfaces_tests
        coreengine_influence_types
        coreengine_math
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_interfaces_tests)
else()
    add_subdirectory(core/math)
    add_subdirectory(core/influence_types)
    add_subdirectory(core/influence)
    add_subdirectory(core/logging)

    file(GLOB_RECURSE CORE_SOURCES
        "core/interfaces/*.cpp"
        "core/ai/*.cpp"
        "core/ai/Behaviors/*.cpp"
        "core/ai/Conditions/*.cpp"
        "core/collision/*.cpp"
        "core/gameloop/*.cpp"
        "core/map/*.cpp"
        "core/map/MapComponents/*.cpp"
        "core/combat/*.cpp"
        "core/gamestate/*.cpp"
        "core/ai_advanced/*.cpp"
        "core/serialization/*.cpp"
    )

    add_library(coreengine STATIC
        ${CORE_SOURCES}
        $<TARGET_OBJECTS:coreengine_math>
        $<TARGET_OBJECTS:coreengine_influence_types>
        $<TARGET_OBJECTS:coreengine_influence>
        $<TARGET_OBJECTS:coreengine_logging>
    )
    target_include_directories(coreengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    if(BUILD_FFI)
        add_library(coreengine_ffi SHARED ffi/c_api.cpp)
        target_link_libraries(coreengine_ffi PRIVATE coreengine)
        target_include_directories(coreengine_ffi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        set_target_properties(coreengine_ffi PROPERTIES
            OUTPUT_NAME "coreengine_ffi"
            POSITION_INDEPENDENT_CODE ON
        )
    endif()

    if(BUILD_SDL2)
        find_package(SDL2 REQUIRED)
        file(GLOB SDL2_SOURCES "platform/sdl2/*.cpp")
        add_library(coreengine_sdl2 STATIC ${SDL2_SOURCES})
        target_link_libraries(coreengine_sdl2 PUBLIC coreengine SDL2::SDL2)
        target_include_directories(coreengine_sdl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(BUILD_RAYLIB)
        find_package(raylib QUIET)
        file(GLOB RAYLIB_SOURCES "platform/raylib/*.cpp")
        add_library(coreengine_raylib STATIC ${RAYLIB_SOURCES})
        target_link_libraries(coreengine_raylib PUBLIC coreengine raylib)
        target_include_directories(coreengine_raylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(BUILD_TESTS)
        enable_testing()
        find_package(GTest REQUIRED)

        file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
        add_executable(coreengine_tests ${TEST_SOURCES})

        target_include_directories(coreengine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

        target_link_libraries(coreengine_tests
            coreengine
            GTest::gtest
            GTest::gtest_main
        )

        include(GoogleTest)
        gtest_discover_tests(coreengine_tests)
    endif()
endif()
