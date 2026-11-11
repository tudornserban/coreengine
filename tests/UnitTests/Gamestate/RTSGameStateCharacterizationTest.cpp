#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include "core/ai/BehaviorContext.h"
#include "core/collision/CollisionEvent.h"
#include "core/collision/ICollisionObserver.h"
#include "core/combat/Explosion.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/interfaces/IBehavior.h"
#include "core/interfaces/IUnitBehaviorHost.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/KinematicMockGameUnit.h"
#include "tests/TestHelpers/MockGameUnit.h"

namespace {

constexpr uint64_t kSeed = 0;
constexpr float kMap100 = 100.0f;

class RecordingCollisionObserver : public ICollisionObserver {
public:
    void OnCollision(const CollisionEvent& /*event*/) override { ++eventCount; }
    int eventCount = 0;
};

class SpyBehavior : public IBehavior {
public:
    int generateFieldItemsCalls = 0;
    int calculateAccelerationCalls = 0;
    std::string name_ = "SpyBehavior";

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& /*ctx*/) override {
        ++generateFieldItemsCalls;
        return {};
    }

    Vector2 CalculateAcceleration(const BehaviorContext& /*ctx*/) override {
        ++calculateAccelerationCalls;
        return Vector2(999.0f, 999.0f);
    }

    const char* GetName() const override { return name_.c_str(); }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    std::map<std::string, float> params_;
};

RTSGameState MakeMap100() {
    return RTSGameState(Vector2(kMap100, kMap100), kSeed);
}

RTSUnit* AddRts(RTSGameState& state, Vector2 pos, int team, float radius = 10.0f, float maxSpeed = 100.0f) {
    auto unit = std::make_unique<RTSUnit>(pos, team, radius, maxSpeed);
    RTSUnit* ptr = unit.get();
    state.AddUnit(std::move(unit));
    return ptr;
}

SpyBehavior* InstallOwnedSpy(RTSUnit& unit) {
    auto spy = std::make_unique<SpyBehavior>();
    SpyBehavior* ptr = spy.get();
    unit.GetStateMachine().AddState("Spy", std::move(spy));
    unit.GetStateMachine().SetState("Spy");
    return ptr;
}

// Non-RTSUnit host for the IUnitBehaviorHost AI tick gate (R4.5 / R4.6 positive pin).
class BehaviorHostMockGameUnit : public MockGameUnit, public IUnitBehaviorHost {
public:
    explicit BehaviorHostMockGameUnit(Vector2 position, int team, float radius, float maxSpeed)
        : MockGameUnit(position, team, radius, maxSpeed) {}

    void InstallSpy(std::unique_ptr<SpyBehavior> spy) {
        spy_ = std::move(spy);
        spyPtr_ = spy_.get();
    }

    SpyBehavior* Spy() const { return spyPtr_; }

    IBehavior* GetCurrentBehavior() override { return spyPtr_; }

private:
    std::unique_ptr<SpyBehavior> spy_;
    SpyBehavior* spyPtr_ = nullptr;
};

}  // namespace

class RTSGameStateContextCharacterizationTest : public ::testing::Test {
protected:
    RTSGameState state{MakeMap100()};
};

TEST_F(RTSGameStateContextCharacterizationTest, PerceptionRadiusDefault) {
    EXPECT_FLOAT_EQ(state.GetPerceptionRadius(), 300.0f);
}

TEST_F(RTSGameStateContextCharacterizationTest, MapCenterSizeAndContextBounds) {
    EXPECT_FLOAT_EQ(state.GetMapCenter().x, 50.0f);
    EXPECT_FLOAT_EQ(state.GetMapCenter().y, 50.0f);
    EXPECT_FLOAT_EQ(state.GetMapSize().x, kMap100);
    EXPECT_FLOAT_EQ(state.GetMapSize().y, kMap100);

    RTSUnit* self = AddRts(state, Vector2(10.0f, 10.0f), 0);
    const BehaviorContext ctx = state.BuildContextForUnit(self, 0.0f);
    EXPECT_FLOAT_EQ(ctx.mapBounds.x, kMap100);
    EXPECT_FLOAT_EQ(ctx.mapBounds.y, kMap100);
}

TEST_F(RTSGameStateContextCharacterizationTest, StaticEdgeItemsPin) {
    const auto& edges = state.GetStaticItems();
    ASSERT_EQ(edges.size(), 4u);
    for (const auto& edge : edges) {
        EXPECT_EQ(edge.GetInfluenceType(), InfluenceType::EDGE);
        EXPECT_FLOAT_EQ(edge.GetEdgeParams().RepulsorStrength, 150.0f);
        EXPECT_FLOAT_EQ(edge.GetEdgeParams().DetectionRange, 80.0f);
    }
    EXPECT_FLOAT_EQ(edges[0].GetPosition().x, -40.0f);
    EXPECT_FLOAT_EQ(edges[0].GetPosition().y, 50.0f);
    EXPECT_FLOAT_EQ(edges[1].GetPosition().x, 140.0f);
    EXPECT_FLOAT_EQ(edges[1].GetPosition().y, 50.0f);
    EXPECT_FLOAT_EQ(edges[2].GetPosition().x, 50.0f);
    EXPECT_FLOAT_EQ(edges[2].GetPosition().y, -40.0f);
    EXPECT_FLOAT_EQ(edges[3].GetPosition().x, 50.0f);
    EXPECT_FLOAT_EQ(edges[3].GetPosition().y, 140.0f);
}

TEST_F(RTSGameStateContextCharacterizationTest, NearbyAlliesEnemiesOrderAndPerception) {
    RTSUnit* self = AddRts(state, Vector2(10.0f, 10.0f), 0);
    AddRts(state, Vector2(20.0f, 10.0f), 0);
    AddRts(state, Vector2(10.0f, 40.0f), 1);
    AddRts(state, Vector2(10.0f, 400.0f), 0);

  const auto all = state.GetAllUnits();
    ASSERT_GE(all.size(), 4u);
    EXPECT_EQ(all[0], self);

    const BehaviorContext ctx = state.BuildContextForUnit(self, 0.016f);
    EXPECT_EQ(ctx.self, self);
    ASSERT_EQ(ctx.nearbyAllies.size(), 1u);
    ASSERT_EQ(ctx.nearbyEnemies.size(), 1u);
    EXPECT_EQ(ctx.nearbyAllies[0]->GetPosition().x, 20.0f);
    EXPECT_EQ(ctx.nearbyEnemies[0]->GetPosition().y, 40.0f);

    for (IGameUnit* ally : ctx.nearbyAllies) {
        EXPECT_NE(ally, self);
    }
    for (IGameUnit* enemy : ctx.nearbyEnemies) {
        EXPECT_NE(enemy, self);
    }
}

TEST_F(RTSGameStateContextCharacterizationTest, NearbyUnitAtExactlyPerceptionRadiusIsIncluded) {
    RTSUnit* self = AddRts(state, Vector2(10.0f, 10.0f), 0);
    AddRts(state, Vector2(10.0f, 310.0f), 0);

    const BehaviorContext ctx = state.BuildContextForUnit(self, 0.016f);
    ASSERT_EQ(ctx.nearbyAllies.size(), 1u);
    EXPECT_FLOAT_EQ(ctx.nearbyAllies[0]->GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(ctx.nearbyAllies[0]->GetPosition().y, 310.0f);
}

TEST_F(RTSGameStateContextCharacterizationTest, StaticFieldItemsAttractorAndObstacle) {
    RTSUnit* self = AddRts(state, Vector2(10.0f, 10.0f), 0);

    const Vector2 attractorPos(25.0f, 25.0f);
    const InfluenceFieldItem attractor =
        InfluenceFieldItem::CreateAttractor(attractorPos, /*importance=*/1.0f);
    state.AddStaticAttractor(attractor);

    const Vector2 obstaclePos(60.0f, 60.0f);
    const InfluenceFieldItem obstacle =
        InfluenceFieldItem::CreateRepulsor(obstaclePos, /*importance=*/1.0f);
    state.AddObstacle(obstacle);

    EXPECT_EQ(state.GetStaticItems().size(), 4u);

    const BehaviorContext ctx = state.BuildContextForUnit(self, 0.0f);
    ASSERT_EQ(ctx.staticFieldItems.size(), 6u);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(ctx.staticFieldItems[i].GetInfluenceType(), InfluenceType::EDGE);
    }
    EXPECT_EQ(ctx.staticFieldItems[4].GetInfluenceType(), InfluenceType::ATTRACTOR);
    EXPECT_FLOAT_EQ(ctx.staticFieldItems[4].GetPosition().x, attractorPos.x);
    EXPECT_EQ(ctx.staticFieldItems[5].GetInfluenceType(), InfluenceType::REPULSOR);
    EXPECT_FLOAT_EQ(ctx.staticFieldItems[5].GetPosition().x, obstaclePos.x);
}

class RTSGameStateUpdateCharacterizationTest : public ::testing::Test {
protected:
    RTSGameState state{MakeMap100()};

    void SubscribeCollisionObserver() {
        state.GetCollisionSystem().Subscribe(&observer_);
    }

    RecordingCollisionObserver observer_;
};

TEST_F(RTSGameStateUpdateCharacterizationTest, MockUnitMapEdgeEmitsWithoutKinematicWrite) {
    SubscribeCollisionObserver();
    auto mock = std::make_unique<MockGameUnit>(Vector2(-5.0f, 50.0f), 0, 10.0f, 100.0f);
    MockGameUnit* ptr = mock.get();
    state.AddUnit(std::move(mock));

    const Vector2 before = ptr->GetPosition();
    state.Update(0.016f);

    EXPECT_EQ(observer_.eventCount, 1);
    EXPECT_FLOAT_EQ(ptr->GetPosition().x, before.x);
    EXPECT_FLOAT_EQ(ptr->GetPosition().y, before.y);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, RtsUnitMapEdgeInsetNotClampedToZero) {
    SubscribeCollisionObserver();
    RTSUnit* unit = AddRts(state, Vector2(-5.0f, 50.0f), 0, 10.0f);

    state.Update(0.016f);

    EXPECT_EQ(observer_.eventCount, 1);
    EXPECT_FLOAT_EQ(unit->GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(unit->GetPosition().y, 50.0f);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, ExplosionImpulseOnRtsUnit) {
    RTSUnit* unit = AddRts(state, Vector2(50.0f, 50.0f), 0, 10.0f, 100.0f);
    state.AddExplosion(Explosion(Vector2(35.0f, 50.0f), 40.0f, 8000.0f));

    state.Update(0.25f);

    EXPECT_FLOAT_EQ(unit->GetVelocity().x, 100.0f);
    EXPECT_FLOAT_EQ(unit->GetVelocity().y, 0.0f);
    EXPECT_FLOAT_EQ(unit->GetPosition().x, 75.0f);
    EXPECT_FLOAT_EQ(unit->GetPosition().y, 50.0f);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, BoundsFallbackClampsCenterNotRadiusInset) {
    RTSUnit* unit = AddRts(state, Vector2(90.0f, 50.0f), 0, 10.0f, 100.0f);
    state.AddExplosion(Explosion(Vector2(70.0f, 50.0f), 40.0f, 8000.0f));

    state.Update(1.0f);

    EXPECT_FLOAT_EQ(unit->GetPosition().x, 100.0f);
    EXPECT_FLOAT_EQ(unit->GetPosition().y, 50.0f);
    EXPECT_FLOAT_EQ(unit->GetVelocity().x, 100.0f);
    EXPECT_FLOAT_EQ(unit->GetVelocity().y, 0.0f);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, AiTickVisitsSpyBehaviorEdgesOnly) {
    RTSGameState large(Vector2(1000.0f, 1000.0f), kSeed);
    RTSUnit* unit = AddRts(large, Vector2(500.0f, 500.0f), 0);
    SpyBehavior* spy = InstallOwnedSpy(*unit);
    const Vector2 start = unit->GetPosition();

    large.Update(0.016f);

    EXPECT_EQ(spy->generateFieldItemsCalls, 1);
    EXPECT_EQ(spy->calculateAccelerationCalls, 0);
    EXPECT_FLOAT_EQ(unit->GetPosition().x, start.x);
    EXPECT_FLOAT_EQ(unit->GetPosition().y, start.y);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, AiTickSkipsMockUnit) {
    RTSGameState large(Vector2(1000.0f, 1000.0f), kSeed);
    RTSUnit* rts = AddRts(large, Vector2(500.0f, 500.0f), 0);
    SpyBehavior* spy = InstallOwnedSpy(*rts);

    auto mock = std::make_unique<MockGameUnit>(Vector2(500.0f, 500.0f), 0, 10.0f, 100.0f);
    mock->SetVelocity(Vector2(30.0f, 0.0f));
    MockGameUnit* mockPtr = mock.get();
    large.AddUnit(std::move(mock));

    const Vector2 mockStart = mockPtr->GetPosition();
  const Vector2 mockVelStart = mockPtr->GetVelocity();

    large.Update(0.5f);

    EXPECT_EQ(spy->generateFieldItemsCalls, 1);
    EXPECT_FLOAT_EQ(mockPtr->GetPosition().x, mockStart.x);
    EXPECT_FLOAT_EQ(mockPtr->GetPosition().y, mockStart.y);
    EXPECT_FLOAT_EQ(mockPtr->GetVelocity().x, mockVelStart.x);
    EXPECT_FLOAT_EQ(mockPtr->GetVelocity().y, mockVelStart.y);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, AiTickVisitsNonRtsBehaviorHost) {
    auto unit = std::make_unique<BehaviorHostMockGameUnit>(Vector2(50.0f, 50.0f), 0, 10.0f, 100.0f);
    unit->SetVelocity(Vector2(20.0f, 0.0f));
    auto spy = std::make_unique<SpyBehavior>();
    SpyBehavior* spyPtr = spy.get();
    unit->InstallSpy(std::move(spy));
    BehaviorHostMockGameUnit* ptr = unit.get();
    const Vector2 posStart = ptr->GetPosition();
    state.AddUnit(std::move(unit));

    state.Update(0.5f);

    EXPECT_EQ(spyPtr->generateFieldItemsCalls, 1);
    EXPECT_FLOAT_EQ(ptr->GetPosition().x, posStart.x + 20.0f * 0.5f);
    EXPECT_FLOAT_EQ(ptr->GetPosition().y, posStart.y);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, KinematicMockSkipsAiTickAndExplosionImpulse) {
    auto unit = std::make_unique<KinematicMockGameUnit>(Vector2(50.0f, 50.0f), 0, 10.0f, 100.0f);
    unit->SetVelocity(Vector2(40.0f, 0.0f));
    KinematicMockGameUnit* ptr = unit.get();
    const Vector2 posStart = ptr->GetPosition();
    const Vector2 velStart = ptr->GetVelocity();
    state.AddUnit(std::move(unit));

    state.Update(0.5f);

    EXPECT_FLOAT_EQ(ptr->GetPosition().x, posStart.x);
    EXPECT_FLOAT_EQ(ptr->GetPosition().y, posStart.y);
    EXPECT_FLOAT_EQ(ptr->GetVelocity().x, velStart.x);
    EXPECT_FLOAT_EQ(ptr->GetVelocity().y, velStart.y);

    state.AddExplosion(Explosion(Vector2(35.0f, 50.0f), 40.0f, 8000.0f));
    state.Update(0.25f);

    EXPECT_FLOAT_EQ(ptr->GetPosition().x, posStart.x);
    EXPECT_FLOAT_EQ(ptr->GetPosition().y, posStart.y);
    EXPECT_FLOAT_EQ(ptr->GetVelocity().x, velStart.x);
    EXPECT_FLOAT_EQ(ptr->GetVelocity().y, velStart.y);
}

TEST_F(RTSGameStateUpdateCharacterizationTest, DebugModeSkipsAiButMovesTowardDestination) {
    RTSUnit* unit = AddRts(state, Vector2(10.0f, 10.0f), 0);
    SpyBehavior* spy = InstallOwnedSpy(*unit);

    unit->SetDebugDestination(Vector2(80.0f, 10.0f));
    state.Update(0.5f);

    EXPECT_EQ(spy->generateFieldItemsCalls, 0);
    EXPECT_GT(unit->GetPosition().x, 10.0f);
    EXPECT_NEAR(unit->GetPosition().y, 10.0f, 1.0f);
}
