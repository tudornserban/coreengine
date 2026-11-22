#pragma once

#include "core/math/Vector2.h"
#include "core/math/Vector3.h"
#include "core/math/MathConstants.h"

#include <cmath>
#include <cstdint>
#include <functional>

namespace CoreEngine {
namespace Unreal {

/// Lightweight 3D vector matching Unreal's FVector layout (X forward/east, Y right/south, Z up in cm).
struct UEVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    UEVec3() = default;
    constexpr UEVec3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {}

    UEVec3 operator+(const UEVec3& other) const { return UEVec3(x + other.x, y + other.y, z + other.z); }
    UEVec3 operator-(const UEVec3& other) const { return UEVec3(x - other.x, y - other.y, z - other.z); }
    UEVec3 operator*(float scalar) const { return UEVec3(x * scalar, y * scalar, z * scalar); }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3 ToCoreVector3() const { return Vector3(x, y, z); }
    static UEVec3 FromCoreVector3(const Vector3& v) { return UEVec3(v.x, v.y, v.z); }
};

/// Lightweight rotator matching Unreal's FRotator (Pitch, Yaw, Roll in degrees).
struct UERotator {
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    UERotator() = default;
    constexpr UERotator(float inPitch, float inYaw, float inRoll)
        : pitch(inPitch), yaw(inYaw), roll(inRoll) {}
};

/// RGBA color matching Unreal's FColor / FLinearColor 8-bit representation.
struct UEColor {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    UEColor() = default;
    constexpr UEColor(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inA = 255)
        : r(inR), g(inG), b(inB), a(inA) {}

    static constexpr UEColor Team1Blue()    { return UEColor(70, 140, 230, 255); }
    static constexpr UEColor Team2Red()     { return UEColor(230, 80, 75, 255); }
    static constexpr UEColor Team3Green()   { return UEColor(75, 210, 120, 255); }
    static constexpr UEColor Team4Gold()    { return UEColor(240, 195, 65, 255); }
    static constexpr UEColor NeutralGray()  { return UEColor(160, 165, 175, 255); }
    static constexpr UEColor SelectedGold() { return UEColor(255, 225, 60, 255); }
    static constexpr UEColor DeadDark()     { return UEColor(85, 85, 90, 180); }
    static constexpr UEColor VelocityCyan() { return UEColor(60, 230, 250, 255); }
    static constexpr UEColor PropulsionGreen() { return UEColor(80, 240, 110, 255); }
    static constexpr UEColor ImpulseOrange() { return UEColor(255, 140, 40, 255); }
    static constexpr UEColor ObstacleSlate() { return UEColor(110, 115, 130, 255); }
    static constexpr UEColor AttractorPurple() { return UEColor(185, 100, 255, 255); }

    static UEColor ForTeam(int teamId) {
        switch (teamId) {
            case 1: return Team1Blue();
            case 2: return Team2Red();
            case 3: return Team3Green();
            case 4: return Team4Gold();
            default: return NeutralGray();
        }
    }
};

/// 3D transform for actors and placeholder primitives.
struct UETransform {
    UEVec3 location{0.0f, 0.0f, 0.0f};
    UERotator rotation{0.0f, 0.0f, 0.0f};
    UEVec3 scale{1.0f, 1.0f, 1.0f};
};

/// Configuration for mapping CoreEngine 2D/3D simulation coordinates to UE5 3D world space.
struct UECoordinateConfig {
    /// Centimeters per CoreEngine world unit (default 10.0: an 800x600 arena is 80m x 60m in UE5).
    float worldScale = 10.0f;
    /// UE5 world-space origin corresponding to CoreEngine (0, 0).
    UEVec3 originOffset{0.0f, 0.0f, 0.0f};
    /// Default ground plane Z in UE5 world space when no height callback is set.
    float defaultGroundZ = 0.0f;
    /// If true, negates CoreEngine Y when converting to UE5 Y.
    bool invertY = false;
    /// Optional terrain height query (takes CoreEngine x, y; returns UE5 world Z in cm).
    std::function<float(float, float)> heightQuery;
};

/// Zero-overhead coordinate & transform bridge between CoreEngine and Unreal Engine 5.
class UECoordinateBridge {
private:
    UECoordinateConfig config_;

public:
    explicit UECoordinateBridge(UECoordinateConfig config = UECoordinateConfig{})
        : config_(std::move(config)) {
        if (config_.worldScale <= 0.0f) {
            config_.worldScale = 1.0f;
        }
    }

    const UECoordinateConfig& GetConfig() const { return config_; }
    void SetConfig(UECoordinateConfig config) {
        config_ = std::move(config);
        if (config_.worldScale <= 0.0f) {
            config_.worldScale = 1.0f;
        }
    }

    float GetWorldScale() const { return config_.worldScale; }
    void SetWorldScale(float scale) {
        if (scale > 0.0f) {
            config_.worldScale = scale;
        }
    }

    /// Convert CoreEngine scalar distance/radius to UE5 centimeters.
    float CoreToUEDistance(float coreDistance) const {
        return coreDistance * config_.worldScale;
    }

    /// Convert UE5 centimeters to CoreEngine scalar distance/radius.
    float UEToCoreDistance(float ueDistance) const {
        return ueDistance / config_.worldScale;
    }

    /// Query ground Z in UE5 world space for a CoreEngine 2D position.
    float SampleGroundZ(Vector2 corePos) const {
        if (config_.heightQuery) {
            return config_.heightQuery(corePos.x, corePos.y);
        }
        return config_.originOffset.z + config_.defaultGroundZ;
    }

    /// Convert a CoreEngine 2D position to a UE5 3D world position.
    UEVec3 CoreToUEPosition(Vector2 corePos, float heightOffsetUE = 0.0f) const {
        const float ySign = config_.invertY ? -1.0f : 1.0f;
        const float ueX = config_.originOffset.x + corePos.x * config_.worldScale;
        const float ueY = config_.originOffset.y + (corePos.y * ySign) * config_.worldScale;
        const float ueZ = SampleGroundZ(corePos) + heightOffsetUE;
        return UEVec3(ueX, ueY, ueZ);
    }

    /// Convert a CoreEngine 3D position to a UE5 3D world position.
    UEVec3 Core3DToUEPosition(const Vector3& corePos) const {
        const float ySign = config_.invertY ? -1.0f : 1.0f;
        return UEVec3(
            config_.originOffset.x + corePos.x * config_.worldScale,
            config_.originOffset.y + (corePos.y * ySign) * config_.worldScale,
            config_.originOffset.z + corePos.z * config_.worldScale);
    }

    /// Convert a UE5 3D world position to a CoreEngine 2D arena position (projects onto XY).
    Vector2 UEToCorePosition(const UEVec3& uePos) const {
        const float ySign = config_.invertY ? -1.0f : 1.0f;
        const float coreX = (uePos.x - config_.originOffset.x) / config_.worldScale;
        const float coreY = ((uePos.y - config_.originOffset.y) / config_.worldScale) * ySign;
        return Vector2(coreX, coreY);
    }

    /// Convert a CoreEngine 2D velocity/force vector to a UE5 3D directional vector.
    UEVec3 CoreToUEVector(Vector2 coreVec, float zComponentUE = 0.0f) const {
        const float ySign = config_.invertY ? -1.0f : 1.0f;
        return UEVec3(
            coreVec.x * config_.worldScale,
            (coreVec.y * ySign) * config_.worldScale,
            zComponentUE);
    }

    /// Compute heading Yaw in degrees [-180, 180] in UE5 world space from a CoreEngine 2D direction/velocity.
    float ComputeHeadingYawDegrees(Vector2 coreDirection, float fallbackYawDegrees = 0.0f) const {
        if (coreDirection.LengthSquared() <= CoreEngine::Math::EPSILON * CoreEngine::Math::EPSILON) {
            return fallbackYawDegrees;
        }
        const float ySign = config_.invertY ? -1.0f : 1.0f;
        const float rad = std::atan2(coreDirection.y * ySign, coreDirection.x);
        return rad * CoreEngine::Math::RAD_TO_DEG;
    }

    /// Build a complete 3D actor transform for a CoreEngine unit.
    UETransform BuildUnitTransform(Vector2 corePos, Vector2 coreVelocity, float capsuleHalfHeightUE,
                                   float fallbackYawDegrees = 0.0f) const {
        UETransform xf;
        xf.location = CoreToUEPosition(corePos, capsuleHalfHeightUE);
        xf.rotation = UERotator(0.0f, ComputeHeadingYawDegrees(coreVelocity, fallbackYawDegrees), 0.0f);
        xf.scale = UEVec3(1.0f, 1.0f, 1.0f);
        return xf;
    }

    /// Intersect a 3D UE5 camera/mouse ray with the ground plane (Z = originOffset.z + defaultGroundZ)
    /// and output the hit point in CoreEngine 2D coordinates.
    bool RaycastGroundToCore(const UEVec3& rayOriginUE, const UEVec3& rayDirectionUE,
                             Vector2* outCorePos) const {
        if (!outCorePos) {
            return false;
        }
        if (std::fabs(rayDirectionUE.z) <= 1e-5f) {
            return false;
        }
        const float planeZ = config_.originOffset.z + config_.defaultGroundZ;
        const float t = (planeZ - rayOriginUE.z) / rayDirectionUE.z;
        if (t < 0.0f) {
            return false;
        }
        const UEVec3 hitUE = rayOriginUE + rayDirectionUE * t;
        *outCorePos = UEToCorePosition(hitUE);
        return true;
    }
};

} // namespace Unreal
} // namespace CoreEngine
