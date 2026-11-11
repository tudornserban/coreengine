#include "Scenario.h"

#include "core/ai/BehaviorFactory.h"
#include "core/ai/Behaviors/FleeBehavior.h"
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/Behaviors/SeparationBehavior.h"
#include "core/interfaces/IBehavior.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IUnitKinematics.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/math/Vector2.h"

namespace
{

class VelocityCoastBehavior : public IBehavior
{
public:
    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& ctx) override
    {
        if (ctx.self == nullptr)
        {
            return {};
        }
        const Vector2 vel = ctx.self->GetVelocity();
        if (vel.LengthSquared() < 1e-6f)
        {
            return {};
        }
        const Vector2 ahead = ctx.self->GetPosition() + vel.Normalized() * 50.0f;
        return {InfluenceFieldItem::CreateAttractor(ahead, 8.0f)};
    }

    Vector2 CalculateAcceleration(const BehaviorContext& /*ctx*/) override { return Vector2::Zero(); }

    const char* GetName() const override { return "Coast"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    std::map<std::string, float> params_;
};

void EnsureBehaviorBuiltinsRegistered()
{
    static bool registered = false;
    if (!registered)
    {
        BehaviorFactory::RegisterBuiltins();
        registered = true;
    }
}

std::unique_ptr<RTSGameState> MakePerceptionScene(std::uint64_t seed)
{
    auto state = std::make_unique<RTSGameState>(Vector2(800.0f, 600.0f), seed);
    state->SetPerceptionRadius(80.0f);
    return state;
}

void BuildTwoUnits(RTSGameState& state)
{
    state.AddUnit(std::make_unique<RTSUnit>(Vector2(200.0f, 300.0f), 1, 12.0f, 100.0f));
    state.AddUnit(std::make_unique<RTSUnit>(Vector2(600.0f, 300.0f), 2, 12.0f, 100.0f));
}

void BuildCollision(RTSGameState& state)
{
    auto unitA = std::make_unique<RTSUnit>(Vector2(380.0f, 300.0f), 1, 16.0f, 100.0f);
    auto unitB = std::make_unique<RTSUnit>(Vector2(420.0f, 300.0f), 2, 16.0f, 100.0f);
    if (IUnitKinematics* kinA = dynamic_cast<IUnitKinematics*>(unitA.get()))
    {
        kinA->SetVelocity(Vector2(80.0f, 0.0f));
    }
    if (IUnitKinematics* kinB = dynamic_cast<IUnitKinematics*>(unitB.get()))
    {
        kinB->SetVelocity(Vector2(-80.0f, 0.0f));
    }
    unitA->GetStateMachine().AddState("Coast", std::make_unique<VelocityCoastBehavior>());
    unitA->GetStateMachine().SetState("Coast");
    unitB->GetStateMachine().AddState("Coast", std::make_unique<VelocityCoastBehavior>());
    unitB->GetStateMachine().SetState("Coast");
    state.AddUnit(std::move(unitA));
    state.AddUnit(std::move(unitB));
}

void BuildCombat(RTSGameState& state)
{
    auto addPair = [&](float centerX, float centerY, float velX)
    {
        auto u1 = std::make_unique<RTSUnit>(Vector2(centerX, centerY - 20.0f), 1, 12.0f, 100.0f);
        auto u2 = std::make_unique<RTSUnit>(Vector2(centerX, centerY + 20.0f), 2, 12.0f, 100.0f);
        if (IUnitKinematics* k1 = dynamic_cast<IUnitKinematics*>(u1.get()))
        {
            k1->SetVelocity(Vector2(velX, 0.0f));
        }
        if (IUnitKinematics* k2 = dynamic_cast<IUnitKinematics*>(u2.get()))
        {
            k2->SetVelocity(Vector2(velX, 0.0f));
        }
        state.AddUnit(std::move(u1));
        state.AddUnit(std::move(u2));
    };
    addPair(360.0f, 300.0f, 80.0f);
    addPair(440.0f, 300.0f, -80.0f);
}

void BuildAi(RTSGameState& state)
{
    EnsureBehaviorBuiltinsRegistered();
    state.AddStaticAttractor(InfluenceFieldItem::CreateAttractor(Vector2(700.0f, 300.0f), 5.0f));

    {
        auto unit = std::make_unique<RTSUnit>(Vector2(120.0f, 300.0f), 0, 12.0f, 100.0f);
        unit->GetStateMachine().AddState("Seek", std::make_unique<SeekBehavior>(Vector2(700.0f, 300.0f)));
        unit->GetStateMachine().SetState("Seek");
        state.AddUnit(std::move(unit));
    }
    {
        auto unit = std::make_unique<RTSUnit>(Vector2(700.0f, 450.0f), 1, 12.0f, 100.0f);
        unit->GetStateMachine().AddState("Flee", std::make_unique<FleeBehavior>());
        unit->GetStateMachine().SetState("Flee");
        state.AddUnit(std::move(unit));
    }
    {
        auto unit = std::make_unique<RTSUnit>(Vector2(140.0f, 260.0f), 0, 12.0f, 100.0f);
        unit->GetStateMachine().AddState("Separation", std::make_unique<SeparationBehavior>());
        unit->GetStateMachine().SetState("Separation");
        state.AddUnit(std::move(unit));
    }
    {
        auto unit = std::make_unique<RTSUnit>(Vector2(140.0f, 340.0f), 0, 12.0f, 100.0f);
        unit->GetStateMachine().AddState("Separation", std::make_unique<SeparationBehavior>());
        unit->GetStateMachine().SetState("Separation");
        state.AddUnit(std::move(unit));
    }
}

} // namespace

Scene BuildPreset(Preset preset, std::uint64_t seed)
{
    Scene scene;
    switch (preset)
    {
    case Preset::TwoUnits:
        scene.state = std::make_unique<RTSGameState>(Vector2(800.0f, 600.0f), seed);
        BuildTwoUnits(*scene.state);
        break;
    case Preset::Collision:
        scene.state = MakePerceptionScene(seed);
        BuildCollision(*scene.state);
        break;
    case Preset::Combat:
        scene.state = MakePerceptionScene(seed);
        BuildCombat(*scene.state);
        break;
    case Preset::Ai:
        scene.state = MakePerceptionScene(seed);
        BuildAi(*scene.state);
        break;
    }
    return scene;
}

std::vector<ScenarioEvent> PendingEvents(Preset preset, const RTSGameState& state)
{
    std::vector<ScenarioEvent> events;
    if (preset == Preset::Combat)
    {
        const Vector2 center = state.GetMapSize() * 0.5f;
        ScenarioEvent event;
        event.time = 0.5f;
        event.explosion = Explosion(center, 90.0f, 8000.0f);
        event.fired = false;
        events.push_back(event);
    }
    return events;
}

const char* PresetToCliName(Preset preset)
{
    switch (preset)
    {
    case Preset::TwoUnits:
        return "two-units";
    case Preset::Collision:
        return "collision";
    case Preset::Combat:
        return "combat";
    case Preset::Ai:
        return "ai";
    }
    return "two-units";
}

bool ParsePresetFromString(const std::string& name, Preset& out)
{
    if (name == "two-units")
    {
        out = Preset::TwoUnits;
        return true;
    }
    if (name == "collision")
    {
        out = Preset::Collision;
        return true;
    }
    if (name == "combat")
    {
        out = Preset::Combat;
        return true;
    }
    if (name == "ai")
    {
        out = Preset::Ai;
        return true;
    }
    return false;
}
