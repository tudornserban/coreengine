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
#include "../core/ai/BehaviorFactory.h"
#include "../core/influence/TerritoryCalculator.h"
#include "../core/serialization/GameStateSerializer.h"
#include <memory>
#include <cstring>
#include <sstream>

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
    });
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
    out->matchOver        = c->running ? 0 : 1;
    out->winner           = 0;
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
    return (teamId == 1) ? result.team1Pct : result.team2Pct;
}

// --- Behavior factory ---

void ce_behavior_register(const char* name)
{
    // Game-specific behaviors registered from game project code.
    // This function is a hook for the C API caller to trigger registration.
    // The actual IBehavior creator must be registered in C++ before this call.
    (void)name;
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
