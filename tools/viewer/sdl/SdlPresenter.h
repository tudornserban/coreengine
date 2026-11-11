#pragma once

#include "DrawList.h"
#include "FrameLoop.h"
#include "ViewerConfig.h"

bool PresentSimulation(FrameLoop& loop, const ViewerConfig& config, Framebuffer& framebuffer);
