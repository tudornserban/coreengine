#include "InputScript.h"

#include <gtest/gtest.h>

TEST(InputScriptTest, PauseThenStepIsTwoCommands)
{
    std::string error;
    auto cmds = ParseScript("pause\nstep\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Pause);
    EXPECT_EQ(cmds[1].kind, ScriptCommand::Kind::Step);
}

TEST(InputScriptTest, ParsesClickAndRightClick)
{
    std::string error;
    auto cmds = ParseScript("click 426.7 360\nrclick 640 360\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Click);
    EXPECT_NEAR(cmds[0].floatArg, 426.7f, 0.01f);
    EXPECT_NEAR(cmds[0].x, 426.7f, 0.01f);
    EXPECT_NEAR(cmds[0].y, 360.0f, 0.01f);
    EXPECT_EQ(cmds[1].kind, ScriptCommand::Kind::RightClick);
}

TEST(InputScriptTest, ParsesBox)
{
    std::string error;
    auto cmds = ParseScript("box 10 20 300 400\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Box);
    EXPECT_FLOAT_EQ(cmds[0].x, 10.0f);
    EXPECT_FLOAT_EQ(cmds[0].y, 20.0f);
    EXPECT_FLOAT_EQ(cmds[0].x2, 300.0f);
    EXPECT_FLOAT_EQ(cmds[0].y2, 400.0f);
}

TEST(InputScriptTest, ParsesDestPauseFrames)
{
    std::string error;
    auto cmds = ParseScript("pause\ndest 0 400 300\nframes 2\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 3u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Pause);
    EXPECT_EQ(cmds[1].kind, ScriptCommand::Kind::Dest);
    EXPECT_EQ(cmds[1].intArg, 0);
    EXPECT_FLOAT_EQ(cmds[1].x, 400.0f);
    EXPECT_FLOAT_EQ(cmds[1].y, 300.0f);
    EXPECT_EQ(cmds[2].kind, ScriptCommand::Kind::Frames);
    EXPECT_EQ(cmds[2].intArg, 2);
}

TEST(InputScriptTest, ParsesPresetCommand)
{
    std::string error;
    auto cmds = ParseScript("preset combat\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Preset);
    EXPECT_EQ(cmds[0].strArg, "combat");
}

TEST(InputScriptTest, RejectsUnknownPreset)
{
    std::string error;
    auto cmds = ParseScript("preset unknown-name\n", error);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(cmds.empty());
}
