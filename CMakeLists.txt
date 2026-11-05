cmake_minimum_required(VERSION 3.17)
project(CoreEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_FFI      "Build shared library with C API"        OFF)
option(BUILD_TESTS    "Build unit tests with GoogleTest"        OFF)
option(BUILD_SDL2     "Build SDL2 platform adapter"            OFF)
option(BUILD_RAYLIB   "Build Raylib platform adapter"          OFF)
option(COREENGINE_MATH_STANDALONE "Build only core.math tests (no full coreengine)" OFF)

add_subdirectory(core/math)

if(COREENGINE_MATH_STANDALONE)
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
else()
    # ---- Core static library ----

    file(GLOB_RECURSE CORE_SOURCES
        "core/interfaces/*.cpp"
        "core/ai/*.cpp"
        "core/ai/Behaviors/*.cpp"
        "core/ai/Conditions/*.cpp"
        "core/influence/*.cpp"
        "core/collision/*.cpp"
        "core/gameloop/*.cpp"
        "core/map/*.cpp"
        "core/map/MapComponents/*.cpp"
        "core/combat/*.cpp"
        "core/gamestate/*.cpp"
        "core/logging/*.cpp"
        "core/ai_advanced/*.cpp"
        "core/serialization/*.cpp"
    )

    add_library(coreengine STATIC ${CORE_SOURCES} $<TARGET_OBJECTS:coreengine_math>)
    target_include_directories(coreengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # ---- FFI shared library ----

    if(BUILD_FFI)
        add_library(coreengine_ffi SHARED ffi/c_api.cpp)
        target_link_libraries(coreengine_ffi PRIVATE coreengine)
        target_include_directories(coreengine_ffi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        set_target_properties(coreengine_ffi PROPERTIES
            OUTPUT_NAME "coreengine_ffi"
            POSITION_INDEPENDENT_CODE ON
        )
    endif()

    # ---- SDL2 platform adapter ----

    if(BUILD_SDL2)
        find_package(SDL2 REQUIRED)
        file(GLOB SDL2_SOURCES "platform/sdl2/*.cpp")
        add_library(coreengine_sdl2 STATIC ${SDL2_SOURCES})
        target_link_libraries(coreengine_sdl2 PUBLIC coreengine SDL2::SDL2)
        target_include_directories(coreengine_sdl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    # ---- Raylib platform adapter ----

    if(BUILD_RAYLIB)
        find_package(raylib QUIET)
        file(GLOB RAYLIB_SOURCES "platform/raylib/*.cpp")
        add_library(coreengine_raylib STATIC ${RAYLIB_SOURCES})
        target_link_libraries(coreengine_raylib PUBLIC coreengine raylib)
        target_include_directories(coreengine_raylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    # ---- Tests ----

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
