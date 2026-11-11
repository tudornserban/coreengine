#pragma once

#include <vector>
#include <memory>
#include "../interfaces/IGameUnit.h"
#include "../interfaces/ISimulationSnapshot.h"
#include "../ai/BehaviorContext.h"
#include "../math/Vector2.h"
#include "../math/DeterministicRNG.h"
#include "../combat/SelectionSystem.h"
#include "../combat/CommandSystem.h"
#include "../influence/InfluenceFieldItem.h"
#include "../map/Map.h"
#include "../collision/CollisionSystem.h"
#include "../combat/Explosion.h"

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

public:
    RTSGameState(Vector2 mapBounds, uint64_t rngSeed = 0);
    RTSGameState(std::unique_ptr<Map> mapInstance, uint64_t rngSeed = 0);

    // Unit management
    void AddUnit(std::unique_ptr<IGameUnit> unit);
    void RemoveUnit(IGameUnit* unit);

    // Main update
    void Update(float deltaTime);

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
};

