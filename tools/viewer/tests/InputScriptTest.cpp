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

TEST(InputScriptTest, ClickLineIsParseErrorUntilV14)
{
    std::string error;
    auto cmds = ParseScript("click 427 360\n", error);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(cmds.empty());
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
