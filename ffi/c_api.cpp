/**
 * CoreEngine C API — Implementation.
 *
 * This file bridges the flat C API to the C++ CoreEngine internals.
 * Each function validates its handle, dispatches to the engine, and
 * returns plain C types. No C++ exceptions cross the C boundary.
 */

#include "c_api.h"
#include "../core/gamestate/RTSGameState.h"
#include "../core/gamestate/RTSUnit.h"
#include "../core/gameloop/GameLogicEngine.h"
#include "../core/gameloop/DeploymentTimer.h"
#include "../core/gameloop/MatchState.h"
#include "../core/ai/BehaviorFactory.h"
#include "../core/ai/BehaviorContext.h"
#include "../core/interfaces/IBehavior.h"
#include "../core/influence/TerritoryCalculator.h"
#include "../core/serialization/GameStateSerializer.h"
#include "../core/math/Vector2.h"
#include <map>
#include <memory>
#include <cstring>
#include <sstream>
#include <string>
#include <exception>

static constexpr float kSpawnChargeSeconds = 10.0f;

// --- Internal engine context ---

struct EngineContext {
    std::unique_ptr<RTSGameState>    gameState;
    std::unique_ptr<GameLogicEngine> logicEngine;
    DeploymentTimer                  p1Timer{kSpawnChargeSeconds};
    DeploymentTimer                  p2Timer{kSpawnChargeSeconds};
    float                            mapWidth  = 1600.0f;
    float                            mapHeight = 900.0f;
    uint64_t                         rngSeed   = 0;
    bool                             running   = true;
    int                              lastResult = CE_OK;
    MatchState                       match;
};

static EngineContext* ctx(CE_Engine e)
{
    return reinterpret_cast<EngineContext*>(e);
}

static bool IsPlayableTeam(int teamId)
{
    return teamId == 1 || teamId == 2;
}

static DeploymentTimer* TimerForTeam(EngineContext* c, int teamId)
{
    return teamId == 1 ? &c->p1Timer : &c->p2Timer;
}

namespace {

class CAccelBehavior : public IBehavior {
public:
    CAccelBehavior(std::string name, CE_AccelFn fn, void* userdata)
        : name_(std::move(name)), fn_(fn), userdata_(userdata)
    {
    }

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& /*ctx*/) override
    {
        return {};
    }

    Vector2 CalculateAcceleration(const BehaviorContext& ctx) override
    {
        float ax = 0.0f;
        float ay = 0.0f;
        if (fn_ && ctx.self) {
            try {
                fn_(ctx.self->GetPosition().x,
                    ctx.self->GetPosition().y,
                    ctx.self->GetVelocity().x,
                    ctx.self->GetVelocity().y,
                    ctx.self->GetTeam(),
                    ctx.deltaTime,
                    &ax,
                    &ay,
                    userdata_);
            } catch (...) {
                return Vector2::Zero();
            }
        }
        return Vector2(ax, ay);
    }

    const char* GetName() const override { return name_.c_str(); }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    std::string name_;
    CE_AccelFn fn_ = nullptr;
    void* userdata_ = nullptr;
    std::map<std::string, float> params_;
};

}  // namespace

// --- Lifecycle ---

CE_Engine ce_engine_create(float mapWidth, float mapHeight, uint64_t rngSeed)
{
    BehaviorFactory::RegisterBuiltins();

    auto* c = new EngineContext();
    c->mapWidth    = mapWidth;
    c->mapHeight   = mapHeight;
    c->rngSeed     = rngSeed;
    c->lastResult  = CE_OK;
    c->gameState   = std::make_unique<RTSGameState>(Vector2(mapWidth, mapHeight), rngSeed);
    c->logicEngine = std::make_unique<GameLogicEngine>(1.0f / 30.0f);
    c->logicEngine->SetUpdateCallback([c](float dt) {
        c->p1Timer.Update(dt);
        c->p2Timer.Update(dt);
        c->gameState->Update(dt);

        TerritoryCalculator tc;
        TerritoryCalculator::Arena arena{c->mapWidth, c->mapHeight};
        auto territory = tc.Calculate(c->gameState->GetAllUnits(), arena);
        c->match.team1TerritoryPct = territory.team1Pct / 100.0f;
        c->match.team2TerritoryPct = territory.team2Pct / 100.0f;
        c->match.Advance(dt);
        if (c->match.IsOver()) {
            c->running = false;
        }
    });
    c->logicEngine->SetShouldEndCallback([c]() { return !c->running; });
    return reinterpret_cast<CE_Engine>(c);
}

void ce_engine_destroy(CE_Engine engine)
{
    delete ctx(engine);
}

int ce_engine_update(CE_Engine engine, float deltaTime)
{
    auto* c = ctx(engine);
    if (!c || !c->running) return 0;
    c->logicEngine->Update(deltaTime);
    return c->running ? 1 : 0;
}

void ce_engine_reset(CE_Engine engine)
{
    auto* c = ctx(engine);
    if (!c) return;
    c->gameState   = std::make_unique<RTSGameState>(
        Vector2(c->mapWidth, c->mapHeight), c->rngSeed);
    c->logicEngine->Reset();
    c->p1Timer     = DeploymentTimer(kSpawnChargeSeconds);
    c->p2Timer     = DeploymentTimer(kSpawnChargeSeconds);
    c->match       = MatchState{};
    c->running     = true;
    c->lastResult  = CE_OK;
}

uint64_t ce_engine_get_seed(CE_Engine engine)
{
    auto* c = ctx(engine);
    if (!c) return 0;
    return c->gameState->GetRNG().GetSeed();
}

// --- Unit management ---

int ce_unit_spawn(CE_Engine engine, int teamId, float x, float y)
{
    auto* c = ctx(engine);
    if (!c) return 0;

    if (!IsPlayableTeam(teamId)) {
        c->lastResult = CE_ERR_INVALID_TEAM;
        return 0;
    }

    DeploymentTimer& timer = *TimerForTeam(c, teamId);
    if (!timer.HasCharge()) {
        c->lastResult = CE_ERR_NO_CHARGE;
        return 0;
    }

    timer.SpendCharge();
    auto unit = std::make_unique<RTSUnit>(Vector2(x, y), teamId);
    c->gameState->AddUnit(std::move(unit));
    c->lastResult = CE_OK;
    return 1;
}

int ce_unit_count(CE_Engine engine)
{
    auto* c = ctx(engine);
    if (!c) return 0;
    return static_cast<int>(c->gameState->GetAllUnits().size());
}

int ce_unit_get_state(CE_Engine engine, int index, CE_UnitState* out)
{
    auto* c = ctx(engine);
    if (!c || !out) return 0;

    auto units = c->gameState->GetAllUnits();
    if (index < 0 || index >= static_cast<int>(units.size())) return 0;

    const IGameUnit* u = units[index];
    out->team   = u->GetTeam();
    out->x      = u->GetPosition().x;
    out->y      = u->GetPosition().y;
    out->vx     = u->GetVelocity().x;
    out->vy     = u->GetVelocity().y;
    out->hp     = u->GetCurrentHP();
    out->maxHp  = u->GetMaxHP();
    out->radius = u->GetRadius();
    return 1;
}

int ce_unit_set_behavior(CE_Engine engine, int index, const char* behaviorName)
{
    auto* c = ctx(engine);
    if (!c || !behaviorName) return 0;

    auto units = c->gameState->GetAllUnits();
    if (index < 0 || index >= static_cast<int>(units.size())) return 0;

    auto* rtsUnit = dynamic_cast<RTSUnit*>(units[index]);
    if (!rtsUnit) return 0;

    if (!BehaviorFactory::IsRegistered(behaviorName)) return 0;

    auto newBehavior = BehaviorFactory::Create(behaviorName);
    rtsUnit->GetStateMachine().AddState(behaviorName, std::move(newBehavior));
    rtsUnit->GetStateMachine().SetState(behaviorName);
    return 1;
}

// --- Field queries ---

float ce_field_query(CE_Engine engine, float x, float y)
{
    auto* c = ctx(engine);
    if (!c) return 0.0f;

    auto units = c->gameState->GetAllUnits();
    float team1 = 0.0f, team2 = 0.0f;
    for (const auto* u : units) {
        float dx = x - u->GetPosition().x;
        float dy = y - u->GetPosition().y;
        float distSq = dx * dx + dy * dy;
        float influence = 1.0f / (1.0f + distSq * 0.001f);
        if (u->GetTeam() == 1) team1 += influence;
        else                   team2 += influence;
    }
    return team1 - team2;
}

// --- Snapshot ---

int ce_snapshot_get(CE_Engine engine, CE_GameSnapshot* out)
{
    auto* c = ctx(engine);
    if (!c || !out) return 0;

    out->mapWidth         = c->mapWidth;
    out->mapHeight        = c->mapHeight;
    out->unitCount        = ce_unit_count(engine);
    out->matchTimeElapsed = c->logicEngine->GetGameTime();
    out->matchOver        = c->match.IsOver() ? 1 : 0;
    switch (c->match.result) {
    case MatchResult::Team1Wins:
        out->winner = 1;
        break;
    case MatchResult::Team2Wins:
        out->winner = 2;
        break;
    case MatchResult::Draw:
        out->winner = 3;
        break;
    default:
        out->winner = 0;
        break;
    }
    return 1;
}

int ce_snapshot_get_json(CE_Engine engine, char* buffer, int bufferSize)
{
    auto* c = ctx(engine);
    if (!c || !buffer || bufferSize <= 0) return -1;

    GameStateSerializer ser(c->gameState.get());
    std::string json = ser.GetStateJSON();

    int len = static_cast<int>(json.size());
    if (len >= bufferSize) return -1;

    std::memcpy(buffer, json.c_str(), len + 1);
    return len;
}

// --- Deployment timer ---

int ce_timer_has_charge(CE_Engine engine, int teamId)
{
    auto* c = ctx(engine);
    if (!c) return 0;
    if (!IsPlayableTeam(teamId)) {
        c->lastResult = CE_ERR_INVALID_TEAM;
        return 0;
    }
    c->lastResult = CE_OK;
    return TimerForTeam(c, teamId)->HasCharge() ? 1 : 0;
}

float ce_timer_get_progress(CE_Engine engine, int teamId)
{
    auto* c = ctx(engine);
    if (!c) return 0.0f;
    if (!IsPlayableTeam(teamId)) {
        c->lastResult = CE_ERR_INVALID_TEAM;
        return 0.0f;
    }
    c->lastResult = CE_OK;
    return TimerForTeam(c, teamId)->GetChargeProgress();
}

int ce_timer_grant_charge(CE_Engine engine, int teamId)
{
    auto* c = ctx(engine);
    if (!c) return 0;
    if (!IsPlayableTeam(teamId)) {
        c->lastResult = CE_ERR_INVALID_TEAM;
        return 0;
    }
    TimerForTeam(c, teamId)->Update(kSpawnChargeSeconds);
    c->lastResult = CE_OK;
    return 1;
}

int ce_last_result(CE_Engine engine)
{
    auto* c = ctx(engine);
    if (!c) return CE_ERR_NULL;
    return c->lastResult;
}

// --- Territory ---

float ce_territory_get_pct(CE_Engine engine, int teamId)
{
    auto* c = ctx(engine);
    if (!c) return 0.0f;
    if (!IsPlayableTeam(teamId)) {
        c->lastResult = CE_ERR_INVALID_TEAM;
        return 0.0f;
    }

    TerritoryCalculator tc;
    TerritoryCalculator::Arena arena{c->mapWidth, c->mapHeight};
    auto result = tc.Calculate(c->gameState->GetAllUnits(), arena);

    c->lastResult = CE_OK;
    const float pct = (teamId == 1) ? result.team1Pct : result.team2Pct;
    return pct / 100.0f;
}

// --- Behavior factory ---

void ce_behavior_register(const char* name)
{
    (void)name;
}

int ce_behavior_register_accel(const char* name, CE_AccelFn fn, void* userdata)
{
    if (!name || name[0] == '\0' || !fn) {
        return 0;
    }
    try {
        const std::string key(name);
        BehaviorFactory::Register(key, [key, fn, userdata]() {
            return std::make_unique<CAccelBehavior>(key, fn, userdata);
        });
        return 1;
    } catch (...) {
        return 0;
    }
}

int ce_behavior_list(char* buffer, int bufferSize)
{
    auto keys = BehaviorFactory::GetRegisteredKeys();
    std::string result;
    for (size_t i = 0; i < keys.size(); ++i) {
        result += keys[i];
        if (i + 1 < keys.size()) result += ",";
    }

    int len = static_cast<int>(result.size());
    if (len >= bufferSize) return -1;
    std::memcpy(buffer, result.c_str(), len + 1);
    return len;
}

// --- Version ---

const char* ce_version()
{
    return "CoreEngine 1.0.0";
}

const char* ce_ffi_version(void)
{
    return CE_FFI_VERSION;
}
