#include "Latency.h"

namespace Session
{

uint32_t InputDelayTicksForRtt(uint32_t rttMs, const LatencyProfile& profile)
{
    uint32_t tickHz = profile.tickHz == 0 ? 1u : profile.tickHz;
    uint32_t tickMs = 1000u / tickHz;
    if (tickMs == 0)
    {
        tickMs = 1;
    }

    const uint32_t oneWayMs = rttMs / 2u;
    uint32_t ticks = (oneWayMs + tickMs - 1u) / tickMs;
    ticks += profile.jitterMarginTicks;

    uint32_t minTicks = profile.minInputDelayTicks;
    uint32_t maxTicks = profile.maxInputDelayTicks;
    if (maxTicks < minTicks)
    {
        maxTicks = minTicks;
    }
    if (ticks < minTicks)
    {
        ticks = minTicks;
    }
    if (ticks > maxTicks)
    {
        ticks = maxTicks;
    }
    return ticks;
}

uint32_t InterpolationDelayMs(uint32_t rttMs)
{
    return rttMs / 2u;
}

}
