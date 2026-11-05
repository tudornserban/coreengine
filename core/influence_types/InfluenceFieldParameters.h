#pragma once

/// Calculation type for attractive forces.
enum class AttractorType {
    LINEAR,                     // Force proportional to distance
    QUADRATIC,                  // Force proportional to distance squared
    CONIC,                      // Constant magnitude (direction only)
    HYBRID_PARABOLIC_CONIC      // Parabolic near target, conic far away
};

/// Parameters for attractive force calculations.
struct AttractorParams {
    float AttractorStrength       = 50.0f;   ///< ξ - Scaling factor
    float TargetApproachDistance  = 100.0f;  ///< d - Hybrid switch threshold
    float TargetVelocityMatching_m = 2.0f;  ///< m - Position exponent (dynamic)
    float TargetVelocityMatching_n = 2.0f;  ///< n - Velocity exponent (dynamic)
    float VelocityWeight_p        = 1.0f;   ///< αp - Position weight (dynamic)
    float VelocityWeight_v        = 1.0f;   ///< αv - Velocity weight (dynamic)
    AttractorType functionType    = AttractorType::HYBRID_PARABOLIC_CONIC;
};

/// Parameters for repulsive force calculations.
struct RepulsorParams {
    float RepulsorStrength        = 100.0f; ///< η - Scaling factor
    float DetectionRange          = 50.0f;  ///< ρ₀ - Maximum range
    float DistanceAttenuation     = 2.0f;   ///< Distance falloff exponent
    bool  DynamicThreatAssessment = false;  ///< Consider relative velocity
    float MaxBrakingAcceleration  = 200.0f; ///< a_max_rep - Braking distance calc
};

/// Simplified parameters for map edges and static obstacles.
struct EdgeParams {
    float RepulsorStrength = 150.0f;
    float DetectionRange   = 80.0f;
};

/// Container for all default influence field parameters.
struct InfluenceFieldParameters {
    AttractorParams attractorDefaults;
    RepulsorParams  repulsorDefaults;
    EdgeParams      edgeDefaults;
    float           maxAcceleration = 300.0f;
};
