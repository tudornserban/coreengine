#pragma once

#include <cstdint>

namespace Session
{

// tickHz is the authoritative server rate (the engine logic rate is 30).
// Input delay hides one-way latency: the client plays that many ticks ahead
// so a command is already waiting when the server reaches its tick.
// rollbackWindowTicks is how long a late command can still correct the sim.
struct LatencyProfile
{
    uint32_t tickHz = 30;
    uint32_t minInputDelayTicks = 1;
    uint32_t maxInputDelayTicks = 8;
    uint32_t jitterMarginTicks = 1;
    uint32_t rollbackWindowTicks = 6;
};

uint32_t InputDelayTicksForRtt(uint32_t rttMs, const LatencyProfile& profile);
uint32_t InterpolationDelayMs(uint32_t rttMs);

}
