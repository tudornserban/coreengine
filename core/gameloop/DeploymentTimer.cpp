#include "DeploymentTimer.h"
#include <cassert>

DeploymentTimer::DeploymentTimer(float timeRequired) : chargeTimeRequired(timeRequired) {
    assert(timeRequired > 0.0f && "Time required must be greater than 0");
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
