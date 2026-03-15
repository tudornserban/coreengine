#include <gtest/gtest.h>
#include "core/math/DeterministicRNG.h"
#include <vector>

// ============================================================
// Seed / reproducibility
// ============================================================

TEST(DeterministicRNGTest, SameSeedSameFirstValue) {
    DeterministicRNG a(12345), b(12345);
    EXPECT_EQ(a.NextUInt32(), b.NextUInt32());
}

TEST(DeterministicRNGTest, SameSeedIdenticalSequence) {
    DeterministicRNG a(99999), b(99999);
    std::vector<uint32_t> sa, sb;
    for (int i = 0; i < 100; ++i) {
        sa.push_back(a.NextUInt32());
        sb.push_back(b.NextUInt32());
    }
    EXPECT_EQ(sa, sb);
}

TEST(DeterministicRNGTest, DifferentSeedsDifferentFirstValue) {
    DeterministicRNG a(1), b(2);
    // Extremely unlikely to collide; serves as a sanity check
    EXPECT_NE(a.NextUInt32(), b.NextUInt32());
}

// ============================================================
// Reset
// ============================================================

TEST(DeterministicRNGTest, ResetRestoresSequence) {
    DeterministicRNG rng(12345);
    uint32_t first  = rng.NextUInt32();
    uint32_t second = rng.NextUInt32();
    rng.Reset();
    EXPECT_EQ(first,  rng.NextUInt32());
    EXPECT_EQ(second, rng.NextUInt32());
}

// ============================================================
// SetSeed
// ============================================================

TEST(DeterministicRNGTest, SetSeedMatchesFreshRNG) {
    DeterministicRNG rng(12345);
    rng.NextUInt32();          // advance state
    rng.SetSeed(67890);
    uint32_t fromSetSeed = rng.NextUInt32();

    DeterministicRNG fresh(67890);
    EXPECT_EQ(fromSetSeed, fresh.NextUInt32());
}

// ============================================================
// NextInt32 range
// ============================================================

TEST(DeterministicRNGTest, NextInt32RangeAlwaysInBounds) {
    DeterministicRNG rng(42);
    for (int i = 0; i < 200; ++i) {
        int32_t v = rng.NextInt32(10, 20);
        EXPECT_GE(v, 10);
        EXPECT_LE(v, 20);
    }
}

TEST(DeterministicRNGTest, NextInt32SingleValueRange) {
    DeterministicRNG rng(42);
    EXPECT_EQ(rng.NextInt32(7, 7), 7);
}

// ============================================================
// NextFloat
// ============================================================

TEST(DeterministicRNGTest, NextFloatInUnitRange) {
    DeterministicRNG rng(123);
    for (int i = 0; i < 200; ++i) {
        float v = rng.NextFloat();
        EXPECT_GE(v, 0.0f);
        EXPECT_LT(v, 1.0f);
    }
}

TEST(DeterministicRNGTest, NextFloatRangeAlwaysInBounds) {
    DeterministicRNG rng(123);
    for (int i = 0; i < 200; ++i) {
        float v = rng.NextFloat(10.0f, 20.0f);
        EXPECT_GE(v, 10.0f);
        EXPECT_LT(v, 20.0f);
    }
}

TEST(DeterministicRNGTest, NextFloatSingleValueRange) {
    DeterministicRNG rng(123);
    EXPECT_FLOAT_EQ(rng.NextFloat(5.0f, 5.0f), 5.0f);
}

// ============================================================
// NextDouble
// ============================================================

TEST(DeterministicRNGTest, NextDoubleInUnitRange) {
    DeterministicRNG rng(456);
    for (int i = 0; i < 200; ++i) {
        double v = rng.NextDouble();
        EXPECT_GE(v, 0.0);
        EXPECT_LT(v, 1.0);
    }
}

// ============================================================
// State save/restore
// ============================================================

TEST(DeterministicRNGTest, GetSetStateRestoresSequence) {
    DeterministicRNG rng(12345);
    rng.NextUInt32();
    rng.NextUInt32();

    uint64_t saved    = rng.GetState();
    uint32_t expected = rng.NextUInt32();

    rng.SetState(saved);
    EXPECT_EQ(rng.NextUInt32(), expected);
}
