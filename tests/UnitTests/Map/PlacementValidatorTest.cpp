#include <gtest/gtest.h>

#include "core/influence_types/InfluenceFieldItem.h"
#include "core/map/MapComponents/MapObstacleComponent.h"
#include "core/map/PlacementValidator.h"
#include "tests/TestHelpers/ProbeUnit.h"

namespace {

InfluenceFieldItem DummyField() {
    return InfluenceFieldItem::CreateRepulsor(Vector2::Zero(), 1.0f);
}

}  // namespace

TEST(PlacementValidatorTest, RejectsPointsOutsideAClampedArena) {
    PlacementValidator validator;
    PlacementValidator::Rules rules;
    EXPECT_FALSE(validator.IsValidPlacement(-1.0f, 10.0f, 1, {}, {}, 100.0f, 80.0f, rules));
    EXPECT_TRUE(validator.IsValidPlacement(0.0f, 10.0f, 1, {}, {}, 100.0f, 80.0f, rules));
}

TEST(PlacementValidatorTest, WrapFoldsOutOfRangeCandidatesIntoTheArena) {
    PlacementValidator validator;
    PlacementValidator::Rules rules;
    rules.wrapBorders = true;
    EXPECT_TRUE(validator.IsValidPlacement(-5.0f, 10.0f, 1, {}, {}, 100.0f, 80.0f, rules));
}

TEST(PlacementValidatorTest, PlacementObstacleRejectsItsDisc) {
    PlacementValidator validator;
    Obstacle wall;
    wall.x = 10.0f;
    wall.y = 10.0f;
    wall.radius = 5.0f;
    wall.blocksPlacement = true;
    PlacementValidator::Rules rules;
    EXPECT_FALSE(validator.IsValidPlacement(12.0f, 10.0f, 1, {}, {wall}, 100.0f, 80.0f, rules));
    EXPECT_TRUE(validator.IsValidPlacement(20.0f, 10.0f, 1, {}, {wall}, 100.0f, 80.0f, rules));

    wall.blocksPlacement = false;
    EXPECT_TRUE(validator.IsValidPlacement(12.0f, 10.0f, 1, {}, {wall}, 100.0f, 80.0f, rules));
}

TEST(PlacementValidatorTest, SameTeamUsesBaseSeparationAndEnemiesCanUseInfluence) {
    PlacementValidator validator;
    ProbeUnit ally(Vector2(0.0f, 0.0f), 1);
    ProbeUnit enemy(Vector2(0.0f, 0.0f), 2);
    enemy.SetInfluenceRadius(30.0f);
    PlacementValidator::Rules rules;
    rules.baseSeparationRadius = 10.0f;

    const std::vector<const IGameUnit*> units = {&ally, &enemy};
    EXPECT_FALSE(validator.IsValidPlacement(5.0f, 0.0f, 1, units, {}, 200.0f, 200.0f, rules));
    EXPECT_TRUE(validator.IsValidPlacement(10.0f, 0.0f, 1, units, {}, 200.0f, 200.0f, rules));

    rules.enemyClearanceUsesInfluence = true;
    EXPECT_FALSE(validator.IsValidPlacement(20.0f, 0.0f, 1, units, {}, 200.0f, 200.0f, rules));
    EXPECT_TRUE(validator.IsValidPlacement(30.0f, 0.0f, 1, units, {}, 200.0f, 200.0f, rules));
}

TEST(PlacementValidatorTest, MinDistanceAndTerritoryPredicateAndDeadUnits) {
    PlacementValidator validator;
    ProbeUnit enemy(Vector2(0.0f, 0.0f), 2);
    enemy.SetHP(0.0f);
    PlacementValidator::Rules rules;
    rules.minPlacementDistance = 15.0f;
    const std::vector<const IGameUnit*> dead = {&enemy};
    EXPECT_TRUE(validator.IsValidPlacement(10.0f, 0.0f, 1, dead, {}, 100.0f, 100.0f, rules));

    ProbeUnit living(Vector2(0.0f, 0.0f), 2);
    const std::vector<const IGameUnit*> units = {&living};
    EXPECT_FALSE(validator.IsValidPlacement(10.0f, 0.0f, 1, units, {}, 100.0f, 100.0f, rules));

    rules.minPlacementDistance = 0.0f;
    rules.territoryFilter = [](float, float, int) { return false; };
    EXPECT_FALSE(validator.IsValidPlacement(40.0f, 40.0f, 1, {}, {}, 100.0f, 100.0f, rules));
    rules.territoryFilter = [](float x, float, int teamId) { return x >= 10.0f && teamId == 1; };
    EXPECT_TRUE(validator.IsValidPlacement(10.0f, 10.0f, 1, {}, {}, 100.0f, 100.0f, rules));
}

TEST(MapObstacleComponentTest, ToObstacleCopiesFlagsAndWorldPosition) {
    MapObstacleComponent component(12.0f, DummyField());
    component.SetObstacleId(7);
    component.SetBlocksPlacement(false);
    component.SetBlocksInfluence(true);
    component.SetKind(ObstacleKind::Wall);
    const Obstacle obstacle = component.ToObstacle(Vector2(3.0f, 4.0f));
    EXPECT_EQ(obstacle.id, 7);
    EXPECT_FLOAT_EQ(obstacle.x, 3.0f);
    EXPECT_FLOAT_EQ(obstacle.y, 4.0f);
    EXPECT_FLOAT_EQ(obstacle.radius, 12.0f);
    EXPECT_FALSE(obstacle.blocksPlacement);
    EXPECT_TRUE(obstacle.blocksInfluence);
    EXPECT_EQ(obstacle.kind, ObstacleKind::Wall);
    EXPECT_TRUE(obstacle.ContainsPoint(3.0f, 4.0f));
}
