#include <gtest/gtest.h>

#include "core/collision/CollisionSystem.h"
#include "core/collision/CollisionEvent.h"
#include "core/collision/ICollisionDetector.h"
#include "core/collision/ICollisionObserver.h"
#include "tests/TestHelpers/KinematicMockGameUnit.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockGameUnit.h"

namespace {

class RecordingCollisionObserver : public ICollisionObserver {
public:
    void OnCollision(const CollisionEvent& event) override {
        ++eventCount;
        lastDamageModifier = event.damageModifier;
    }

    int eventCount = 0;
    float lastDamageModifier = 0.0f;
};

}  // namespace

TEST(CollisionSystemKinematicsGateTest, agent_agent_skips_when_either_side_lacks_iunit_kinematics) {
    CollisionSystem system;
    RecordingCollisionObserver observer;
    system.Subscribe(&observer);

    // Overlapping circles, far from map edges (edge detection can emit events even without kinematics).
    MockGameUnit a(Vector2(2000.0f, 2000.0f), 0, 10.0f, 100.0f);
    MockGameUnit b(Vector2(2015.0f, 2000.0f), 0, 10.0f, 100.0f);

    MapBounds bounds(Vector2(1000.0f, 1000.0f), Vector2(3000.0f, 3000.0f));
    std::vector<IGameUnit*> units = {&a, &b};

    system.ProcessCollisions(units, bounds);

    EXPECT_EQ(observer.eventCount, 0);
}

TEST(CollisionSystemKinematicsGateTest, map_edge_emits_for_mock_without_iunit_kinematics) {
    CollisionSystem system;
    RecordingCollisionObserver observer;
    system.Subscribe(&observer);

    // Past the left edge; MockGameUnit does not implement IUnitKinematics.
    MockGameUnit unit(Vector2(-5.0f, 50.0f), 0, 10.0f, 100.0f);

    MapBounds bounds(Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f));
    std::vector<IGameUnit*> units = {&unit};

    Vector2 posBefore = unit.GetPosition();
    system.ProcessCollisions(units, bounds);

    EXPECT_EQ(observer.eventCount, 1);
    EXPECT_FLOAT_EQ(observer.lastDamageModifier, unit.GetDamageModifier());
    EXPECT_EQ(unit.GetPosition().x, posBefore.x);
    EXPECT_EQ(unit.GetPosition().y, posBefore.y);
}

TEST(CollisionSystemKinematicsGateTest, map_edge_resolves_for_kinematic_unit) {
    CollisionSystem system;
    RecordingCollisionObserver observer;
    system.Subscribe(&observer);

    // Center left of the map so the circle sticks out past x=0 (map-edge contact).
    KinematicMockGameUnit unit(Vector2(-5.0f, 50.0f), 0, 10.0f, 100.0f);

    MapBounds bounds(Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f));
    std::vector<IGameUnit*> units = {&unit};

    system.ProcessCollisions(units, bounds);

    EXPECT_EQ(observer.eventCount, 1);
    EXPECT_FLOAT_EQ(observer.lastDamageModifier, unit.GetDamageModifier());
}
