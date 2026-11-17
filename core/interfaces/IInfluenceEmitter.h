#pragma once

/// Optional influence-disc surface. Not part of IGameUnit.
/// Concrete units that project a territorial or attrition radius implement this.
class IInfluenceEmitter {
public:
    virtual ~IInfluenceEmitter() = default;

    virtual float GetInfluenceRadius() const = 0;
    virtual void SetInfluenceRadius(float radius) = 0;
};
