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
