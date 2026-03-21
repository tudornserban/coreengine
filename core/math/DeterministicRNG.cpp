#include "DeterministicRNG.h"
#include <algorithm>
#include <cmath>

DeterministicRNG::DeterministicRNG(uint64_t initialSeed)
    : seed(initialSeed), state(initialSeed) {
}

void DeterministicRNG::SetSeed(uint64_t newSeed) {
    seed = newSeed;
    state = newSeed;
}

void DeterministicRNG::Reset() {
    state = seed;
}

uint32_t DeterministicRNG::NextUInt32() {
    // Linear Congruential Generator
    state = (LCG_MULTIPLIER * state + LCG_INCREMENT) & 0x7FFFFFFFULL;
    return static_cast<uint32_t>(state);
}

int32_t DeterministicRNG::NextInt32() {
    return static_cast<int32_t>(NextUInt32());
}

int32_t DeterministicRNG::NextInt32(int32_t min, int32_t max) {
    if (min >= max) return min;
    uint32_t range = static_cast<uint32_t>(max - min + 1);
    uint32_t random = NextUInt32() % range;
    return min + static_cast<int32_t>(random);
}

float DeterministicRNG::NextFloat() {
    // Convert to float in range [0.0, 1.0)
    return static_cast<float>(NextUInt32()) / static_cast<float>(LCG_MODULUS + 1ULL);
}

float DeterministicRNG::NextFloat(float min, float max) {
    if (min >= max) return min;
    return min + NextFloat() * (max - min);
}

double DeterministicRNG::NextDouble() {
    // Convert to double in range [0.0, 1.0)
    return static_cast<double>(NextUInt32()) / static_cast<double>(LCG_MODULUS + 1ULL);
}

double DeterministicRNG::NextDouble(double min, double max) {
    if (min >= max) return min;
    return min + NextDouble() * (max - min);
}

