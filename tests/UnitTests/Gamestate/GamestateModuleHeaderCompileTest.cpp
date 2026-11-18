#include <gtest/gtest.h>

#include "core/gamestate/GameStateManager.h"
#include "core/gamestate/IGameLogicRenderer.h"
#include "core/gamestate/IGameState.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/gamestate/SimulationDiagnostics.h"
#include "core/gamestate/TimedActionClock.h"

namespace {

/// Smoke compile: exported gamestate headers parse under repo-root includes.
class GamestateHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(GamestateHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}
