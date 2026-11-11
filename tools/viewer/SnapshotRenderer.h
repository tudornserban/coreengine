#pragma once

#include "Camera.h"
#include "DrawList.h"

class ISimulationSnapshot;

Rgba ColorForTeam(int team);
DrawList BuildSnapshotDrawList(const ISimulationSnapshot& snapshot, const Camera& camera);
