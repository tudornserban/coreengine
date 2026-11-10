#include "RTSGameState.h"
#include "BehaviorContextAssembler.h"
#include "../combat/Explosion.h"
#include "RTSUnit.h"
#include <algorithm>
#include "../map/MapComponents/MapObstacleComponent.h"
#include "../influence/InfluenceFieldItem.h"
#include "../influence/InfluenceFieldCalculator.h"
#include "../collision/ICollisionDetector.h"
#include "../math/DeterministicRNG.h"
#include "../logging/Logger.h"

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

void RTSGameState::AddUnit(std::unique_ptr<IGameUnit> unit)
{
    if (!unit) {
        LOG_ERROR("RTSGameState::AddUnit: Attempted to add null unit");
        return;
    }
    
    IGameUnit* unitPtr = unit.get();
    LOG_DEBUG("RTSGameState::AddUnit: Adding unit at %p, position=(%.2f, %.2f), team=%d", 
              unitPtr, unitPtr->GetPosition().x, unitPtr->GetPosition().y, unitPtr->GetTeam());
    
    units_.push_back(std::move(unit));
    
    LOG_DEBUG("RTSGameState::AddUnit: Unit added successfully. Total units: %zu", units_.size());
}

void RTSGameState::RemoveUnit(IGameUnit* unit)
{
    auto it = std::find_if(units_.begin(), units_.end(),
        [unit](const std::unique_ptr<IGameUnit>& u) { return u.get() == unit; });

    if (it != units_.end())
    {
        units_.erase(it);
    }
}

void RTSGameState::Update(float deltaTime)
{
    // Build context and update each unit
    for (auto& unit : units_)
    {
        RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit.get());
        if (!rtsUnit) continue;

        // Update debug movement first
        rtsUnit->Update(deltaTime);

        // If in debug mode, skip AI
        if (rtsUnit->IsInDebugMode())
        {
            // Apply debug movement
            Vector2 toTarget = rtsUnit->GetPosition();  // Position already updated in Update()
            continue;
        }

        // Build behavior context
        BehaviorContext ctx = BuildContextForUnit(unit.get(), deltaTime);

        // Get current behavior and calculate acceleration
        IBehavior* currentBehavior = rtsUnit->GetStateMachine().GetCurrentBehavior();
        if (currentBehavior)
        {
            Vector2 acceleration;

            // Behavior field items + static influences (map edges, obstacles, static attractors)
            std::vector<InfluenceFieldItem> items = currentBehavior->GenerateFieldItems(ctx);
            items.insert(items.end(), ctx.staticFieldItems.begin(), ctx.staticFieldItems.end());

            if (!items.empty())
            {
                acceleration = InfluenceField::CalculateTotalForce(
                    rtsUnit->GetPosition(),
                    rtsUnit->GetVelocity(),
                    items,
                    300.0f  // max acceleration
                );
            }
            else
            {
                acceleration = currentBehavior->CalculateAcceleration(ctx);
            }

            rtsUnit->ApplyAcceleration(acceleration, deltaTime);
        }
    }

    // Process collisions (runs in game loop)
    if (map_)
    {
        Vector2 mapSize = map_->GetDimensions();
        MapBounds bounds(Vector2::Zero(), mapSize);

        std::vector<IGameUnit*> allUnits = GetAllUnits();
        collisionSystem_.ProcessCollisions(allUnits, bounds);
    }

    // Process explosions (created by collisions or other sources)
    // Apply impulse forces to units within explosion radius
    for (const Explosion& explosion : explosions_)
    {
        // Find units within explosion radius
        std::vector<IGameUnit*> affectedUnits = GetUnitsInRadius(explosion.GetPosition(), explosion.GetRadius());

        // Apply impulse to each affected unit
        for (IGameUnit* unit : affectedUnits)
        {
            RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit);
            if (!rtsUnit) continue;

            // Calculate impulse for this unit
            Vector2 impulse = explosion.CalculateImpulse(rtsUnit->GetPosition(), rtsUnit->CalculateMass());

            // Apply impulse as acceleration (impulse is force, convert to acceleration)
            // Impulse = force * deltaTime, so acceleration = impulse / deltaTime
            // But since we want instant impulse, we apply it directly as velocity change
            if (deltaTime > 0.0f)
            {
                Vector2 velocityChange = impulse / deltaTime;
                rtsUnit->ApplyAcceleration(velocityChange, deltaTime);
            }
        }
    }

    // Clear explosions after processing (single frame duration)
    explosions_.clear();

    // Update immunity system (decrement frames)
    collisionSystem_.UpdateImmunity();

    // Keep units within bounds (fallback - collision system should handle this)
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

