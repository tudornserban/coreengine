#include "SelectionSystem.h"
#include "core/interfaces/IGameUnit.h"
#include <algorithm>

void SelectionSystem::SelectUnit(IGameUnit* unit, bool addToSelection) {
    if (!unit) return;

    if (!addToSelection) {
        ClearSelection();
    }

    // Toggle if already selected
    auto it = std::find(selectedUnits.begin(), selectedUnits.end(), unit);
    if (it != selectedUnits.end()) {
        unit->SetSelected(false);
        selectedUnits.erase(it);
    } else {
        unit->SetSelected(true);
        selectedUnits.push_back(unit);
    }
}

void SelectionSystem::SelectUnits(const std::vector<IGameUnit*>& units, bool addToSelection) {
    if (!addToSelection) {
        ClearSelection();
    }

    for (IGameUnit* unit : units) {
        if (unit && std::find(selectedUnits.begin(), selectedUnits.end(), unit) == selectedUnits.end()) {
            unit->SetSelected(true);
            selectedUnits.push_back(unit);
        }
    }
}

void SelectionSystem::StartBoxSelection(Vector2 start) {
    selectionStart = start;
    selectionEnd = start;
    isBoxSelecting = true;
}

void SelectionSystem::UpdateBoxSelection(Vector2 current) {
    if (isBoxSelecting) {
        selectionEnd = current;
    }
}

void SelectionSystem::EndBoxSelection(const std::vector<IGameUnit*>& allUnits) {
    if (!isBoxSelecting) return;

    isBoxSelecting = false;

    // Calculate box bounds
    Vector2 boxMin(
        std::min(selectionStart.x, selectionEnd.x),
        std::min(selectionStart.y, selectionEnd.y)
    );
    Vector2 boxMax(
        std::max(selectionStart.x, selectionEnd.x),
        std::max(selectionStart.y, selectionEnd.y)
    );

    // Select units in box
    std::vector<IGameUnit*> unitsInBox;
    for (IGameUnit* unit : allUnits) {
        if (unit && IsPointInBox(unit->GetPosition(), boxMin, boxMax)) {
            unitsInBox.push_back(unit);
        }
    }

    SelectUnits(unitsInBox, false);
}

void SelectionSystem::ClearSelection() {
    for (IGameUnit* unit : selectedUnits) {
        if (unit) {
            unit->SetSelected(false);
        }
    }
    selectedUnits.clear();
}

bool SelectionSystem::IsPointInBox(Vector2 point, Vector2 boxMin, Vector2 boxMax) const {
    return point.x >= boxMin.x && point.x <= boxMax.x &&
           point.y >= boxMin.y && point.y <= boxMax.y;
}

