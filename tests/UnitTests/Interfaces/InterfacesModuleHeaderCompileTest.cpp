#include <gtest/gtest.h>

#include "core/interfaces/IBehavior.h"
#include "core/interfaces/IGameLogicRenderer.h"
#include "core/interfaces/IGameState.h"
#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/IUnitKinematics.h"
#include "core/interfaces/IUnitAgent.h"
#include "core/interfaces/IInputReader.h"
#include "core/interfaces/IMapComponent.h"
#include "core/interfaces/ISerializer.h"
#include "core/interfaces/ITransitionCondition.h"
#include "core/interfaces/ISimulationSnapshot.h"

namespace {

/// Smoke compile: exported interface headers parse with declared foundation deps only.
class InterfacesHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(InterfacesHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}