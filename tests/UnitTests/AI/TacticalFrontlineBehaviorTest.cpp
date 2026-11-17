#include <gtest/gtest.h>

#include "core/ai/BehaviorFactory.h"
#include "core/ai/Behaviors/TacticalFrontlineBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "tests/TestHelpers/MockGameUnit.h"

namespace {

BehaviorContext ContextFor(IGameUnit* self) {
    BehaviorContext ctx;
    ctx.self = self;
    ctx.deltaTime = 1.0f;
    ctx.mapSize = Vector2(200.0f, 200.0f);
    return ctx;
}

}  // namespace

TEST(TacticalFrontlineBehaviorTest, PushesAwayFromAlliesAndHarderUpClose) {
    MockGameUnit self(Vector2(0.0f, 0.0f), 1, 10.0f, 10000.0f);
    MockGameUnit close(Vector2(10.0f, 0.0f), 1, 10.0f, 10000.0f);
    MockGameUnit far(Vector2(70.0f, 0.0f), 1, 10.0f, 10000.0f);
    TacticalFrontlineBehavior behavior;
    behavior.GetParameters()["enemyAvoidStrength"] = 0.0f;
    behavior.GetParameters()["clusterAttractStrength"] = 0.0f;

    BehaviorContext nearCtx = ContextFor(&self);
    nearCtx.nearbyAllies = {&close};
    const float nearMag = behavior.CalculateAcceleration(nearCtx).Length();

    BehaviorContext farCtx = ContextFor(&self);
    farCtx.nearbyAllies = {&far};
    const Vector2 farAccel = behavior.CalculateAcceleration(farCtx);
    EXPECT_LT(farAccel.x, 0.0f);
    EXPECT_GT(nearMag, farAccel.Length());
}

TEST(TacticalFrontlineBehaviorTest, AvoidsEnemiesAndAttractsToTheLocalCluster) {
    MockGameUnit self(Vector2(0.0f, 0.0f), 1);
    MockGameUnit enemy(Vector2(40.0f, 0.0f), 2);
    TacticalFrontlineBehavior avoid;
    avoid.GetParameters()["allyAvoidStrength"] = 0.0f;
    avoid.GetParameters()["allyRepulsionStrength"] = 0.0f;
    avoid.GetParameters()["clusterAttractStrength"] = 0.0f;
    BehaviorContext avoidCtx = ContextFor(&self);
    avoidCtx.nearbyEnemies = {&enemy};
    EXPECT_LT(avoid.CalculateAcceleration(avoidCtx).x, 0.0f);

    TacticalFrontlineBehavior attract;
    attract.GetParameters()["allyAvoidStrength"] = 0.0f;
    attract.GetParameters()["allyRepulsionStrength"] = 0.0f;
    attract.GetParameters()["enemyAvoidStrength"] = 0.0f;
    MockGameUnit distant(Vector2(100.0f, 0.0f), 2);
    BehaviorContext attractCtx = ContextFor(&self);
    attractCtx.nearbyEnemies = {&distant};
    EXPECT_GT(attract.CalculateAcceleration(attractCtx).x, 0.0f);
}

TEST(TacticalFrontlineBehaviorTest, FactoryRegistersTheBuiltin) {
    BehaviorFactory::RegisterBuiltins();
    auto behavior = BehaviorFactory::Create("TacticalFrontline");
    ASSERT_NE(behavior, nullptr);
    EXPECT_STREQ(behavior->GetName(), "TacticalFrontline");
}
