#include <gtest/gtest.h>

#include "core/influence/ClusterDetector.h"
#include "core/influence/EncirclementChecker.h"
#include "core/influence/SurroundChecker.h"
#include "core/influence/TerritoryCalculator.h"
#include "tests/TestHelpers/ProbeUnit.h"

TEST(ClusterDetectorTest, LinksSameTeamAcrossAWrappedSeam) {
    ProbeUnit left(Vector2(1.0f, 0.0f), 1);
    ProbeUnit right(Vector2(99.0f, 0.0f), 1);
    const std::vector<const IGameUnit*> units = {&left, &right};

    const auto open = ClusterDetector::Detect(units, 5.0f, false, 100.0f, 50.0f);
    EXPECT_EQ(open.size(), 2u);

    const auto wrapped = ClusterDetector::Detect(units, 5.0f, true, 100.0f, 50.0f);
    ASSERT_EQ(wrapped.size(), 1u);
    EXPECT_EQ(wrapped[0].size(), 2u);
}

TEST(ClusterDetectorTest, DoesNotLinkDifferentTeamsAndKeepsSingletons) {
    ProbeUnit a(Vector2(0.0f, 0.0f), 1);
    ProbeUnit b(Vector2(1.0f, 0.0f), 2);
    ProbeUnit c(Vector2(50.0f, 0.0f), 1);
    const auto clusters = ClusterDetector::Detect({&a, &b, &c}, 10.0f, false, 100.0f, 100.0f);
    EXPECT_EQ(clusters.size(), 3u);
}

TEST(EncirclementCheckerTest, OpenCenterIsNotEncircled) {
    ProbeUnit unit(Vector2(200.0f, 115.0f), 1);
    EncirclementChecker::Query query;
    query.clusterLinkDistance = 20.0f;
    EXPECT_FALSE(EncirclementChecker::IsEncircled({&unit}, {}, 400.0f, 230.0f, query));
}

TEST(EncirclementCheckerTest, FullObstacleMaskEncirclesAndIgnoredObstaclesDoNot) {
    ProbeUnit unit(Vector2(200.0f, 115.0f), 1);
    Obstacle wall;
    wall.x = 200.0f;
    wall.y = 115.0f;
    wall.radius = 10000.0f;
    wall.blocksInfluence = true;
    EncirclementChecker::Query query;
    query.clusterLinkDistance = 20.0f;
    query.obstacles = {wall};
    EXPECT_TRUE(EncirclementChecker::IsEncircled({&unit}, {}, 400.0f, 230.0f, query));

    query.obstacles[0].blocksInfluence = false;
    EXPECT_FALSE(EncirclementChecker::IsEncircled({&unit}, {}, 400.0f, 230.0f, query));
}

TEST(EncirclementCheckerTest, WrappedEnemyDiscClosesAEuclideanGap) {
    ProbeUnit unit(Vector2(5.0f, 15.0f), 1);
    ProbeUnit enemy(Vector2(48.0f, 15.0f), 2);
    enemy.SetInfluenceRadius(20.0f);
    EncirclementChecker::Query query;
    query.cols = 5;
    query.rows = 3;
    query.clusterLinkDistance = 4.0f;

    query.wrapBorders = false;
    EXPECT_FALSE(EncirclementChecker::IsEncircled({&unit}, {&enemy}, 50.0f, 30.0f, query));
    query.wrapBorders = true;
    EXPECT_TRUE(EncirclementChecker::IsEncircled({&unit}, {&enemy}, 50.0f, 30.0f, query));
}

TEST(SurroundCheckerTest, EnemyDiscAndObstaclesAndWrap) {
    ProbeUnit target(Vector2(50.0f, 50.0f), 1);
    target.SetInfluenceRadius(10.0f);
    ProbeUnit enemy(Vector2(50.0f, 50.0f), 2);
    enemy.SetInfluenceRadius(12.0f);
    const std::vector<const IGameUnit*> both = {&target, &enemy};
    EXPECT_TRUE(SurroundChecker::IsSurrounded(&target, both, {}, 100.0f, 100.0f, false));
    EXPECT_FALSE(SurroundChecker::IsSurrounded(&target, {&target}, {}, 100.0f, 100.0f, false));

    ProbeUnit friendUnit(Vector2(50.0f, 50.0f), 1);
    friendUnit.SetInfluenceRadius(12.0f);
    EXPECT_FALSE(SurroundChecker::IsSurrounded(&target, {&target, &enemy, &friendUnit}, {}, 100.0f, 100.0f, false));

    Obstacle cover;
    cover.x = 50.0f;
    cover.y = 50.0f;
    cover.radius = 12.0f;
    cover.blocksInfluence = true;
    EXPECT_TRUE(SurroundChecker::IsSurrounded(&target, {&target}, {cover}, 100.0f, 100.0f, false));

    ProbeUnit edge(Vector2(0.0f, 50.0f), 1);
    edge.SetInfluenceRadius(10.0f);
    EXPECT_FALSE(SurroundChecker::IsSurrounded(&edge, {&edge}, {}, 100.0f, 100.0f, true));
}

TEST(TerritoryGridTest, ActivePassiveContestedObstacleTimedCaptureAndWrap) {
    TerritoryCalculator calc;
    TerritoryCalculator::Arena arena{40.0f, 40.0f};
    TerritoryCalculator::GridUpdate update;
    update.cols = 4;
    update.rows = 4;
    update.captureTimeSeconds = 0.0f;

    ProbeUnit team3(Vector2(5.0f, 5.0f), 3);
    team3.SetInfluenceRadius(6.0f);
    calc.UpdateGrid({&team3}, {}, arena, update);
    const auto& claimed = calc.GetGrid()[0];
    EXPECT_EQ(claimed.owner, 3);
    EXPECT_EQ(claimed.state, TerritoryCalculator::TerritoryCellState::Active);

    calc.UpdateGrid({}, {}, arena, update);
    EXPECT_EQ(calc.GetGrid()[0].owner, 3);
    EXPECT_EQ(calc.GetGrid()[0].state, TerritoryCalculator::TerritoryCellState::Passive);

    ProbeUnit team1(Vector2(5.0f, 5.0f), 1);
    ProbeUnit team2(Vector2(5.0f, 5.0f), 2);
    team1.SetInfluenceRadius(6.0f);
    team2.SetInfluenceRadius(6.0f);
    calc.UpdateGrid({&team1, &team2}, {}, arena, update);
    EXPECT_EQ(calc.GetGrid()[0].state, TerritoryCalculator::TerritoryCellState::Contested);
    EXPECT_EQ(calc.GetGrid()[0].owner, 3);

    Obstacle mask;
    mask.x = 5.0f;
    mask.y = 5.0f;
    mask.radius = 1.0f;
    mask.blocksInfluence = true;
    calc.UpdateGrid({&team3}, {mask}, arena, update);
    EXPECT_EQ(calc.GetGrid()[0].state, TerritoryCalculator::TerritoryCellState::Obstacle);
    EXPECT_EQ(calc.GetGrid()[0].owner, 0);

    TerritoryCalculator timed;
    TerritoryCalculator::GridUpdate slow = update;
    slow.captureTimeSeconds = 1.0f;
    slow.dt = 0.4f;
    timed.UpdateGrid({&team3}, {}, arena, slow);
    EXPECT_EQ(timed.GetGrid()[0].owner, 0);
    EXPECT_NEAR(timed.GetGrid()[0].captureProgress, 0.4f, 1e-4f);
    EXPECT_EQ(timed.GetGrid()[0].state, TerritoryCalculator::TerritoryCellState::Active);
    slow.dt = 0.7f;
    timed.UpdateGrid({&team3}, {}, arena, slow);
    EXPECT_EQ(timed.GetGrid()[0].owner, 3);
    EXPECT_FLOAT_EQ(timed.GetGrid()[0].captureProgress, 1.0f);

    TerritoryCalculator wrapped;
    TerritoryCalculator::Arena belt{40.0f, 10.0f};
    TerritoryCalculator::GridUpdate seam;
    seam.cols = 4;
    seam.rows = 1;
    seam.wrapBorders = false;
    ProbeUnit seamUnit(Vector2(1.0f, 5.0f), 4);
    seamUnit.SetInfluenceRadius(8.0f);
    wrapped.UpdateGrid({&seamUnit}, {}, belt, seam);
    EXPECT_EQ(wrapped.GetGrid()[3].owner, 0);
    seam.wrapBorders = true;
    wrapped.UpdateGrid({&seamUnit}, {}, belt, seam);
    EXPECT_EQ(wrapped.GetGrid()[3].owner, 4);
    EXPECT_EQ(wrapped.GetGrid()[3].state, TerritoryCalculator::TerritoryCellState::Active);
}
