#include <gtest/gtest.h>

#include "core/gameloop/DeploymentTimer.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gameloop/MatchState.h"
#include "core/gameloop/PlaybackController.h"
#include "core/gameloop/ThinkTimer.h"

namespace {

/// Smoke compile: exported gameloop headers parse under repo-root includes.
class GameloopHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(GameloopHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}
