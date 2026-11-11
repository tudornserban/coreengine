#pragma once

#include "DrawList.h"

constexpr Rgba kRasterGuardSentinel{0xA1, 0xB2, 0xC3, 0xD4};

void Rasterize(const DrawList& list, Framebuffer& fb);

void FramebufferInitWithBottomRowGuard(Framebuffer& fb, int width, int height);
void FramebufferInitWithRightColumnGuard(Framebuffer& fb, int width, int height);
bool FramebufferGuardRowBelowIsClean(const Framebuffer& fb);
bool FramebufferGuardColumnRightIsClean(const Framebuffer& fb);
