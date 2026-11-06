#include <gtest/gtest.h>

// Include canonical and legacy paths in one TU — single definition from interfaces;
// gamestate shims forward only (R1.4).
#include "core/gamestate/IGameLogicRenderer.h"
#include "core/gamestate/IGameState.h"
#include "core/interfaces/IGameLogicRenderer.h"
#include "core/interfaces/IGameState.h"

namespace {

class GameStateInterfaceForwardCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(GameStateInterfaceForwardCompileTest, BothIncludePathsAgree) {
    SUCCEED();
}
