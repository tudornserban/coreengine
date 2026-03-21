#pragma once

#include <vector>
#include "../math/Vector2.h"

class IGameUnit;

class SelectionSystem {
private:
    std::vector<IGameUnit*> selectedUnits;

    Vector2 selectionStart;
    Vector2 selectionEnd;
    bool isBoxSelecting;

public:
    SelectionSystem() : isBoxSelecting(false) {}

    // Single click selection
    void SelectUnit(IGameUnit* unit, bool addToSelection = false);
    void SelectUnits(const std::vector<IGameUnit*>& units, bool addToSelection = false);

    // Box selection
    void StartBoxSelection(Vector2 start);
    void UpdateBoxSelection(Vector2 current);
    void EndBoxSelection(const std::vector<IGameUnit*>& allUnits);

    // Clear selection
    void ClearSelection();

    // Getters
    const std::vector<IGameUnit*>& GetSelectedUnits() const { return selectedUnits; }
    bool IsBoxSelecting() const { return isBoxSelecting; }
    Vector2 GetBoxStart() const { return selectionStart; }
    Vector2 GetBoxEnd() const { return selectionEnd; }

private:
    bool IsPointInBox(Vector2 point, Vector2 boxMin, Vector2 boxMax) const;
};

