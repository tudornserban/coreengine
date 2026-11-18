#include <gtest/gtest.h>

#include "core/combat/ProjectileSimulation.h"
#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/IUnitCombat.h"
#include "core/math/Vector2.h"

namespace {

class Body : public IGameUnit, public IUnitCombat {
public:
    Body(Vector2 position, Vector2 velocity, int team, float radius, uint64_t id)
        : position_(position), velocity_(velocity), team_(team), radius_(radius), id_(id) {}

    void Update(float) override {}
    Vector2 GetPosition() const override { return position_; }
    Vector2 GetVelocity() const override { return velocity_; }
    void ApplyAcceleration(Vector2, float) override {}
    int GetTeam() const override { return team_; }
    float GetRadius() const override { return radius_; }
    float GetMaxSpeed() const override { return 0.0f; }
    float GetCurrentHP() const override { return hp_; }
    float GetMaxHP() const override { return 100.0f; }
    uint64_t GetEntityId() const override { return id_; }
    void TakeDamage(float amount) override { ++damageCalls; hp_ -= std::max(0.0f, amount - armor); }
    bool ParticipatesInCollision() const override { return enabled; }
    bool enabled = true;
    float armor = 0;
    int damageCalls = 0;
    bool IsDead() const override { return hp_ <= 0.0f; }

private:
    Vector2 position_;
    Vector2 velocity_;
    int team_;
    float radius_;
    uint64_t id_;
    float hp_ = 100.0f;
};

}  // namespace

TEST(ProjectileSimulationTest, FastProjectileDoesNotTunnelAndExpires) {
    ProjectileSimulation simulation;
    Body target(Vector2(100.0f, 0.0f), Vector2::Zero(), 2, 10.0f, 4);
    Projectile shot;
    shot.ownerId = 1;
    shot.team = 1;
    shot.position = Vector2(0.0f, 0.0f);
    shot.velocity = Vector2(10000.0f, 0.0f);
    shot.radius = 1.0f;
    shot.timeRemaining = 1.0f;
    shot.damage = 15.0f;
    simulation.Spawn(shot);

    const auto impacts = simulation.Advance(0.016f, {&target});
    ASSERT_EQ(impacts.size(), 1u);
    EXPECT_EQ(impacts[0].targetId, 4u);
    EXPECT_FLOAT_EQ(target.GetCurrentHP(), 85.0f);
    EXPECT_TRUE(simulation.Projectiles().empty());
}

TEST(ProjectileSimulationTest, ImpactOrderIsTimeThenIdsAndAlliesAreSkipped) {
    ProjectileSimulation simulation;
    Body ally(Vector2(10.0f, 0.0f), Vector2::Zero(), 1, 2.0f, 2);
    Body far(Vector2(30.0f, 0.0f), Vector2::Zero(), 2, 2.0f, 9);
    Body near(Vector2(12.0f, 0.0f), Vector2::Zero(), 2, 2.0f, 3);
    Projectile first;
    first.team = 1;
    first.ownerId = 1;
    first.position = Vector2(0.0f, 0.0f);
    first.velocity = Vector2(100.0f, 0.0f);
    first.radius = 1.0f;
    first.timeRemaining = 1.0f;
    first.damage = 1.0f;
    const uint64_t firstId = simulation.Spawn(first);
    Projectile second = first;
    second.position = Vector2(0.0f, 0.0f);
    second.velocity = Vector2(50.0f, 0.0f);
    const uint64_t secondId = simulation.Spawn(second);

    const auto impacts = simulation.Advance(1.0f, {&ally, &far, &near});
    ASSERT_EQ(impacts.size(), 2u);
    EXPECT_EQ(impacts[0].projectileId, firstId);
    EXPECT_EQ(impacts[0].targetId, 3u);
    EXPECT_EQ(impacts[1].projectileId, secondId);
    EXPECT_EQ(impacts[1].targetId, 3u);
    EXPECT_FLOAT_EQ(ally.GetCurrentHP(), 100.0f);
}

TEST(ProjectileSimulationTest, InterceptIsIndependentOfFiring) {
    const InterceptSolution solution = ProjectileSimulation::Intercept(
        Vector2::Zero(), 10.0f, Vector2(10.0f, 0.0f), Vector2::Zero());
    EXPECT_TRUE(solution.hit);
    EXPECT_FLOAT_EQ(solution.time, 1.0f);
    EXPECT_FLOAT_EQ(solution.point.x, 10.0f);
}

TEST(ProjectileSimulationTest, ExpiredShotIsRemovedWithoutImpact) {
    ProjectileSimulation simulation;
    Body target(Vector2(1000.0f, 0.0f), Vector2::Zero(), 2, 1.0f, 5);
    Projectile shot;
    shot.team = 1;
    shot.position = Vector2::Zero();
    shot.velocity = Vector2(1.0f, 0.0f);
    shot.timeRemaining = 0.01f;
    shot.damage = 5.0f;
    simulation.Spawn(shot);
    const auto impacts = simulation.Advance(0.05f, {&target});
    EXPECT_TRUE(impacts.empty());
    EXPECT_TRUE(simulation.Projectiles().empty());
    EXPECT_FLOAT_EQ(target.GetCurrentHP(), 100.0f);
}

TEST(ProjectileSimulationTest, EligibilityDamageAttributionAndSequentialKills) {
    ProjectileSimulation simulation;
    Body dead({10, 0}, {}, 2, 1, 2);
    dead.TakeDamage(100);
    Body disabled({20, 0}, {}, 2, 1, 3);
    disabled.enabled = false;
    Body near({30, 0}, {}, 2, 1, 4);
    near.armor = 10;
    Body far({40, 0}, {}, 2, 1, 5);
    Projectile shot;
    shot.ownerId = 77;
    shot.team = 1;
    shot.velocity = {100, 0};
    shot.timeRemaining = 1;
    shot.damage = 150;
    simulation.Spawn(shot);
    simulation.Spawn(shot);
    simulation.Advance(1, {&dead, &disabled, &near, &far});
    const auto events = simulation.LastImpacts();
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].targetId, 4u);
    EXPECT_EQ(events[1].targetId, 5u);
    EXPECT_EQ(events[0].ownerId, 77u);
    EXPECT_EQ(events[0].team, 1);
    EXPECT_FLOAT_EQ(events[0].requestedDamage, 150);
    EXPECT_FLOAT_EQ(events[0].appliedDamage, 100);
    EXPECT_EQ(near.damageCalls, 1);
    EXPECT_EQ(far.damageCalls, 1);
    EXPECT_EQ(disabled.damageCalls, 0);
    simulation.Advance(1, {});
    EXPECT_TRUE(simulation.LastImpacts().empty());
    EXPECT_EQ(events[0].targetId, 4u);
}

TEST(ProjectileSimulationTest, ReportsDamageAfterArmorExactlyOnce) {
    ProjectileSimulation simulation;
    Body target({10, 0}, {}, 2, 1, 2);
    target.armor = 7;
    Projectile shot;
    shot.team = 1;
    shot.velocity = {100, 0};
    shot.timeRemaining = 1;
    shot.damage = 20;
    simulation.Spawn(shot);
    simulation.Advance(1, {&target});
    ASSERT_EQ(simulation.LastImpacts().size(), 1u);
    EXPECT_FLOAT_EQ(simulation.LastImpacts()[0].appliedDamage, 13);
    EXPECT_EQ(target.damageCalls, 1);
}
