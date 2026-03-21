#pragma once

#include "../math/Vector2.h"
#include <vector>
#include <unordered_map>
#include <set>

class IGameUnit;

class SpatialGrid {
private:
    struct Cell {
        std::vector<IGameUnit*> units;
    };

    float cellSize;
    int gridWidth;
    int gridHeight;
    Vector2 worldMin;
    Vector2 worldMax;
    std::vector<std::vector<Cell>> grid;

    int GetCellX(float x) const;
    int GetCellY(float y) const;
    bool IsValidCell(int x, int y) const;

public:
    SpatialGrid(float cellSize, Vector2 worldMin, Vector2 worldMax);

    void Clear();
    void Insert(IGameUnit* unit);

    // Get potential collision pairs
    std::vector<std::pair<IGameUnit*, IGameUnit*>> GetPotentialPairs() const;

    // Get units in a specific cell
    std::vector<IGameUnit*> GetUnitsInCell(int x, int y) const;

    // Get units near a position
    std::vector<IGameUnit*> GetUnitsNear(Vector2 position, float radius) const;
};

