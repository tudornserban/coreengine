#include <gtest/gtest.h>
#include "core/map/Map.h"
#include "core/map/MapEntity.h"

// ============================================================
// Construction / basic getters
// ============================================================

TEST(MapTest, ConstructionStoresDimensions) {
    Map map(Vector2(800.0f, 600.0f));
    EXPECT_FLOAT_EQ(map.GetDimensions().x, 800.0f);
    EXPECT_FLOAT_EQ(map.GetDimensions().y, 600.0f);
}

TEST(MapTest, CenterIsHalfOfDimensions) {
    Map map(Vector2(800.0f, 600.0f));
    EXPECT_FLOAT_EQ(map.GetCenter().x, 400.0f);
    EXPECT_FLOAT_EQ(map.GetCenter().y, 300.0f);
}

TEST(MapTest, DefaultShapeIsRectangle) {
    Map map(Vector2(1000.0f, 1000.0f));
    EXPECT_EQ(map.GetShape(), MapShape::RECTANGLE);
}

TEST(MapTest, IdAndNameStoredOnConstruction) {
    Map map(Vector2(100.0f, 100.0f), MapShape::RECTANGLE, "map_01", "Arena");
    EXPECT_EQ(map.GetMapId(), "map_01");
    EXPECT_EQ(map.GetMapName(), "Arena");
}

TEST(MapTest, SetDimensions) {
    Map map(Vector2(100.0f, 100.0f));
    map.SetDimensions(Vector2(500.0f, 300.0f));
    EXPECT_FLOAT_EQ(map.GetDimensions().x, 500.0f);
    EXPECT_FLOAT_EQ(map.GetDimensions().y, 300.0f);
}

// ============================================================
// IsValidPosition
// ============================================================

TEST(MapTest, CenterIsValidPosition) {
    Map map(Vector2(1000.0f, 1000.0f));
    EXPECT_TRUE(map.IsValidPosition(Vector2(500.0f, 500.0f)));
}

TEST(MapTest, OriginIsValidPosition) {
    Map map(Vector2(1000.0f, 1000.0f));
    EXPECT_TRUE(map.IsValidPosition(Vector2(0.0f, 0.0f)));
}

TEST(MapTest, NegativePositionIsInvalid) {
    Map map(Vector2(1000.0f, 1000.0f));
    EXPECT_FALSE(map.IsValidPosition(Vector2(-1.0f, 0.0f)));
    EXPECT_FALSE(map.IsValidPosition(Vector2(0.0f, -1.0f)));
}

TEST(MapTest, OutOfBoundsPositionIsInvalid) {
    Map map(Vector2(100.0f, 100.0f));
    EXPECT_FALSE(map.IsValidPosition(Vector2(101.0f, 50.0f)));
    EXPECT_FALSE(map.IsValidPosition(Vector2(50.0f, 101.0f)));
}

// ============================================================
// Entity management
// ============================================================

TEST(MapTest, AddEntityAndRetrieve) {
    Map map(Vector2(1000.0f, 1000.0f));
    auto entity = std::make_unique<MapEntity>("e1", Vector2(100.0f, 100.0f));
    map.AddEntity(std::move(entity));
    EXPECT_NE(map.GetEntity("e1"), nullptr);
}

TEST(MapTest, GetNonExistentEntityIsNull) {
    Map map(Vector2(1000.0f, 1000.0f));
    EXPECT_EQ(map.GetEntity("nonexistent"), nullptr);
}

TEST(MapTest, RemoveEntityLeavesOthersIntact) {
    Map map(Vector2(1000.0f, 1000.0f));
    map.AddEntity(std::make_unique<MapEntity>("a", Vector2(10.0f, 10.0f)));
    map.AddEntity(std::make_unique<MapEntity>("b", Vector2(20.0f, 20.0f)));
    map.RemoveEntity("a");
    EXPECT_EQ(map.GetEntity("a"), nullptr);
    EXPECT_NE(map.GetEntity("b"), nullptr);
}

TEST(MapTest, GetEntitiesCountIsCorrect) {
    Map map(Vector2(1000.0f, 1000.0f));
    map.AddEntity(std::make_unique<MapEntity>("x", Vector2(0.0f, 0.0f)));
    map.AddEntity(std::make_unique<MapEntity>("y", Vector2(1.0f, 0.0f)));
    EXPECT_EQ(map.GetEntities().size(), 2u);
}

TEST(MapTest, GetEntitiesInRadiusFindsClose) {
    Map map(Vector2(1000.0f, 1000.0f));
    map.AddEntity(std::make_unique<MapEntity>("near", Vector2(50.0f, 50.0f)));
    map.AddEntity(std::make_unique<MapEntity>("far",  Vector2(900.0f, 900.0f)));
    auto found = map.GetEntitiesInRadius(Vector2(50.0f, 50.0f), 100.0f);
    bool hasNear = false;
    bool hasFar  = false;
    for (auto* e : found) {
        if (e->GetId() == "near") hasNear = true;
        if (e->GetId() == "far")  hasFar  = true;
    }
    EXPECT_TRUE(hasNear);
    EXPECT_FALSE(hasFar);
}
