#include "ViewerConfig.h"

#include <gtest/gtest.h>

TEST(ViewerConfigTest, DefaultsAndFramesFlag)
{
    const char* argv[] = {"viewer", "--frames", "4", "--no-window", "--screenshot", "/tmp/a.png"};
    ViewerConfig config = ParseViewerConfig(6, const_cast<char**>(argv));
    EXPECT_EQ(config.frames, 4);
    EXPECT_TRUE(config.noWindow);
    EXPECT_EQ(config.screenshotPath, "/tmp/a.png");
    EXPECT_EQ(config.width, 1280);
    EXPECT_EQ(config.height, 720);
}

TEST(ViewerConfigTest, PresetFlagSelectsCollision)
{
    const char* argv[] = {"viewer", "--preset", "collision", "--no-window", "--frames", "1"};
    ViewerConfig config = ParseViewerConfig(6, const_cast<char**>(argv));
    EXPECT_FALSE(config.parseError);
    EXPECT_EQ(config.preset, Preset::Collision);
    EXPECT_EQ(config.frames, 1);
}

TEST(ViewerConfigTest, UnknownPresetFailsParse)
{
    const char* argv[] = {"viewer", "--preset", "bogus"};
    ViewerConfig config = ParseViewerConfig(3, const_cast<char**>(argv));
    EXPECT_TRUE(config.parseError);
}
