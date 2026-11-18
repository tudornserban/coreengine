#include "RTSGameState.h"
#include "BehaviorContextAssembler.h"
#include "../combat/Explosion.h"
#include "RTSUnit.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include "../map/MapComponents/MapObstacleComponent.h"
#include "../influence/InfluenceFieldItem.h"
#include "../influence/InfluenceFieldCalculator.h"
#include "../collision/ICollisionDetector.h"
#include "../math/DeterministicRNG.h"
#include "../logging/Logger.h"
#include "../interfaces/IUnitBehaviorHost.h"
#include "../ai/StateMachine.h"

namespace {

Vector2 ClampMagnitude(Vector2 value, float maxMagnitude) {
    if (!(maxMagnitude > 0.0f) || !std::isfinite(value.x) || !std::isfinite(value.y)) {
        return Vector2::Zero();
    }
    const float limitSq = maxMagnitude * maxMagnitude;
    if (!(value.LengthSquared() > limitSq)) {
        return value;
    }
    const float length = value.Length();
    if (!(length > 0.0f)) {
        return Vector2::Zero();
    }
    return value * (maxMagnitude / length);
}

}  // namespace

RTSGameState::RTSGameState(Vector2 mapBounds, uint64_t rngSeed)
    : map_(std::make_unique<Map>(mapBounds))
    , perceptionRadius_(300.0f)
    , rng_(rngSeed)
{
    LOG_DEBUG("RTSGameState: Constructor (Vector2) called, mapBounds=(%.2f, %.2f), rngSeed=%llu", 
              mapBounds.x, mapBounds.y, rngSeed);
    LOG_DEBUG("RTSGameState: Map created: %p", map_.get());
    LOG_DEBUG("RTSGameState: Initializing static field items...");
    InitializeStaticFieldItems();
    LOG_DEBUG("RTSGameState: Constructor (Vector2) completed");
}

RTSGameState::RTSGameState(std::unique_ptr<Map> mapInstance, uint64_t rngSeed)
    : map_(std::move(mapInstance))
    , perceptionRadius_(300.0f)
    , rng_(rngSeed)
{
    LOG_DEBUG("RTSGameState: Constructor (Map*) called, mapInstance=%p, rngSeed=%llu", 
              map_.get(), rngSeed);
    LOG_DEBUG("RTSGameState: Initializing static field items...");
    InitializeStaticFieldItems();
    LOG_DEBUG("RTSGameState: Constructor (Map*) completed");
}

void RTSGameState::AdoptUnit(std::unique_ptr<IGameUnit> unit)
{
    IGameUnit* unitPtr = unit.get();
    const uint64_t id = nextEntityId_++;
    entityIds_[unitPtr] = id;
    unitPtr->SetEntityId(id);
    LOG_DEBUG("RTSGameState::AddUnit: Adding unit at %p, id=%llu, position=(%.2f, %.2f), team=%d",
              unitPtr, static_cast<unsigned long long>(id), unitPtr->GetPosition().x,
              unitPtr->GetPosition().y, unitPtr->GetTeam());
    units_.push_back(std::move(unit));
}

void RTSGameState::DestroyUnit(IGameUnit* unit)
{
    if (!unit) {
        return;
    }
    const uint64_t id = GetEntityId(unit);
    selectionSystem_.ForgetUnit(unit);
    collisionSystem_.ForgetUnit(unit);
    if (combat_) {
        combat_->ForgetEntity(id);
    }
    entityIds_.erase(unit);
    auto it = std::find_if(units_.begin(), units_.end(),
        [unit](const std::unique_ptr<IGameUnit>& candidate) { return candidate.get() == unit; });
    if (it != units_.end()) {
        units_.erase(it);
    }
}

void RTSGameState::CommitStructure()
{
    std::vector<IGameUnit*> removals;
    removals.swap(pendingDespawns_);
    for (IGameUnit* unit : removals) {
        DestroyUnit(unit);
    }
    std::vector<std::unique_ptr<IGameUnit>> spawns;
    spawns.swap(pendingSpawns_);
    for (auto& unit : spawns) {
        if (unit) {
            AdoptUnit(std::move(unit));
        }
    }
}

void RTSGameState::ObserveDeaths()
{
    for (const auto& unit : units_) {
        if (!unit->ConsumeDeathEvent()) {
            continue;
        }
        lifecycleEvents_.push_back(LifecycleEvent{
            LifecycleEventKind::Died, GetEntityId(unit.get()), simulationTick_});
    }
}

void RTSGameState::AddUnit(std::unique_ptr<IGameUnit> unit)
{
    if (!unit) {
        LOG_ERROR("RTSGameState::AddUnit: Attempted to add null unit");
        return;
    }
    if (updating_) {
        pendingSpawns_.push_back(std::move(unit));
        return;
    }
    AdoptUnit(std::move(unit));
}

void RTSGameState::RemoveUnit(IGameUnit* unit)
{
    if (!unit) {
        return;
    }
    if (updating_) {
        if (std::find(pendingDespawns_.begin(), pendingDespawns_.end(), unit) == pendingDespawns_.end()) {
            pendingDespawns_.push_back(unit);
        }
        return;
    }
    DestroyUnit(unit);
}

uint64_t RTSGameState::GetEntityId(const IGameUnit* unit) const
{
    if (!unit) {
        return 0;
    }
    const auto found = entityIds_.find(unit);
    if (found != entityIds_.end()) {
        return found->second;
    }
    return unit->GetEntityId();
}

void RTSGameState::SetGameExtension(
    std::function<std::vector<uint8_t>()> capture,
    std::function<bool(const std::vector<uint8_t>&)> validate,
    std::function<void(const std::vector<uint8_t>&)> apply)
{
    extensionCapture_ = std::move(capture);
    extensionValidate_ = std::move(validate);
    extensionApply_ = std::move(apply);
}

void RTSGameState::Update(float deltaTime)
{
    if (paused_) {
        return;
    }

    ++simulationTick_;
    diagnostics_.clear();
    updating_ = true;
    ObserveDeaths();

    struct MotionNote {
        IGameUnit* unit = nullptr;
        Vector2 propulsion;
        Vector2 impulse;
        bool transitioned = false;
        std::string fromState;
        std::string toState;
        std::string behavior;
        bool record = false;
    };
    std::vector<MotionNote> notes;

    for (auto& held : units_)
    {
        IGameUnit* unit = held.get();
        IUnitBehaviorHost* behaviorHost = dynamic_cast<IUnitBehaviorHost*>(unit);
        if (!behaviorHost) continue;

        unit->Update(deltaTime);

        if (unit->IsInDebugMode())
        {
            continue;
        }

        Vector2 propulsion = Vector2::Zero();
        MotionNote note;
        note.unit = unit;
        note.record = diagnosticsEnabled_;

        if (unit->ParticipatesInActions())
        {
            BehaviorContext ctx = BuildContextForUnit(unit, deltaTime);
            if (auto* rtsUnit = dynamic_cast<RTSUnit*>(unit)) {
                const StateMachine::TransitionEval transition =
                    rtsUnit->GetStateMachine().EvaluateAutomaticTransitions(ctx);
                note.transitioned = transition.changed;
                note.fromState = transition.fromState;
                note.toState = transition.toState;
            }

            IBehavior* currentBehavior = behaviorHost->GetCurrentBehavior();
            if (currentBehavior)
            {
                note.behavior = currentBehavior->GetName() ? currentBehavior->GetName() : "";
                const auto& parameters = currentBehavior->GetParameters();
                const auto targetX = parameters.find("targetX");
                const auto targetY = parameters.find("targetY");
                if (targetX != parameters.end() && targetY != parameters.end() && note.record) {
                    // Target is recorded with the diagnostic below.
                }
                const float maxAcceleration = unit->GetMaxAcceleration();
                // Field sums stay unclamped here. The hub clamps behavior plus static once.
                const float fieldCap = std::numeric_limits<float>::infinity();
                std::vector<InfluenceFieldItem> behaviorItems =
                    currentBehavior->GenerateFieldItems(ctx);
                if (!behaviorItems.empty())
                {
                    propulsion = InfluenceField::CalculateTotalForce(
                        unit->GetPosition(), unit->GetVelocity(), behaviorItems, fieldCap);
                }
                else
                {
                    propulsion = currentBehavior->CalculateAcceleration(ctx);
                }
                if (!ctx.staticFieldItems.empty())
                {
                    propulsion += InfluenceField::CalculateTotalForce(
                        unit->GetPosition(), unit->GetVelocity(), ctx.staticFieldItems, fieldCap);
                }
                propulsion = ClampMagnitude(propulsion, maxAcceleration);

                if (unit->GetMotionIntegration() == MotionIntegration::SplitTick) {
                    if (unit->ParticipatesInMovement()) {
                        unit->ApplyPropulsion(propulsion, deltaTime);
                    }
                } else {
                    unit->ApplyAcceleration(propulsion, deltaTime);
                }
            }
        }
        note.propulsion = propulsion;
        notes.push_back(std::move(note));
    }

    for (const Explosion& explosion : explosions_)
    {
        std::vector<IGameUnit*> affectedUnits = GetUnitsInRadius(explosion.GetPosition(), explosion.GetRadius());
        for (IGameUnit* unit : affectedUnits)
        {
            if (unit->GetMotionIntegration() != MotionIntegration::SplitTick) continue;
            if (unit->IsInDebugMode() || !unit->ParticipatesInMovement()) continue;
            const Vector2 impulse = explosion.CalculateImpulse(unit->GetPosition(), unit->CalculateMass());
            unit->ApplyVelocityChange(impulse);
            for (MotionNote& note : notes) {
                if (note.unit == unit) {
                    note.impulse += impulse;
                }
            }
        }
    }
    explosions_.clear();

    for (const MotionNote& note : notes)
    {
        if (!note.unit) continue;
        if (note.unit->GetMotionIntegration() == MotionIntegration::SplitTick
            && note.unit->ParticipatesInMovement()
            && !note.unit->IsInDebugMode())
        {
            note.unit->IntegrateMotion(deltaTime);
        }
        if (!note.record) continue;
        UnitDiagnosticRecord record;
        record.entityId = GetEntityId(note.unit);
        record.tick = simulationTick_;
        record.behavior = note.behavior;
        record.transitioned = note.transitioned;
        record.fromState = note.fromState;
        record.toState = note.toState;
        record.propulsion = note.propulsion;
        record.impulse = note.impulse;
        if (IUnitBehaviorHost* host = dynamic_cast<IUnitBehaviorHost*>(note.unit)) {
            if (IBehavior* behavior = host->GetCurrentBehavior()) {
                const auto& parameters = behavior->GetParameters();
                const auto targetX = parameters.find("targetX");
                const auto targetY = parameters.find("targetY");
                if (targetX != parameters.end() && targetY != parameters.end()) {
                    record.hasTarget = true;
                    record.target = Vector2(targetX->second, targetY->second);
                }
            }
        }
        if (diagnosticExtension_) {
            diagnosticExtension_(*note.unit, record.extensions);
        }
        diagnostics_.push_back(std::move(record));
    }

    if (map_)
    {
        Vector2 mapSize = map_->GetDimensions();
        MapBounds bounds(Vector2::Zero(), mapSize);
        std::vector<IGameUnit*> colliding;
        for (const auto& unit : units_) {
            if (unit->ParticipatesInCollision()) {
                colliding.push_back(unit.get());
            }
        }
        collisionSystem_.ProcessCollisions(colliding, bounds);
    }

    projectiles_.Advance(deltaTime, GetAllUnits());
    actionEvents_ = actions_.Advance();
    collisionSystem_.UpdateImmunity();

    if (map_)
    {
        Vector2 mapSize = map_->GetDimensions();
        for (auto& unit : units_)
        {
            Vector2 pos = unit->GetPosition();
            bool outOfBounds = false;

            if (pos.x < 0.0f) { pos.x = 0.0f; outOfBounds = true; }
            if (pos.y < 0.0f) { pos.y = 0.0f; outOfBounds = true; }
            if (pos.x > mapSize.x) { pos.x = mapSize.x; outOfBounds = true; }
            if (pos.y > mapSize.y) { pos.y = mapSize.y; outOfBounds = true; }

            if (outOfBounds)
            {
                RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit.get());
                if (rtsUnit)
                {
                    rtsUnit->SetPosition(pos);
                }
            }
        }
    }

    ObserveDeaths();
    CommitStructure();
    updating_ = false;
}

BehaviorContext RTSGameState::BuildContextForUnit(IGameUnit* unit, float deltaTime)
{
    std::vector<IGameUnit*> others;
    others.reserve(units_.size());
    for (const auto& u : units_)
    {
        others.push_back(u.get());
    }

    std::vector<InfluenceFieldItem> obstacleItems;
    if (map_)
    {
        for (const auto& entity : map_->GetEntities())
        {
            auto* obstacleComp = entity->GetComponent<MapObstacleComponent>();
            if (obstacleComp)
            {
                obstacleItems.push_back(obstacleComp->GetInfluenceFieldItem());
            }
        }
    }

    BehaviorContextInputs inputs{
        unit,
        deltaTime,
        perceptionRadius_,
        GetMapCenter(),
        GetMapSize(),
        &rng_,
        others,
        mapEdgeItems_,
        staticAttractors_,
        obstacleItems};

    return AssembleBehaviorContext(inputs);
}

std::vector<IGameUnit*> RTSGameState::GetUnitsInRadius(Vector2 position, float radius) const
{
    std::vector<IGameUnit*> result;
    for (const auto& unit : units_)
    {
        if (unit->GetPosition().Distance(position) <= radius)
        {
            result.push_back(unit.get());
        }
    }
    return result;
}

std::vector<IGameUnit*> RTSGameState::GetTeamUnitsInRadius(Vector2 position, float radius, int team) const
{
    std::vector<IGameUnit*> result;
    for (const auto& unit : units_)
    {
        if (unit->GetTeam() == team && unit->GetPosition().Distance(position) <= radius)
        {
            result.push_back(unit.get());
        }
    }
    return result;
}

std::vector<IGameUnit*> RTSGameState::GetAllUnits() const
{
    std::vector<IGameUnit*> result;
    result.reserve(units_.size());
    for (const auto& unit : units_)
    {
        result.push_back(unit.get());
    }
    return result;
}

IGameUnit* RTSGameState::GetUnitAtPosition(Vector2 position, float tolerance) const
{
    for (const auto& unit : units_)
    {
        float distance = unit->GetPosition().Distance(position);
        if (distance <= unit->GetRadius() + tolerance)
        {
            return unit.get();
        }
    }
    return nullptr;
}

Vector2 RTSGameState::GetMapSize() const
{
    return map_ ? map_->GetDimensions() : Vector2(0.0f, 0.0f);
}

Vector2 RTSGameState::GetMapCenter() const
{
    return map_ ? map_->GetCenter() : Vector2(0.0f, 0.0f);
}

const std::vector<InfluenceFieldItem>& RTSGameState::GetStaticItems() const
{
    return mapEdgeItems_;
}

void RTSGameState::AddObstacle(const InfluenceFieldItem& obstacle)
{
    if (map_)
    {
        static int obstacleCounter = 0;
        std::string obstacleId = "obstacle_" + std::to_string(obstacleCounter++);

        auto entity = std::make_unique<MapEntity>(obstacleId, obstacle.GetPosition());
        float radius = obstacle.GetRepulsorParams().DetectionRange;
        auto obstacleComp = std::make_unique<MapObstacleComponent>(radius, obstacle);
        entity->AddComponent(std::move(obstacleComp));
        map_->AddEntity(std::move(entity));
    }
}

void RTSGameState::AddExplosion(const Explosion& explosion)
{
    explosions_.push_back(explosion);
}

void RTSGameState::ClearObstacles()
{
    // Remove all obstacle entities from map
    if (map_)
    {
        // Collect obstacle entity IDs
        std::vector<std::string> obstacleIds;
        for (const auto& entity : map_->GetEntities())
        {
            if (entity->GetComponent<MapObstacleComponent>())
            {
                obstacleIds.push_back(entity->GetId());
            }
        }
        // Remove them
        for (const auto& id : obstacleIds)
        {
            map_->RemoveEntity(id);
        }
    }
}

void RTSGameState::InitializeStaticFieldItems()
{
    // Create edge field items for the four map boundaries
    // These act as invisible walls that push units away from map edges

    if (!map_) return;

    Vector2 mapSize = map_->GetDimensions();

    EdgeParams edgeParams;
    edgeParams.RepulsorStrength = 150.0f;
    edgeParams.DetectionRange = 80.0f;

    mapEdgeItems_.clear();

    // Left edge (x = 0)
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Creating left edge...");
    mapEdgeItems_.push_back(InfluenceFieldItem::CreateEdge(
        Vector2(-edgeParams.DetectionRange * 0.5f, mapSize.y * 0.5f),
        edgeParams
    ));
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Left edge created");

    // Right edge (x = mapSize.x)
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Creating right edge...");
    mapEdgeItems_.push_back(InfluenceFieldItem::CreateEdge(
        Vector2(mapSize.x + edgeParams.DetectionRange * 0.5f, mapSize.y * 0.5f),
        edgeParams
    ));
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Right edge created");

    // Top edge (y = 0)
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Creating top edge...");
    mapEdgeItems_.push_back(InfluenceFieldItem::CreateEdge(
        Vector2(mapSize.x * 0.5f, -edgeParams.DetectionRange * 0.5f),
        edgeParams
    ));
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Top edge created");

    // Bottom edge (y = mapSize.y)
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Creating bottom edge...");
    mapEdgeItems_.push_back(InfluenceFieldItem::CreateEdge(
        Vector2(mapSize.x * 0.5f, mapSize.y + edgeParams.DetectionRange * 0.5f),
        edgeParams
    ));
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Bottom edge created");
    LOG_DEBUG("RTSGameState::InitializeStaticFieldItems: Completed, created %zu edge items", mapEdgeItems_.size());
}

void RTSGameState::AddStaticAttractor(const InfluenceFieldItem& attractor) {
    staticAttractors_.push_back(attractor);
}

void RTSGameState::ClearStaticAttractors() {
    staticAttractors_.clear();
}

