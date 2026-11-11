#include <gtest/gtest.h>

#include "core/interfaces/IGameLogicRenderer.h"

namespace {

class MinimalRenderer : public IGameLogicRenderer {
public:
    const RTSGameState* GetCurrentGameState() const override { return nullptr; }
};

TEST(IGameLogicRendererDefaultSnapshotTest, GetSimulationSnapshotDefaultsToNull) {
    MinimalRenderer renderer;
    EXPECT_EQ(renderer.GetSimulationSnapshot(), nullptr);
}

}  // namespace
