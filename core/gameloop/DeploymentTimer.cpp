#include "DeploymentTimer.h"
#include <cassert>

DeploymentTimer::DeploymentTimer(float timeRequired, float initialChargeFraction)
    : chargeTimeRequired(timeRequired)
    , currentChargeTime(timeRequired * initialChargeFraction)
{
    assert(timeRequired > 0.0f && "Time required must be greater than 0");
    assert(initialChargeFraction >= 0.0f && initialChargeFraction <= 1.0f
           && "Initial charge fraction must be in [0, 1]");
    while (currentChargeTime >= chargeTimeRequired) {
        ++charges;
        currentChargeTime -= chargeTimeRequired;
    }
}

void DeploymentTimer::Update(float deltaTime) {
    assert(deltaTime >= 0.0f && "Delta time must be non-negative");

    currentChargeTime += deltaTime;
    while (currentChargeTime >= chargeTimeRequired) {
        ++charges;
        currentChargeTime -= chargeTimeRequired;
    }
}

float DeploymentTimer::GetChargeProgress() const {
    if (charges > 0) return 1.0f;
    return currentChargeTime / chargeTimeRequired;
}

void DeploymentTimer::SpendCharge() {
    if (charges > 0) {
        charges--;
        currentChargeTime = 0.0f;
    }
}

void DeploymentTimer::GrantCharge() {
    ++charges;
}
