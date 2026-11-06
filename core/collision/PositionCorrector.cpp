#include "PositionCorrector.h"
#include "../interfaces/IUnitKinematics.h"
#include "../math/Vector2.h"

void PositionCorrector::CorrectAgentAgent(
    IGameUnit* unitA,
    IGameUnit* unitB,
    const CollisionContact& contact,
    float correctionFactor,
    float massA,
    float massB) {

    if (!unitA || !unitB || correctionFactor <= 0.0f) return;

    // Calculate correction amount
    float correction = contact.penetration * correctionFactor;

    // Distribute correction based on mass (heavier objects move less)
    float totalMass = massA + massB;
    if (totalMass < 0.0001f) return;

    float ratioA = massB / totalMass;
    float ratioB = massA / totalMass;

    Vector2 correctionVector = contact.normal * correction;

    // Apply corrections (opposite directions)
    ApplyCorrection(unitA, -correctionVector * ratioA);
    ApplyCorrection(unitB, correctionVector * ratioB);
}

void PositionCorrector::CorrectAgentMap(
    IGameUnit* unit,
    const CollisionContact& contact,
    float correctionFactor) {

    if (!unit || correctionFactor <= 0.0f) return;

    // Calculate correction amount
    float correction = contact.penetration * correctionFactor;
    Vector2 correctionVector = contact.normal * correction;

    // Push agent away from map edge
    ApplyCorrection(unit, -correctionVector);
}

void PositionCorrector::ApplyCorrection(IGameUnit* unit, Vector2 correction) {
    if (!unit) return;

    IUnitKinematics* kinematics = dynamic_cast<IUnitKinematics*>(unit);
    if (kinematics) {
        Vector2 currentPos = unit->GetPosition();
        kinematics->SetPosition(currentPos + correction);
    }
}

