#pragma once

#include "../math/Vector2.h"
#include "InfluenceFieldParameters.h"
#include <vector>

/// Type of influence this item exerts on an agent.
enum class InfluenceType {
    ATTRACTOR,  ///< Attracts the agent (goal, resource, ally beacon)
    REPULSOR,   ///< Repels the agent (enemy, obstacle, danger zone)
    EDGE,       ///< Map boundary — optimized calculation
    CUSTOM      ///< User-defined (used after mixing attractor + repulsor)
};

/// Unified influence field entry.
///
/// Replaces VenusWars PotentialFieldItem with a generic name.
/// Used by InfluenceFieldCalculator to compute total acceleration for a unit.
/// Behaviors return std::vector<InfluenceFieldItem> from GenerateFieldItems().
class InfluenceFieldItem {
private:
    Vector2       position_;
    Vector2       velocity_;
    float         importance_;
    InfluenceType influenceType_;

    AttractorParams attractorParams_;
    RepulsorParams  repulsorParams_;
    EdgeParams      edgeParams_;

public:
    InfluenceFieldItem();
    InfluenceFieldItem(Vector2 pos, Vector2 vel, float importance, InfluenceType type);

    // --- Factory methods ---
    static InfluenceFieldItem CreateAttractor(
        Vector2 position, float importance = 1.0f,
        const AttractorParams& params = AttractorParams());

    static InfluenceFieldItem CreateAttractor(
        Vector2 position, Vector2 velocity, float importance = 1.0f,
        const AttractorParams& params = AttractorParams());

    static InfluenceFieldItem CreateRepulsor(
        Vector2 position, float importance = 1.0f,
        const RepulsorParams& params = RepulsorParams());

    static InfluenceFieldItem CreateRepulsor(
        Vector2 position, Vector2 velocity, float importance = 1.0f,
        const RepulsorParams& params = RepulsorParams());

    static InfluenceFieldItem CreateEdge(
        Vector2 position,
        const EdgeParams& params = EdgeParams());

    // --- Getters ---
    Vector2       GetPosition()      const { return position_; }
    Vector2       GetVelocity()      const { return velocity_; }
    float         GetImportance()    const { return importance_; }
    InfluenceType GetInfluenceType() const { return influenceType_; }

    const AttractorParams& GetAttractorParams() const { return attractorParams_; }
    const RepulsorParams&  GetRepulsorParams()  const { return repulsorParams_; }
    const EdgeParams&      GetEdgeParams()      const { return edgeParams_; }

    // --- Setters ---
    void SetPosition(Vector2 pos)   { position_ = pos; }
    void SetVelocity(Vector2 vel)   { velocity_ = vel; }
    void SetImportance(float imp)   { importance_ = imp; }

    // --- Aggregation ---
    InfluenceFieldItem operator+(const InfluenceFieldItem& other) const;
    static InfluenceFieldItem Aggregate(const std::vector<InfluenceFieldItem>& items);

private:
    static InfluenceType ReconcileTypes(InfluenceType t1, InfluenceType t2);
};