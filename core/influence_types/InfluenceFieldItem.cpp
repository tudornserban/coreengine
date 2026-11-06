#include "InfluenceFieldItem.h"
#include <algorithm>

InfluenceFieldItem::InfluenceFieldItem()
    : position_(Vector2::Zero()), velocity_(Vector2::Zero())
    , importance_(1.0f), influenceType_(InfluenceType::CUSTOM) {}

InfluenceFieldItem::InfluenceFieldItem(Vector2 pos, Vector2 vel, float importance, InfluenceType type)
    : position_(pos), velocity_(vel), importance_(importance), influenceType_(type) {}

InfluenceFieldItem InfluenceFieldItem::CreateAttractor(Vector2 position, float importance, const AttractorParams& params)
{
    InfluenceFieldItem item(position, Vector2::Zero(), importance, InfluenceType::ATTRACTOR);
    item.attractorParams_ = params;
    return item;
}

InfluenceFieldItem InfluenceFieldItem::CreateAttractor(Vector2 position, Vector2 velocity, float importance, const AttractorParams& params)
{
    InfluenceFieldItem item(position, velocity, importance, InfluenceType::ATTRACTOR);
    item.attractorParams_ = params;
    return item;
}

InfluenceFieldItem InfluenceFieldItem::CreateRepulsor(Vector2 position, float importance, const RepulsorParams& params)
{
    InfluenceFieldItem item(position, Vector2::Zero(), importance, InfluenceType::REPULSOR);
    item.repulsorParams_ = params;
    return item;
}

InfluenceFieldItem InfluenceFieldItem::CreateRepulsor(Vector2 position, Vector2 velocity, float importance, const RepulsorParams& params)
{
    InfluenceFieldItem item(position, velocity, importance, InfluenceType::REPULSOR);
    item.repulsorParams_ = params;
    return item;
}

InfluenceFieldItem InfluenceFieldItem::CreateEdge(Vector2 position, const EdgeParams& params)
{
    InfluenceFieldItem item(position, Vector2::Zero(), 1.0f, InfluenceType::EDGE);
    item.edgeParams_ = params;
    return item;
}

InfluenceFieldItem InfluenceFieldItem::operator+(const InfluenceFieldItem& other) const
{
    float total = importance_ + other.importance_;
    if (total < 0.0001f) return InfluenceFieldItem();

    Vector2 newPos = (position_ * importance_ + other.position_ * other.importance_) / total;
    Vector2 newVel = (velocity_ * importance_ + other.velocity_ * other.importance_) / total;
    InfluenceType newType = ReconcileTypes(influenceType_, other.influenceType_);

    InfluenceFieldItem result(newPos, newVel, total, newType);

    float w1 = importance_ / total;
    float w2 = other.importance_ / total;

    if (newType == InfluenceType::ATTRACTOR || newType == InfluenceType::CUSTOM) {
        result.attractorParams_.AttractorStrength =
            attractorParams_.AttractorStrength * w1 + other.attractorParams_.AttractorStrength * w2;
        result.attractorParams_.TargetApproachDistance =
            attractorParams_.TargetApproachDistance * w1 + other.attractorParams_.TargetApproachDistance * w2;
        result.attractorParams_.functionType =
            (importance_ > other.importance_) ? attractorParams_.functionType : other.attractorParams_.functionType;
    }

    if (newType == InfluenceType::REPULSOR || newType == InfluenceType::CUSTOM) {
        result.repulsorParams_.RepulsorStrength =
            repulsorParams_.RepulsorStrength * w1 + other.repulsorParams_.RepulsorStrength * w2;
        result.repulsorParams_.DetectionRange =
            std::max(repulsorParams_.DetectionRange, other.repulsorParams_.DetectionRange);
    }

    return result;
}

InfluenceFieldItem InfluenceFieldItem::Aggregate(const std::vector<InfluenceFieldItem>& items)
{
    if (items.empty()) return InfluenceFieldItem();
    if (items.size() == 1) return items[0];

    InfluenceFieldItem result = items[0];
    for (size_t i = 1; i < items.size(); ++i) {
        result = result + items[i];
    }
    return result;
}

InfluenceType InfluenceFieldItem::ReconcileTypes(InfluenceType t1, InfluenceType t2)
{
    if (t1 == t2) return t1;
    if (t1 == InfluenceType::CUSTOM) return t2;
    if (t2 == InfluenceType::CUSTOM) return t1;
    if (t1 == InfluenceType::EDGE)   return t2;
    if (t2 == InfluenceType::EDGE)   return t1;
    return InfluenceType::CUSTOM;
}