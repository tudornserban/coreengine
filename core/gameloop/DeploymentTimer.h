#pragma once


class DeploymentTimer {
private:
    float chargeTimeRequired;
    float currentChargeTime = 0.0f;
    int charges = 0;
public:
    // timeRequired: seconds to accumulate one charge.
    // initialChargeFraction: [0, 1] pre-fill applied at construction.
    //   0.0 = empty (default), 1.0 = immediately ready.
    // Charges keep accumulating across Update calls (multi-charge bank).
    explicit DeploymentTimer(float timeRequired, float initialChargeFraction = 0.0f);
    void Update(float deltaTime);
    [[nodiscard]] bool HasCharge() const { return charges > 0; }
    [[nodiscard]] int GetCharges() const { return charges; }
    // Returns [0, 1]: 1.0 when a charge is ready, otherwise the fill fraction.
    [[nodiscard]] float GetChargeProgress() const;
    void SpendCharge();
    /// Grant one charge immediately (ability / shared-charge hook).
    void GrantCharge();
};

