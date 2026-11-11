find_package(SDL2 REQUIRED)

add_library(coreengine_viewer_logic STATIC
    ViewerConfig.cpp
    Camera.cpp
    Raster.cpp
    PngWriter.cpp
    Scenario.cpp
    SnapshotRenderer.cpp
    PlaybackControl.cpp
    InputScript.cpp
    FrameLoop.cpp
    Overlay.cpp
    ViewerInput.cpp
)
target_link_libraries(coreengine_viewer_logic PUBLIC coreengine)
target_include_directories(coreengine_viewer_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(coreengine_sdl2_viewer
    main.cpp
    sdl/SdlPresenter.cpp
)
target_link_libraries(coreengine_sdl2_viewer PRIVATE
    coreengine_viewer_logic
    SDL2::SDL2
)
target_include_directories(coreengine_sdl2_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_executable(coreengine_sdl2_viewer_tests
        tests/CameraTest.cpp
        tests/PngWriterTest.cpp
        tests/RasterTest.cpp
        tests/ViewerConfigTest.cpp
        tests/SnapshotRendererTest.cpp
        tests/PlaybackControlTest.cpp
        tests/InputScriptTest.cpp
        tests/FrameLoopResetTest.cpp
        tests/CommandScriptTest.cpp
        tests/OverlayTest.cpp
        tests/ScenarioTest.cpp
        tests/ViewerInputTest.cpp
    )
    target_link_libraries(coreengine_sdl2_viewer_tests PRIVATE
        coreengine_viewer_logic
        GTest::gtest
        GTest::gtest_main
    )
    target_include_directories(coreengine_sdl2_viewer_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    include(GoogleTest)
    gtest_discover_tests(coreengine_sdl2_viewer_tests)

    set(VIEWER_CTEST_COLLISION_PNG "${CMAKE_CURRENT_BINARY_DIR}/ctest_collision_cli.png")
    add_test(
        NAME viewer_cli_collision_preset_screenshot
        COMMAND
            python3
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_collision_preset_cli.py
            $<TARGET_FILE:coreengine_sdl2_viewer>
            ${VIEWER_CTEST_COLLISION_PNG}
    )
    set_tests_properties(viewer_cli_collision_preset_screenshot PROPERTIES
        DEPENDS coreengine_sdl2_viewer
        LABELS "viewer_cli"
    )
endif()
