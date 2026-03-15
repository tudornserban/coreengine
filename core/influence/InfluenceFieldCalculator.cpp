#include "InfluenceFieldCalculator.h"
#include <cmath>
#include <algorithm>

namespace InfluenceField {

static Vector2 CalcLinearAttraction(const Vector2& toTarget, float distance, const AttractorParams& p)
{
    return toTarget.Normalized() * p.AttractorStrength;
}

static Vector2 CalcQuadraticAttraction(const Vector2& toTarget, float distance, const AttractorParams& p)
{
    return toTarget.Normalized() * p.AttractorStrength * distance;
}

static Vector2 CalcConicAttraction(const Vector2& toTarget, float distance, const AttractorParams& p)
{
    return toTarget.Normalized() * p.AttractorStrength;
}

static Vector2 CalcHybridAttraction(const Vector2& toTarget, float distance, const AttractorParams& p)
{
    if (distance <= p.TargetApproachDistance) {
        return toTarget.Normalized() * p.AttractorStrength * distance;
    }
    float mag = p.AttractorStrength * p.TargetApproachDistance;
    return toTarget.Normalized() * mag;
}

Vector2 CalculateAttractorForce(
    const Vector2& agentPos,
    const Vector2& agentVel,
    const InfluenceFieldItem& item)
{
    Vector2 toTarget = item.GetPosition() - agentPos;
    float distance = toTarget.Length();
    if (distance < 0.001f) return Vector2::Zero();

    const AttractorParams& p = item.GetAttractorParams();

    switch (p.functionType) {
        case AttractorType::LINEAR:
            return CalcLinearAttraction(toTarget, distance, p) * item.GetImportance();
        case AttractorType::QUADRATIC:
            return CalcQuadraticAttraction(toTarget, distance, p) * item.GetImportance();
        case AttractorType::CONIC:
            return CalcConicAttraction(toTarget, distance, p) * item.GetImportance();
        case AttractorType::HYBRID_PARABOLIC_CONIC:
        default:
            return CalcHybridAttraction(toTarget, distance, p) * item.GetImportance();
    }
}

Vector2 CalculateRepulsorForce(
    const Vector2& agentPos,
    const Vector2& agentVel,
    const InfluenceFieldItem& item)
{
    Vector2 toAgent = agentPos - item.GetPosition();
    float dist = toAgent.Length();

    const RepulsorParams& p = item.GetRepulsorParams();

    if (dist < 0.001f || dist >= p.DetectionRange) return Vector2::Zero();

    Vector2 n_away = toAgent / dist;

    if (p.DynamicThreatAssessment) {
        Vector2 relVel = agentVel - item.GetVelocity();
        float v_RO = -relVel.Dot(n_away);
        if (v_RO <= 0.0f) return Vector2::Zero();

        float rho_m = (v_RO * v_RO) / (2.0f * p.MaxBrakingAcceleration);
        float adjusted = dist - rho_m;
        if (adjusted <= 0.0f || adjusted >= p.DetectionRange) return Vector2::Zero();

        float inv = 1.0f / adjusted - 1.0f / p.DetectionRange;
        float mag = p.RepulsorStrength * inv * (1.0f / (adjusted * adjusted));
        return n_away * mag * item.GetImportance();
    }

    float inv = 1.0f / dist - 1.0f / p.DetectionRange;
    float mag = p.RepulsorStrength * inv * (1.0f / (dist * dist));
    return n_away * mag * item.GetImportance();
}

Vector2 CalculateEdgeForce(
    const Vector2& agentPos,
    const InfluenceFieldItem& item)
{
    Vector2 toAgent = agentPos - item.GetPosition();
    float dist = toAgent.Length();
    const EdgeParams& p = item.GetEdgeParams();

    if (dist < 0.001f || dist >= p.DetectionRange) return Vector2::Zero();

    float inv = 1.0f / dist - 1.0f / p.DetectionRange;
    float mag = p.RepulsorStrength * inv * (1.0f / (dist * dist));
    return toAgent.Normalized() * mag;
}

Vector2 CalculateTotalForce(
    const Vector2& agentPos,
    const Vector2& agentVel,
    const std::vector<InfluenceFieldItem>& items,
    float maxAcceleration)
{
    Vector2 total = Vector2::Zero();

    for (const auto& item : items) {
        switch (item.GetInfluenceType()) {
            case InfluenceType::ATTRACTOR:
                total += CalculateAttractorForce(agentPos, agentVel, item);
                break;
            case InfluenceType::REPULSOR:
                total += CalculateRepulsorForce(agentPos, agentVel, item);
                break;
            case InfluenceType::EDGE:
                total += CalculateEdgeForce(agentPos, item);
                break;
            case InfluenceType::CUSTOM:
                total += CalculateAttractorForce(agentPos, agentVel, item);
                total += CalculateRepulsorForce(agentPos, agentVel, item);
                break;
        }
    }

    return Vector2::ClampMagnitude(total, maxAcceleration);
}

} // namespace InfluenceField
