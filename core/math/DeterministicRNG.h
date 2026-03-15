#pragma once

#include <cstdint>

// Deterministic Random Number Generator
// Uses a seeded linear congruential generator for reproducible randomness
// Same seed always produces the same sequence of random numbers
class DeterministicRNG {
private:
    uint64_t state;
    uint64_t seed;

public:
    DeterministicRNG(uint64_t initialSeed = 0);
    
    // Set seed and reset state
    void SetSeed(uint64_t newSeed);
    
    // Get current seed
    uint64_t GetSeed() const { return seed; }
    
    // Generate random numbers
    uint32_t NextUInt32();
    int32_t NextInt32();
    int32_t NextInt32(int32_t min, int32_t max);  // Inclusive range [min, max]
    
    float NextFloat();  // [0.0, 1.0)
    float NextFloat(float min, float max);  // [min, max)
    
    double NextDouble();  // [0.0, 1.0)
    double NextDouble(double min, double max);  // [min, max)
    
    // Reset to initial seed
    void Reset();
    
    // Get current state (for save/restore)
    uint64_t GetState() const { return state; }
    void SetState(uint64_t newState) { state = newState; }

private:
    // LCG constants (used in many PRNG implementations)
    static constexpr uint64_t LCG_MULTIPLIER = 1103515245ULL;
    static constexpr uint64_t LCG_INCREMENT = 12345ULL;
    static constexpr uint64_t LCG_MODULUS = 0x7FFFFFFFULL;
};

