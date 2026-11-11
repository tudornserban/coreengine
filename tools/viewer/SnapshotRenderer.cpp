#include "SnapshotRenderer.h"

#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/ISimulationSnapshot.h"

Rgba ColorForTeam(int team)
{
    switch (team)
    {
    case 0:
        return {80, 200, 120, 255};
    case 1:
        return {70, 140, 230, 255};
    case 2:
        return {220, 80, 80, 255};
    default:
        return {230, 230, 230, 255};
    }
}

DrawList BuildSnapshotDrawList(const ISimulationSnapshot& snapshot, const Camera& camera)
{
    DrawList list;
    list.clear = {20, 24, 28, 255};

    const Vector2 mapSize = snapshot.GetMapSize();
    const Vector2 worldMin(0.0f, 0.0f);
    const Vector2 worldMax = mapSize;
    const Vector2 screenMin = camera.WorldToScreen(worldMin);
    const Vector2 screenMax = camera.WorldToScreen(worldMax);

    DrawRect fill;
    fill.min = screenMin;
    fill.max = screenMax;
    fill.filled = true;
    fill.color = {32, 36, 42, 255};
    list.rects.push_back(fill);

    DrawRect border;
    border.min = screenMin;
    border.max = screenMax;
    border.filled = false;
    border.color = {180, 180, 180, 255};
    list.rects.push_back(border);

    constexpr Rgba kSelectionOutline{255, 220, 40, 255};

    for (IGameUnit* unit : snapshot.GetAllUnits())
    {
        if (unit == nullptr)
        {
            continue;
        }
        DrawCircle circle;
        circle.center = camera.WorldToScreen(unit->GetPosition());
        circle.radius = unit->GetRadius() * camera.pixelsPerUnit;
        circle.fill = ColorForTeam(unit->GetTeam());
        circle.outline = unit->IsSelected() ? kSelectionOutline : circle.fill;
        list.circles.push_back(circle);
    }

    return list;
}
