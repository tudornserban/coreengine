#include "SpatialGrid.h"
#include "../interfaces/IGameUnit.h"
#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid(float cellSize, Vector2 worldMin, Vector2 worldMax)
    : cellSize(cellSize)
    , worldMin(worldMin)
    , worldMax(worldMax) {

    gridWidth = static_cast<int>(std::ceil((worldMax.x - worldMin.x) / cellSize));
    gridHeight = static_cast<int>(std::ceil((worldMax.y - worldMin.y) / cellSize));

    grid.resize(gridHeight);
    for (int y = 0; y < gridHeight; y++) {
        grid[y].resize(gridWidth);
    }
}

int SpatialGrid::GetCellX(float x) const {
    int cellX = static_cast<int>((x - worldMin.x) / cellSize);
    return std::max(0, std::min(gridWidth - 1, cellX));
}

int SpatialGrid::GetCellY(float y) const {
    int cellY = static_cast<int>((y - worldMin.y) / cellSize);
    return std::max(0, std::min(gridHeight - 1, cellY));
}

bool SpatialGrid::IsValidCell(int x, int y) const {
    return x >= 0 && x < gridWidth && y >= 0 && y < gridHeight;
}

void SpatialGrid::Clear() {
    for (auto& row : grid) {
        for (auto& cell : row) {
            cell.units.clear();
        }
    }
}

void SpatialGrid::Insert(IGameUnit* unit) {
    if (!unit) return;

    Vector2 pos = unit->GetPosition();
    int cellX = GetCellX(pos.x);
    int cellY = GetCellY(pos.y);

    if (IsValidCell(cellX, cellY)) {
        grid[cellY][cellX].units.push_back(unit);
    }
}

std::vector<std::pair<IGameUnit*, IGameUnit*>> SpatialGrid::GetPotentialPairs() const {
    std::vector<std::pair<IGameUnit*, IGameUnit*>> pairs;
    std::set<std::pair<IGameUnit*, IGameUnit*>> seenPairs;  // Prevent duplicate pairs

    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            const Cell& cell = grid[y][x];

            // Check pairs within the same cell
            for (size_t i = 0; i < cell.units.size(); i++) {
                for (size_t j = i + 1; j < cell.units.size(); j++) {
                    IGameUnit* unitA = cell.units[i];
                    IGameUnit* unitB = cell.units[j];

                    // Ensure consistent ordering (pointer comparison)
                    IGameUnit* first = (unitA < unitB) ? unitA : unitB;
                    IGameUnit* second = (unitA < unitB) ? unitB : unitA;

                    auto pair = std::make_pair(first, second);
                    if (seenPairs.find(pair) == seenPairs.end()) {
                        seenPairs.insert(pair);
                        pairs.push_back(pair);
                    }
                }
            }

            // Check pairs with adjacent cells (to catch edge cases)
            // Check right neighbor
            if (x + 1 < gridWidth) {
                const Cell& rightCell = grid[y][x + 1];
                for (IGameUnit* unitA : cell.units) {
                    for (IGameUnit* unitB : rightCell.units) {
                        IGameUnit* first = (unitA < unitB) ? unitA : unitB;
                        IGameUnit* second = (unitA < unitB) ? unitB : unitA;

                        auto pair = std::make_pair(first, second);
                        if (seenPairs.find(pair) == seenPairs.end()) {
                            seenPairs.insert(pair);
                            pairs.push_back(pair);
                        }
                    }
                }
            }

            // Check bottom neighbor
            if (y + 1 < gridHeight) {
                const Cell& bottomCell = grid[y + 1][x];
                for (IGameUnit* unitA : cell.units) {
                    for (IGameUnit* unitB : bottomCell.units) {
                        IGameUnit* first = (unitA < unitB) ? unitA : unitB;
                        IGameUnit* second = (unitA < unitB) ? unitB : unitA;

                        auto pair = std::make_pair(first, second);
                        if (seenPairs.find(pair) == seenPairs.end()) {
                            seenPairs.insert(pair);
                            pairs.push_back(pair);
                        }
                    }
                }
            }

            // Check bottom-right neighbor
            if (x + 1 < gridWidth && y + 1 < gridHeight) {
                const Cell& diagCell = grid[y + 1][x + 1];
                for (IGameUnit* unitA : cell.units) {
                    for (IGameUnit* unitB : diagCell.units) {
                        IGameUnit* first = (unitA < unitB) ? unitA : unitB;
                        IGameUnit* second = (unitA < unitB) ? unitB : unitA;

                        auto pair = std::make_pair(first, second);
                        if (seenPairs.find(pair) == seenPairs.end()) {
                            seenPairs.insert(pair);
                            pairs.push_back(pair);
                        }
                    }
                }
            }
        }
    }

    return pairs;
}

std::vector<IGameUnit*> SpatialGrid::GetUnitsInCell(int x, int y) const {
    if (!IsValidCell(x, y)) {
        return std::vector<IGameUnit*>();
    }
    return grid[y][x].units;
}

std::vector<IGameUnit*> SpatialGrid::GetUnitsNear(Vector2 position, float radius) const {
    std::vector<IGameUnit*> result;

    // Calculate cell range
    int minX = GetCellX(position.x - radius);
    int maxX = GetCellX(position.x + radius);
    int minY = GetCellY(position.y - radius);
    int maxY = GetCellY(position.y + radius);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (IsValidCell(x, y)) {
                for (IGameUnit* unit : grid[y][x].units) {
                    float distance = unit->GetPosition().Distance(position);
                    if (distance <= radius) {
                        result.push_back(unit);
                    }
                }
            }
        }
    }

    return result;
}

