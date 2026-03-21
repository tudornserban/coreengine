#include <gtest/gtest.h>
#include "core/collision/SpatialGrid.h"
#include "tests/TestHelpers/MockGameUnit.h"
#include <algorithm>

static bool Contains(const std::vector<IGameUnit*>& v, IGameUnit* u) {
    return std::find(v.begin(), v.end(), u) != v.end();
}

static bool ContainsPair(const std::vector<std::pair<IGameUnit*, IGameUnit*>>& pairs,
                         IGameUnit* a, IGameUnit* b) {
    for (const auto& p : pairs)
        if ((p.first == a && p.second == b) || (p.first == b && p.second == a))
            return true;
    return false;
}

// ============================================================
// Clear
// ============================================================

TEST(SpatialGridTest, ClearRemovesAllUnits) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit unit(Vector2(50.0f, 50.0f));
    grid.Insert(&unit);
    grid.Clear();
    EXPECT_TRUE(grid.GetUnitsInCell(0, 0).empty());
}

// ============================================================
// Insert / GetUnitsInCell
// ============================================================

TEST(SpatialGridTest, InsertedUnitAppearsInCorrectCell) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit unit(Vector2(50.0f, 50.0f));
    grid.Insert(&unit);
    auto cells = grid.GetUnitsInCell(0, 0);
    ASSERT_EQ(cells.size(), 1u);
    EXPECT_EQ(cells[0], &unit);
}

TEST(SpatialGridTest, InsertNullIsNoOp) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    EXPECT_NO_THROW(grid.Insert(nullptr));
    EXPECT_TRUE(grid.GetUnitsInCell(0, 0).empty());
}

TEST(SpatialGridTest, UnitsInDifferentCellsAreSeparated) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit a(Vector2(50.0f, 50.0f));   // cell (0,0)
    MockGameUnit b(Vector2(150.0f, 50.0f));  // cell (1,0)
    grid.Insert(&a); grid.Insert(&b);
    EXPECT_EQ(grid.GetUnitsInCell(0, 0).size(), 1u);
    EXPECT_EQ(grid.GetUnitsInCell(1, 0).size(), 1u);
}

TEST(SpatialGridTest, OutOfBoundsCellReturnsEmpty) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    EXPECT_TRUE(grid.GetUnitsInCell(-1, 0).empty());
    EXPECT_TRUE(grid.GetUnitsInCell(0, -1).empty());
    EXPECT_TRUE(grid.GetUnitsInCell(999, 999).empty());
}

// ============================================================
// GetPotentialPairs
// ============================================================

TEST(SpatialGridTest, NoPairsWhenEmpty) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    EXPECT_TRUE(grid.GetPotentialPairs().empty());
}

TEST(SpatialGridTest, NoPairsForSingleUnit) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit unit(Vector2(50.0f, 50.0f));
    grid.Insert(&unit);
    EXPECT_TRUE(grid.GetPotentialPairs().empty());
}

TEST(SpatialGridTest, PairForTwoUnitsInSameCell) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit a(Vector2(20.0f, 20.0f));
    MockGameUnit b(Vector2(80.0f, 80.0f));  // both in cell (0,0)
    grid.Insert(&a); grid.Insert(&b);
    EXPECT_TRUE(ContainsPair(grid.GetPotentialPairs(), &a, &b));
}

TEST(SpatialGridTest, PairForAdjacentCells) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit a(Vector2(90.0f, 50.0f));   // cell (0,0)
    MockGameUnit b(Vector2(110.0f, 50.0f));  // cell (1,0)
    grid.Insert(&a); grid.Insert(&b);
    EXPECT_TRUE(ContainsPair(grid.GetPotentialPairs(), &a, &b));
}

TEST(SpatialGridTest, NoDuplicatePairs) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit a(Vector2(20.0f, 20.0f));
    MockGameUnit b(Vector2(80.0f, 80.0f));
    grid.Insert(&a); grid.Insert(&b);
    int count = 0;
    for (const auto& p : grid.GetPotentialPairs())
        if ((p.first == &a && p.second == &b) || (p.first == &b && p.second == &a))
            ++count;
    EXPECT_EQ(count, 1);
}

TEST(SpatialGridTest, DistantUnitsNotPaired) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit a(Vector2(50.0f, 50.0f));    // cell (0,0)
    MockGameUnit b(Vector2(950.0f, 950.0f));  // cell (9,9)
    grid.Insert(&a); grid.Insert(&b);
    EXPECT_FALSE(ContainsPair(grid.GetPotentialPairs(), &a, &b));
}

// ============================================================
// GetUnitsNear
// ============================================================

TEST(SpatialGridTest, GetUnitsNearFindsClose) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit unit(Vector2(50.0f, 50.0f));
    grid.Insert(&unit);
    EXPECT_TRUE(Contains(grid.GetUnitsNear(Vector2(60.0f, 60.0f), 50.0f), &unit));
}

TEST(SpatialGridTest, GetUnitsNearExcludesFarUnits) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    MockGameUnit unit(Vector2(500.0f, 500.0f));
    grid.Insert(&unit);
    EXPECT_FALSE(Contains(grid.GetUnitsNear(Vector2(0.0f, 0.0f), 50.0f), &unit));
}

TEST(SpatialGridTest, GetUnitsNearEmptyGridIsEmpty) {
    SpatialGrid grid(100.0f, Vector2(0.0f, 0.0f), Vector2(1000.0f, 1000.0f));
    EXPECT_TRUE(grid.GetUnitsNear(Vector2(500.0f, 500.0f), 200.0f).empty());
}
