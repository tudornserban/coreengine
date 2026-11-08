#include <gtest/gtest.h>
#include <memory>

#include "core/combat/Explosion.h"
#include "core/gamestate/RTSGameState.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockGameUnit.h"

// R2.2 — documents current RTSGameState dynamic_cast<RTSUnit*> gates only.
// MockGameUnit implements IGameUnit but not IUnitKinematics / IUnitAgent / RTSUnit.
// These tests do not prove behavior if bounds fallback is retargeted to IUnitKinematics:
// MockGameUnit would still not match that cast.

namespace {

constexpr float kMapSize = 100.0f;
constexpr uint64_t kRngSeed = 0;

class RTSGameStateNonRTSUnitGateTest : public ::testing::Test {
protected:
    RTSGameState state{Vector2(kMapSize, kMapSize), kRngSeed};

    MockGameUnit* AddMock(Vector2 position, Vector2 velocity) {
        auto mock = std::make_unique<MockGameUnit>(position, /*team=*/0, 10.0f, 100.0f);
        mock->SetVelocity(velocity);
        MockGameUnit* ptr = mock.get();
        state.AddUnit(std::move(mock));
        return ptr;
    }
};

}  // namespace

TEST_F(RTSGameStateNonRTSUnitGateTest, AiTickGate_MockUnitPositionAndVelocityUnchanged) {
  const Vector2 startPos(40.0f, 40.0f);
  const Vector2 startVel(25.0f, -10.0f);
  MockGameUnit* unit = AddMock(startPos, startVel);

  state.Update(0.5f);

  EXPECT_FLOAT_EQ(unit->GetPosition().x, startPos.x);
  EXPECT_FLOAT_EQ(unit->GetPosition().y, startPos.y);
  EXPECT_FLOAT_EQ(unit->GetVelocity().x, startVel.x);
  EXPECT_FLOAT_EQ(unit->GetVelocity().y, startVel.y);
}

TEST_F(RTSGameStateNonRTSUnitGateTest, ExplosionImpulseGate_MockUnitPositionAndVelocityUnchanged) {
  const Vector2 startPos(50.0f, 50.0f);
  const Vector2 startVel(12.0f, 8.0f);
  MockGameUnit* unit = AddMock(startPos, startVel);

  // Offset center so the unit is inside radius with nonzero impulse (not at singularity).
  const Vector2 explosionCenter(35.0f, 50.0f);
  state.AddExplosion(Explosion(explosionCenter, /*radius=*/40.0f, /*strength=*/8000.0f));

  state.Update(0.25f);

  EXPECT_FLOAT_EQ(unit->GetPosition().x, startPos.x);
  EXPECT_FLOAT_EQ(unit->GetPosition().y, startPos.y);
  EXPECT_FLOAT_EQ(unit->GetVelocity().x, startVel.x);
  EXPECT_FLOAT_EQ(unit->GetVelocity().y, startVel.y);
}

TEST_F(RTSGameStateNonRTSUnitGateTest, BoundsFallbackGate_MockOutsideMapPositionUnchanged) {
  const Vector2 outsidePos(kMapSize + 20.0f, 30.0f);
  const Vector2 startVel(5.0f, -3.0f);
  MockGameUnit* unit = AddMock(outsidePos, startVel);

  state.Update(1.0f);

  EXPECT_FLOAT_EQ(unit->GetPosition().x, outsidePos.x);
  EXPECT_FLOAT_EQ(unit->GetPosition().y, outsidePos.y);
  EXPECT_FLOAT_EQ(unit->GetVelocity().x, startVel.x);
  EXPECT_FLOAT_EQ(unit->GetVelocity().y, startVel.y);
}
