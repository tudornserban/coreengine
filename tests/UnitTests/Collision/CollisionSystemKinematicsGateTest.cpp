#include <gtest/gtest.h>

#include "core/collision/CollisionSystem.h"
#include "core/collision/CollisionEvent.h"
#include "core/collision/ICollisionDetector.h"
#include "core/collision/ICollisionObserver.h"
#include "core/gamestate/RTSUnit.h"
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

TEST(CollisionSystemKinematicsGateTest, skips_resolve_when_unit_lacks_iunit_kinematics) {
    CollisionSystem system;
    RecordingCollisionObserver observer;
    system.Subscribe(&observer);

    // Overlapping circles: would produce a contact, but MockGameUnit is not IUnitKinematics.
    MockGameUnit a(Vector2(0.0f, 0.0f), 0, 10.0f, 100.0f);
    MockGameUnit b(Vector2(15.0f, 0.0f), 0, 10.0f, 100.0f);

    MapBounds bounds(Vector2(0.0f, 0.0f), Vector2(200.0f, 200.0f));
    std::vector<IGameUnit*> units = {&a, &b};

    system.ProcessCollisions(units, bounds);

    EXPECT_EQ(observer.eventCount, 0);
}

TEST(CollisionSystemKinematicsGateTest, map_edge_resolves_for_rts_unit) {
    CollisionSystem system;
    RecordingCollisionObserver observer;
    system.Subscribe(&observer);

    // Center left of the map so the circle sticks out past x=0 (map-edge contact).
    RTSUnit unit(Vector2(-5.0f, 50.0f), 0, 10.0f, 100.0f);

    MapBounds bounds(Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f));
    std::vector<IGameUnit*> units = {&unit};

    system.ProcessCollisions(units, bounds);

    EXPECT_EQ(observer.eventCount, 1);
    EXPECT_FLOAT_EQ(observer.lastDamageModifier, unit.GetDamageModifier());
}
