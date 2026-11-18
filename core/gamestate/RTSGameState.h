#pragma once

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../interfaces/IGameUnit.h"
#include "../interfaces/ISimulationSnapshot.h"
#include "../ai/BehaviorContext.h"
#include "../math/Vector2.h"
#include "../math/DeterministicRNG.h"
#include "../combat/SelectionSystem.h"
#include "../combat/CommandSystem.h"
#include "../combat/CombatManager.h"
#include "../combat/ProjectileSimulation.h"
#include "../influence/InfluenceFieldItem.h"
#include "../map/Map.h"
#include "../collision/CollisionSystem.h"
#include "../combat/Explosion.h"
#include "SimulationDiagnostics.h"
#include "TimedActionClock.h"

/// Simulation hub.
///
/// `Update` tick, skipped entirely while `IsSimulationPaused()` (rendering may
/// keep reading the last diagnostics and must not call `Update`):
/// 1. Increment the simulation tick (the first update is tick 1).
/// 2. Observe deaths. The first observation emits one `LifecycleEvent` and
///    disables action participation. Movement and collision flags stay as set,
///    so coasting, immobilization, and contact policy remain game choices.
/// 3. For each behavior host that participates in actions and is not in debug
///    mode: evaluate automatic FSM transitions once (registration order, first
///    match, enter/exit callbacks), then evaluate the current behavior once.
///    Behavior forces and static-field forces are summed and clamped to the
///    unit's maximum acceleration.
/// 4. Split-motion units with movement participation apply that propulsion to
///    velocity. The speed limit applies here. Legacy combined-motion hosts
///    still use `ApplyAcceleration`.
/// 5. Explosions become velocity changes. They do not integrate position and
///    do not apply the propulsion speed limit. Multiple impulses sum in spawn order.
/// 6. Split-motion units with movement participation integrate position once.
///    Debug movement has already integrated inside `Update` and is not integrated again.
/// 7. Collisions for units that participate in collision.
/// 8. Projectile sweep against this tick's start-to-post-contact target chords, impacts, and expiry.
/// 9. Timed actions advance one tick.
/// 10. Collision immunity countdown and the RTS-unit bounds fallback.
/// 11. Observe deaths again so damage from this tick latches before commit.
/// 12. Commit spawn and despawn requests made during the tick. A unit spawned
///    during tick N is visible after `Update` returns and first acts on tick N+1.
///    Ids start at 1 and are not reused. Selection and collision immunity drop
///    removed units.
///
/// Diagnostics copy the values from steps 3 and 5. Enabling them does not
/// evaluate behavior or draw RNG a second time.
class RTSGameState : public ISimulationSnapshot
{
private:
    std::vector<std::unique_ptr<IGameUnit>> units_;
    SelectionSystem selectionSystem_;
    CommandSystem commandSystem_;

    std::unique_ptr<Map> map_;
    float perceptionRadius_;  // Default radius for nearby unit queries
    std::vector<InfluenceFieldItem> mapEdgeItems_;       // Map boundary field items (derived from Map)
    std::vector<InfluenceFieldItem> staticAttractors_;  // Static attractors (goals, resources, etc.)
    
    // Deterministic RNG for reproducible randomness
    DeterministicRNG rng_;

    // Collision system
    CollisionSystem collisionSystem_;

    // Explosion system
    std::vector<Explosion> explosions_;

    ProjectileSimulation projectiles_;
    TimedActionClock actions_;
    CombatManager* combat_ = nullptr;
    std::string rulesIdentity_ = "unspecified";
    std::vector<LifecycleEvent> lifecycleEvents_;
    uint32_t simulationTick_ = 0;
    uint64_t nextEntityId_ = 1;
    std::unordered_map<const IGameUnit*, uint64_t> entityIds_;
    bool paused_ = false;
    bool updating_ = false;
    bool diagnosticsEnabled_ = false;
    std::vector<UnitDiagnosticRecord> diagnostics_;
    std::function<void(const IGameUnit&, std::map<std::string, float>&)> diagnosticExtension_;
    std::function<std::vector<uint8_t>()> extensionCapture_;
    std::function<bool(const std::vector<uint8_t>&)> extensionValidate_;
    std::function<void(const std::vector<uint8_t>&)> extensionApply_;
    std::vector<std::unique_ptr<IGameUnit>> pendingSpawns_;
    std::vector<IGameUnit*> pendingDespawns_;
    std::vector<TimedActionEvent> actionEvents_;

public:
    RTSGameState(Vector2 mapBounds, uint64_t rngSeed = 0);
    RTSGameState(std::unique_ptr<Map> mapInstance, uint64_t rngSeed = 0);

    // Unit management. Calls during `Update` are deferred to the commit phase.
    void AddUnit(std::unique_ptr<IGameUnit> unit);
    void RemoveUnit(IGameUnit* unit);
    uint64_t GetEntityId(const IGameUnit* unit) const;

    // Main update
    void Update(float deltaTime);
    void SetSimulationPaused(bool paused) { paused_ = paused; }
    bool IsSimulationPaused() const { return paused_; }
    uint32_t GetSimulationTick() const { return simulationTick_; }
    const std::vector<LifecycleEvent>& GetLifecycleEvents() const { return lifecycleEvents_; }

    void SetDiagnosticsEnabled(bool enabled) { diagnosticsEnabled_ = enabled; }
    bool DiagnosticsEnabled() const { return diagnosticsEnabled_; }
    const std::vector<UnitDiagnosticRecord>& GetDiagnostics() const { return diagnostics_; }
    void SetDiagnosticExtension(std::function<void(const IGameUnit&, std::map<std::string, float>&)> extension) {
        diagnosticExtension_ = std::move(extension);
    }

    ProjectileSimulation& GetProjectiles() { return projectiles_; }
    const ProjectileSimulation& GetProjectiles() const { return projectiles_; }
    // Value events from the last completed tick; unchanged by paused updates.
    const std::vector<ProjectileImpactRecord>& GetProjectileImpacts() const { return projectiles_.LastImpacts(); }
    TimedActionClock& GetTimedActions() { return actions_; }
    const TimedActionClock& GetTimedActions() const { return actions_; }
    const std::vector<TimedActionEvent>& GetActionEvents() const { return actionEvents_; }

    /// Non-owning. Canonical snapshots include this manager's id-keyed cooldowns.
    void AttachCombatManager(CombatManager* combat) { combat_ = combat; }

    void SetRulesIdentity(std::string rulesIdentity) { rulesIdentity_ = std::move(rulesIdentity); }
    const std::string& GetRulesIdentity() const { return rulesIdentity_; }

    /// Game-owned bytes inside the canonical snapshot. `validate` must not mutate.
    /// `apply` runs only after the snapshot has otherwise been accepted.
    void SetGameExtension(
        std::function<std::vector<uint8_t>()> capture,
        std::function<bool(const std::vector<uint8_t>&)> validate,
        std::function<void(const std::vector<uint8_t>&)> apply);

    /// Versioned restorable state. Distinct from diagnostic JSON.
    /// Topology (which behaviors exist) must already match; unknown states fail.
    std::string CaptureCanonicalState() const;
    uint64_t CanonicalStateHash() const;
    bool RestoreCanonicalState(const std::string& blob);

    // Queries
    std::vector<IGameUnit*> GetUnitsInRadius(Vector2 position, float radius) const;
    std::vector<IGameUnit*> GetTeamUnitsInRadius(Vector2 position, float radius, int team) const;
    std::vector<IGameUnit*> GetAllUnits() const override;
    IGameUnit* GetUnitAtPosition(Vector2 position, float tolerance = 15.0f) const;

    // Systems
    SelectionSystem& GetSelectionSystem() { return selectionSystem_; }
    CommandSystem& GetCommandSystem() { return commandSystem_; }
    CollisionSystem& GetCollisionSystem() { return collisionSystem_; }
    DeterministicRNG& GetRNG() { return rng_; }
    const DeterministicRNG& GetRNG() const { return rng_; }

    // Perception radius
    void SetPerceptionRadius(float radius) { perceptionRadius_ = radius; }
    float GetPerceptionRadius() const { return perceptionRadius_; }

    // Getters
    Vector2 GetMapSize() const override;
    Vector2 GetMapCenter() const;
    Map* GetMap() { return map_.get(); }
    const Map* GetMap() const { return map_.get(); }
    const std::vector<InfluenceFieldItem>& GetStaticItems() const;
    const std::vector<InfluenceFieldItem>& GetStaticAttractors() const { return staticAttractors_; }

    // Obstacle management (for backward compatibility - now uses Map entities)
    void AddObstacle(const InfluenceFieldItem& obstacle);
    void ClearObstacles();
    
    // Attractor management
    void AddStaticAttractor(const InfluenceFieldItem& attractor);
    void ClearStaticAttractors();

    // Explosion management
    void AddExplosion(const Explosion& explosion);
    const std::vector<Explosion>& GetExplosions() const { return explosions_; }

    // Debug access
    BehaviorContext BuildContextForUnit(IGameUnit* unit, float deltaTime);

private:
    void InitializeStaticFieldItems();
    void AdoptUnit(std::unique_ptr<IGameUnit> unit);
    void DestroyUnit(IGameUnit* unit);
    void CommitStructure();
    void ObserveDeaths();
};

