#include <gtest/gtest.h>
#include "core/influence/InfluenceFieldItem.h"
#include "core/influence/InfluenceFieldCalculator.h"
#include "core/influence/InfluenceFieldParameters.h"
#include "core/influence/TerritoryCalculator.h"
#include "tests/TestHelpers/MockGameUnit.h"

// ============================================================
// InfluenceFieldItem — factory methods and accessors
// ============================================================

TEST(InfluenceFieldItemTest, CreateAttractorType) {
    auto item = InfluenceFieldItem::CreateAttractor(Vector2(100.0f, 0.0f), 2.0f);
    EXPECT_EQ(item.GetInfluenceType(), InfluenceType::ATTRACTOR);
    EXPECT_FLOAT_EQ(item.GetImportance(), 2.0f);
    EXPECT_FLOAT_EQ(item.GetPosition().x, 100.0f);
}

TEST(InfluenceFieldItemTest, CreateRepulsorType) {
    auto item = InfluenceFieldItem::CreateRepulsor(Vector2(0.0f, 50.0f), 1.5f);
    EXPECT_EQ(item.GetInfluenceType(), InfluenceType::REPULSOR);
    EXPECT_FLOAT_EQ(item.GetImportance(), 1.5f);
}

TEST(InfluenceFieldItemTest, CreateEdgeType) {
    auto item = InfluenceFieldItem::CreateEdge(Vector2(0.0f, 0.0f));
    EXPECT_EQ(item.GetInfluenceType(), InfluenceType::EDGE);
}

TEST(InfluenceFieldItemTest, SetPosition) {
    InfluenceFieldItem item;
    item.SetPosition(Vector2(7.0f, 3.0f));
    EXPECT_FLOAT_EQ(item.GetPosition().x, 7.0f);
    EXPECT_FLOAT_EQ(item.GetPosition().y, 3.0f);
}

TEST(InfluenceFieldItemTest, SetVelocity) {
    InfluenceFieldItem item;
    item.SetVelocity(Vector2(1.0f, -1.0f));
    EXPECT_FLOAT_EQ(item.GetVelocity().x,  1.0f);
    EXPECT_FLOAT_EQ(item.GetVelocity().y, -1.0f);
}

TEST(InfluenceFieldItemTest, SetImportance) {
    InfluenceFieldItem item;
    item.SetImportance(5.0f);
    EXPECT_FLOAT_EQ(item.GetImportance(), 5.0f);
}

// ============================================================
// InfluenceFieldCalculator — directional sanity checks
// ============================================================

TEST(InfluenceFieldCalculatorTest, AttractorPullsAgentToward) {
    // Agent at origin, attractor to the right — force should be in +x direction
    Vector2 agentPos(0.0f, 0.0f);
    Vector2 agentVel(0.0f, 0.0f);
    auto attractor = InfluenceFieldItem::CreateAttractor(Vector2(300.0f, 0.0f), 1.0f);

    Vector2 force = InfluenceField::CalculateAttractorForce(agentPos, agentVel, attractor);
    EXPECT_GT(force.x, 0.0f);
}

TEST(InfluenceFieldCalculatorTest, RepulsorPushesAgentAway) {
    // Agent at (0,0), repulsor at (30,0) within default detection range — force in -x
    Vector2 agentPos(0.0f, 0.0f);
    Vector2 agentVel(0.0f, 0.0f);
    auto repulsor = InfluenceFieldItem::CreateRepulsor(Vector2(30.0f, 0.0f), 1.0f);

    Vector2 force = InfluenceField::CalculateRepulsorForce(agentPos, agentVel, repulsor);
    EXPECT_LT(force.x, 0.0f);
}

TEST(InfluenceFieldCalculatorTest, AttractorAtAgentPositionProducesZeroOrSmallForce) {
    Vector2 agentPos(100.0f, 100.0f);
    auto attractor = InfluenceFieldItem::CreateAttractor(Vector2(100.0f, 100.0f), 1.0f);
    Vector2 force = InfluenceField::CalculateAttractorForce(agentPos, Vector2::Zero(), attractor);
    // Distance == 0 should produce zero or near-zero force
    EXPECT_NEAR(force.Length(), 0.0f, 1.0f);
}

TEST(InfluenceFieldCalculatorTest, TotalForceClampedToMax) {
    Vector2 agentPos(0.0f, 0.0f);
    Vector2 agentVel(0.0f, 0.0f);
    float maxAccel = 50.0f;

    std::vector<InfluenceFieldItem> items;
    // Many strong attractors in the same direction
    for (int i = 0; i < 10; ++i)
        items.push_back(InfluenceFieldItem::CreateAttractor(Vector2(500.0f, 0.0f), 10.0f));

    Vector2 total = InfluenceField::CalculateTotalForce(agentPos, agentVel, items, maxAccel);
    EXPECT_LE(total.Length(), maxAccel + 1e-3f);
}

TEST(InfluenceFieldCalculatorTest, EmptyItemsProducesZeroForce) {
    Vector2 force = InfluenceField::CalculateTotalForce(
        Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f), {}, 300.0f);
    EXPECT_FLOAT_EQ(force.Length(), 0.0f);
}

// ============================================================
// TerritoryCalculator
// ============================================================

TEST(TerritoryCalculatorTest, EqualUnitsYieldApproximatelyFiftyFifty) {
    TerritoryCalculator calc;
    TerritoryCalculator::Arena arena{1000.0f, 1000.0f};

    // Team 1 anchored on the left, team 2 on the right
    MockGameUnit t1(Vector2(100.0f, 500.0f), 1);
    MockGameUnit t2(Vector2(900.0f, 500.0f), 2);

    std::vector<IGameUnit*> units = {&t1, &t2};
    auto result = calc.Calculate(units, arena);

    EXPECT_NEAR(result.team1Pct, 50.0f, 10.0f);
    EXPECT_NEAR(result.team2Pct, 50.0f, 10.0f);
}

TEST(TerritoryCalculatorTest, AllUnitsTeam1OwnsAll) {
    TerritoryCalculator calc;
    TerritoryCalculator::Arena arena{1000.0f, 1000.0f};

    MockGameUnit t1a(Vector2(250.0f, 500.0f), 1);
    MockGameUnit t1b(Vector2(750.0f, 500.0f), 1);
    std::vector<IGameUnit*> units = {&t1a, &t1b};

    auto result = calc.Calculate(units, arena);
    EXPECT_FLOAT_EQ(result.team2Pct, 0.0f);
    EXPECT_NEAR(result.team1Pct, 100.0f, 1.0f);
}

TEST(TerritoryCalculatorTest, PercentagesSumToOneHundred) {
    TerritoryCalculator calc;
    TerritoryCalculator::Arena arena{500.0f, 500.0f};

    MockGameUnit t1(Vector2(100.0f, 250.0f), 1);
    MockGameUnit t2(Vector2(400.0f, 250.0f), 2);
    std::vector<IGameUnit*> units = {&t1, &t2};

    auto result = calc.Calculate(units, arena);
    EXPECT_NEAR(result.team1Pct + result.team2Pct, 100.0f, 1.0f);
}

TEST(TerritoryCalculatorTest, EmptyUnitListYieldZeroPercentages) {
    TerritoryCalculator calc;
    TerritoryCalculator::Arena arena{1000.0f, 1000.0f};
    auto result = calc.Calculate({}, arena);
    EXPECT_FLOAT_EQ(result.team1Pct, 0.0f);
    EXPECT_FLOAT_EQ(result.team2Pct, 0.0f);
}
