#pragma once

struct CollisionSettings {
    float defaultRestitution = 0.7f;
    float mapRestitution = 0.8f;
    float positionCorrection = 0.2f;  // Optional, can be 0.0
    int immunityFrames = 3;  // Game update frames
    float defaultMass = 100.0f;
    float damageModifier = 1.0f;
};

