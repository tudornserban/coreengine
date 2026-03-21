#pragma once


class DeploymentTimer {
private:
    float chargeTimeRequired;
    float currentChargeTime = 0.0f;
    int charges = 0;
public:
    explicit DeploymentTimer(float timeRequired);
    void Update(float deltaTime);
    [[nodiscard]] bool HasCharge() const { return charges > 0; }
    [[nodiscard]] int GetCharges() const { return charges; }
    // Returns [0, 1]: 1.0 when a charge is ready, otherwise the fill fraction.
    [[nodiscard]] float GetChargeProgress() const;
    void SpendCharge();
};

