#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/ai/BehaviorContext.h"
#include "core/ai/Conditions/TimeElapsedCondition.h"
#include "core/ai/StateMachine.h"
#include "core/combat/CombatManager.h"
#include "core/combat/Explosion.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IBehavior.h"
#include "core/interfaces/ITransitionCondition.h"
#include "core/math/Vector2.h"

namespace {

class AccelBehavior : public IBehavior {
public:
    explicit AccelBehavior(Vector2 acceleration) : acceleration_(acceleration) {
        parameters_["targetX"] = 12.0f;
        parameters_["targetY"] = 8.0f;
        parameters_["strength"] = 1.0f;
    }

    Vector2 CalculateAcceleration(const BehaviorContext&) override { return acceleration_; }
    const char* GetName() const override { return "Accel"; }
    std::map<std::string, float>& GetParameters() override { return parameters_; }
    const std::map<std::string, float>& GetParameters() const override { return parameters_; }

private:
    Vector2 acceleration_;
    std::map<std::string, float> parameters_;
};

class RngCondition : public ITransitionCondition {
public:
    bool Evaluate(const BehaviorContext& ctx) const override {
        if (ctx.rng) {
            (void)ctx.rng->NextUInt32();
        }
        return false;
    }
    const char* GetDescription() const override { return "rng"; }
};

class SpawnOnEval : public IBehavior {
public:
    explicit SpawnOnEval(RTSGameState* state) : state_(state) {}
    Vector2 CalculateAcceleration(const BehaviorContext&) override {
        if (!spawned_) {
            state_->AddUnit(std::make_unique<RTSUnit>(Vector2(80.0f, 80.0f), 1, 4.0f, 10.0f));
            spawned_ = true;
        }
        return Vector2::Zero();
    }
    const char* GetName() const override { return "Spawn"; }
    std::map<std::string, float>& GetParameters() override { return parameters_; }
    const std::map<std::string, float>& GetParameters() const override { return parameters_; }
    int Calls() const { return spawned_ ? 1 : 0; }

private:
    RTSGameState* state_;
    bool spawned_ = false;
    std::map<std::string, float> parameters_;
};

RTSGameState MakeWorld() {
    RTSGameState state(Vector2(500.0f, 500.0f), 99);
    state.SetRulesIdentity("venus-fixture");
    auto unit = std::make_unique<RTSUnit>(Vector2(40.0f, 40.0f), 0, 8.0f, 1000.0f);
    unit->SetMaxAcceleration(100.0f);
    unit->GetStateMachine().AddState("Accel", std::make_unique<AccelBehavior>(Vector2(300.0f, 0.0f)));
    unit->GetStateMachine().AddState("Later", std::make_unique<AccelBehavior>(Vector2(0.0f, 10.0f)));
    unit->GetStateMachine().AddTransition(StateTransition(
        "Accel", "Later", std::make_unique<TimeElapsedCondition>(10.0f), true));
    unit->GetStateMachine().AddTransition(StateTransition(
        "Accel", "Later", std::make_unique<RngCondition>(), true));
    state.AddUnit(std::move(unit));
    return state;
}

void Step(RTSGameState& state) {
    if (state.GetSimulationTick() == 1) {
        state.AddExplosion(Explosion(Vector2(20.0f, 40.0f), 80.0f, 50.0f));
    }
    state.Update(0.5f);
}

}  // namespace

TEST(VenusWarsCapabilities, AutomaticTransitionPriorityAndSingleBehaviorEval) {
    RTSGameState state(Vector2(400.0f, 400.0f), 1);
    auto unit = std::make_unique<RTSUnit>(Vector2(50.0f, 50.0f), 0, 6.0f, 1000.0f);
    RTSUnit* raw = unit.get();
    raw->GetStateMachine().AddState("A", std::make_unique<AccelBehavior>(Vector2(0.0f, 0.0f)));
    raw->GetStateMachine().AddState("B", std::make_unique<AccelBehavior>(Vector2(0.0f, 0.0f)));
    raw->GetStateMachine().AddState("C", std::make_unique<AccelBehavior>(Vector2(0.0f, 0.0f)));
    raw->GetStateMachine().AddTransition(StateTransition(
        "A", "B", std::make_unique<TimeElapsedCondition>(0.0f), true));
    raw->GetStateMachine().AddTransition(StateTransition(
        "A", "C", std::make_unique<TimeElapsedCondition>(0.0f), true));
    raw->GetStateMachine().AddTransition(StateTransition(
        "B", "C", std::make_unique<TimeElapsedCondition>(0.0f), false));
    state.AddUnit(std::move(unit));

    state.Update(0.1f);
    EXPECT_EQ(raw->GetStateMachine().GetCurrentState(), "B");
    raw->GetStateMachine().SetState("C");
    EXPECT_EQ(raw->GetStateMachine().GetCurrentState(), "C");
}

TEST(VenusWarsCapabilities, AccelerationLimitsAndCombinedClamp) {
    RTSGameState state(Vector2(1000.0f, 1000.0f), 2);
    auto slow = std::make_unique<RTSUnit>(Vector2(100.0f, 100.0f), 0, 5.0f, 10000.0f);
    auto fast = std::make_unique<RTSUnit>(Vector2(100.0f, 300.0f), 0, 5.0f, 10000.0f);
    slow->SetMaxAcceleration(50.0f);
    fast->SetMaxAcceleration(100.0f);
    slow->GetStateMachine().AddState("Accel", std::make_unique<AccelBehavior>(Vector2(300.0f, 0.0f)));
    fast->GetStateMachine().AddState("Accel", std::make_unique<AccelBehavior>(Vector2(300.0f, 0.0f)));
    RTSUnit* slowPtr = slow.get();
    RTSUnit* fastPtr = fast.get();
    state.AddUnit(std::move(slow));
    state.AddUnit(std::move(fast));
    state.Update(1.0f);
    EXPECT_NEAR(slowPtr->GetVelocity().x, 50.0f, 1.0e-3f);
    EXPECT_NEAR(fastPtr->GetVelocity().x, 100.0f, 1.0e-3f);
    EXPECT_GT(fastPtr->GetVelocity().x, slowPtr->GetVelocity().x);
}

TEST(VenusWarsCapabilities, ImpulseDoesNotIntegrateAndMassScales) {
    RTSUnit heavy(Vector2(10.0f, 10.0f), 0);
    heavy.SetBaseMass(200.0f);
    heavy.SetHP(100.0f);
    const Vector2 start = heavy.GetPosition();
    heavy.ApplyImpulse(Vector2(200.0f, 0.0f));
    heavy.ApplyImpulse(Vector2(200.0f, 0.0f));
    EXPECT_FLOAT_EQ(heavy.GetPosition().x, start.x);
    EXPECT_NEAR(heavy.GetVelocity().x, 2.0f, 1.0e-4f);

    RTSUnit light(Vector2(10.0f, 10.0f), 0);
    light.SetBaseMass(50.0f);
    light.ApplyImpulse(Vector2(200.0f, 0.0f));
    EXPECT_GT(light.GetVelocity().x, heavy.GetVelocity().x);

    heavy.SetHP(0.0f);
    heavy.SetRadius(0.0f);
    heavy.ApplyImpulse(Vector2(10.0f, 0.0f));
    EXPECT_TRUE(std::isfinite(heavy.GetVelocity().x));
    EXPECT_TRUE(std::isfinite(heavy.GetVisualRadius()));
    EXPECT_FLOAT_EQ(heavy.GetRadius(), 0.0f);
}

TEST(VenusWarsCapabilities, MaxHpPoliciesAndVisualCurve) {
    RTSUnit unit(Vector2::Zero(), 0);
    unit.SetHP(40.0f);
    unit.SetMaxHP(50.0f, MaxHpPolicy::PreserveRatio);
    EXPECT_FLOAT_EQ(unit.GetMaxHP(), 50.0f);
    EXPECT_FLOAT_EQ(unit.GetCurrentHP(), 20.0f);
    unit.SetHP(80.0f);
    unit.SetMaxHP(200.0f, MaxHpPolicy::ClampCurrent);
    unit.SetMaxHP(50.0f, MaxHpPolicy::ClampCurrent);
    EXPECT_FLOAT_EQ(unit.GetCurrentHP(), 50.0f);
    unit.SetMaxHP(-5.0f, MaxHpPolicy::ClampCurrent);
    EXPECT_FLOAT_EQ(unit.GetMaxHP(), 50.0f);

    unit.SetVisualRadiusCurve([](float, float, float) { return 7.0f; });
    EXPECT_FLOAT_EQ(unit.GetVisualRadius(), 7.0f);
    EXPECT_FLOAT_EQ(unit.GetRadius(), 10.0f);
}

TEST(VenusWarsCapabilities, DeathLatchParticipationAndDeferredSpawn) {
    RTSGameState state(Vector2(200.0f, 200.0f), 3);
    auto mover = std::make_unique<RTSUnit>(Vector2(20.0f, 20.0f), 0, 5.0f, 100.0f);
    mover->SetVelocity(Vector2(10.0f, 0.0f));
    mover->TakeDamage(1000.0f);
    RTSUnit* dead = mover.get();
    auto spawner = std::make_unique<RTSUnit>(Vector2(30.0f, 30.0f), 0, 5.0f, 10.0f);
    auto behavior = std::make_unique<SpawnOnEval>(&state);
    SpawnOnEval* behaviorPtr = behavior.get();
    spawner->GetStateMachine().AddState("Spawn", std::move(behavior));
    state.AddUnit(std::move(mover));
    state.AddUnit(std::move(spawner));
    state.GetSelectionSystem().SelectUnit(dead);

    state.Update(1.0f);
    EXPECT_EQ(state.GetLifecycleEvents().size(), 1u);
    EXPECT_EQ(state.GetLifecycleEvents()[0].entityId, dead->GetEntityId());
    EXPECT_FALSE(dead->ParticipatesInActions());
    EXPECT_TRUE(dead->ParticipatesInMovement());
    EXPECT_FLOAT_EQ(dead->GetPosition().x, 30.0f);
    ASSERT_EQ(state.GetAllUnits().size(), 3u);
    IGameUnit* spawned = state.GetAllUnits()[2];
    EXPECT_EQ(spawned->GetEntityId(), 3u);
    EXPECT_FLOAT_EQ(spawned->GetPosition().x, 80.0f);
    EXPECT_EQ(behaviorPtr->Calls(), 1);

    state.Update(1.0f);
    EXPECT_EQ(state.GetLifecycleEvents().size(), 1u);
    EXPECT_FLOAT_EQ(spawned->GetPosition().x, 80.0f);

    dead->SetMovementParticipation(false);
    const float held = dead->GetPosition().x;
    state.Update(1.0f);
    EXPECT_FLOAT_EQ(dead->GetPosition().x, held);

    const uint64_t removedId = spawned->GetEntityId();
    state.GetSelectionSystem().SelectUnit(spawned);
    state.RemoveUnit(spawned);
    EXPECT_TRUE(state.GetSelectionSystem().GetSelectedUnits().empty());
    auto replacement = std::make_unique<RTSUnit>(Vector2(1.0f, 1.0f), 0);
    state.AddUnit(std::move(replacement));
    EXPECT_NE(state.GetAllUnits().back()->GetEntityId(), removedId);
}

TEST(VenusWarsCapabilities, DiagnosticsDoNotChangeStateAndPauseDoesNotAdvance) {
    RTSGameState quiet = MakeWorld();
    RTSGameState watched = MakeWorld();
    watched.SetDiagnosticsEnabled(true);
    watched.SetDiagnosticExtension([](const IGameUnit& unit, std::map<std::string, float>& fields) {
        fields["hp"] = unit.GetCurrentHP();
    });
    Step(quiet);
    Step(watched);
    Step(quiet);
    Step(watched);
    EXPECT_EQ(quiet.CanonicalStateHash(), watched.CanonicalStateHash());
    ASSERT_FALSE(watched.GetDiagnostics().empty());
    EXPECT_EQ(watched.GetDiagnostics()[0].behavior, "Accel");
    EXPECT_TRUE(watched.GetDiagnostics()[0].hasTarget);
    EXPECT_FLOAT_EQ(watched.GetDiagnostics()[0].target.x, 12.0f);
    EXPECT_EQ(watched.GetDiagnostics()[0].extensions.at("hp"), watched.GetAllUnits()[0]->GetCurrentHP());

    const uint64_t id = watched.GetDiagnostics()[0].entityId;
    const Vector2 position = watched.GetAllUnits()[0]->GetPosition();
    watched.RemoveUnit(watched.GetAllUnits()[0]);
    EXPECT_EQ(watched.GetDiagnostics()[0].entityId, id);
    EXPECT_TRUE(std::isfinite(watched.GetDiagnostics()[0].propulsion.x));

    watched.SetSimulationPaused(true);
    const uint32_t tick = watched.GetSimulationTick();
    const uint64_t rng = watched.GetRNG().GetState();
    watched.Update(1.0f);
    EXPECT_EQ(watched.GetSimulationTick(), tick);
    EXPECT_EQ(watched.GetRNG().GetState(), rng);
    (void)position;
}

TEST(VenusWarsCapabilities, TimedActionsCompleteOnceAndRestore) {
    TimedActionClock clock;
    const uint64_t id = clock.Start(4, 2);
    EXPECT_TRUE(clock.Advance().empty());
    const auto done = clock.Advance();
    ASSERT_EQ(done.size(), 1u);
    EXPECT_EQ(done[0].status, TimedActionStatus::Completed);
    EXPECT_TRUE(clock.Advance().empty());

    const uint64_t other = clock.Start(4, 5);
    clock.Interrupt(other);
    const TimedAction* interrupted = clock.Find(other);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(interrupted->status, TimedActionStatus::Interrupted);
    EXPECT_EQ(interrupted->elapsedTicks, 0u);
    const auto events = clock.TakeEvents();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_TRUE(clock.TakeEvents().empty());
    EXPECT_FALSE(clock.Cancel(other));

    TimedActionClock saved;
    const uint64_t running = saved.Start(1, 4);
    saved.Advance();
    const auto captured = saved.Capture();
    saved.Advance();
    saved.Advance();
    saved.Advance();
    EXPECT_EQ(saved.Find(running)->status, TimedActionStatus::Completed);
    saved.Restore(captured, saved.NextId());
    EXPECT_EQ(saved.Find(running)->status, TimedActionStatus::Running);
    EXPECT_EQ(saved.Find(running)->elapsedTicks, 1u);
    EXPECT_FLOAT_EQ(TimedActionClock::Lerp(0.0f, 10.0f, TimedActionClock::Progress(*saved.Find(running))), 2.5f);
}

TEST(VenusWarsCapabilities, CanonicalRestoreMatchesUninterruptedRun) {
    RTSGameState uninterrupted = MakeWorld();
    for (int i = 0; i < 4; ++i) {
        Step(uninterrupted);
    }
    const uint64_t expected = uninterrupted.CanonicalStateHash();

    RTSGameState partial = MakeWorld();
    Step(partial);
    Step(partial);
    const std::string blob = partial.CaptureCanonicalState();
    const Vector2 before = partial.GetAllUnits()[0]->GetPosition();
    EXPECT_FALSE(partial.RestoreCanonicalState("not-a-snapshot"));
    EXPECT_FLOAT_EQ(partial.GetAllUnits()[0]->GetPosition().x, before.x);
    std::string unknown = blob;
    const auto behavior = unknown.find("behavior Accel");
    ASSERT_NE(behavior, std::string::npos);
    unknown.replace(behavior, std::string("behavior Accel").size(), "behavior Missing");
    EXPECT_FALSE(partial.RestoreCanonicalState(unknown));
    EXPECT_FLOAT_EQ(partial.GetAllUnits()[0]->GetPosition().x, before.x);

    RTSGameState restored = MakeWorld();
    ASSERT_TRUE(restored.RestoreCanonicalState(blob));
    Step(restored);
    Step(restored);
    EXPECT_EQ(restored.CanonicalStateHash(), expected);
}

TEST(VenusWarsCapabilities, FixedStepPartitionsAndDiagnosticsStayIdentical) {
    auto runSteps = [](int steps) {
        RTSGameState state = MakeWorld();
        for (int i = 0; i < steps; ++i) {
            Step(state);
        }
        return state.CanonicalStateHash();
    };
    const uint64_t direct = runSteps(3);

    RTSGameState partitioned = MakeWorld();
    GameLogicEngine slow(0.5f);
    slow.SetUpdateCallback([&](float dt) {
        if (partitioned.GetSimulationTick() == 1) {
            partitioned.AddExplosion(Explosion(Vector2(20.0f, 40.0f), 80.0f, 50.0f));
        }
        partitioned.Update(dt);
    });
    slow.Update(0.6f);
    slow.Update(0.9f);
    EXPECT_EQ(partitioned.GetSimulationTick(), 3u);
    EXPECT_EQ(partitioned.CanonicalStateHash(), direct);

    RTSGameState fastState = MakeWorld();
    GameLogicEngine fast(0.5f);
    fast.SetSpeedMultiplier(2.0f);
    fast.SetUpdateCallback([&](float dt) {
        if (fastState.GetSimulationTick() == 1) {
            fastState.AddExplosion(Explosion(Vector2(20.0f, 40.0f), 80.0f, 50.0f));
        }
        fastState.Update(dt);
    });
    fast.Update(0.75f);
    EXPECT_EQ(fastState.GetSimulationTick(), 3u);
    EXPECT_EQ(fastState.CanonicalStateHash(), direct);
}

TEST(VenusWarsCapabilities, CombatCooldownRestoresByEntityId) {
    CombatManager manager;
    RTSUnit attacker(Vector2(0.0f, 0.0f), 0, 5.0f, 10.0f);
    RTSUnit defender(Vector2(10.0f, 0.0f), 1, 5.0f, 10.0f);
    attacker.SetEntityId(5);
    defender.SetEntityId(8);
    attacker.SetBaseDPS(10.0f);
    attacker.SetAttackCooldown(1.0f);
    attacker.SetAttackRange(100.0f);
    CombatManager::Params params;
    params.mode = CombatMode::DirectAttack;
    manager.Resolve({&attacker, &defender}, 0.4f, params);
    const auto exported = manager.ExportCooldowns();
    ASSERT_EQ(exported.size(), 1u);
    EXPECT_EQ(exported[0].entityId, 5u);
    EXPECT_NEAR(exported[0].remaining, 0.4f, 1.0e-4f);

    CombatManager restored;
    restored.ImportCooldowns(exported);
    EXPECT_EQ(restored.ExportCooldowns()[0].entityId, 5u);
    EXPECT_NEAR(restored.ExportCooldowns()[0].remaining, 0.4f, 1.0e-4f);
}
