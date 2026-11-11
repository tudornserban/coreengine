#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "core/interfaces/ISimulationSnapshot.h"
#include "core/math/Vector2.h"
#include "core/serialization/GameStateSerializer.h"
#include "tests/TestHelpers/MockGameUnit.h"

namespace {

class FakeSimulationSnapshot : public ISimulationSnapshot {
public:
    FakeSimulationSnapshot(Vector2 mapSize, std::vector<IGameUnit*> units)
        : mapSize_(mapSize), units_(std::move(units)) {}

    Vector2 GetMapSize() const override { return mapSize_; }
    std::vector<IGameUnit*> GetAllUnits() const override { return units_; }

private:
    Vector2 mapSize_;
    std::vector<IGameUnit*> units_;
};

const char* kFakeSnapshotJson = R"({
  "version": 1,
  "format": "CoreEngine-JSON-v1",
  "map": { "width": 10, "height": 20 },
  "units": [
    { "team": 1, "x": 10, "y": 20, "vx": 0, "vy": 0, "hp": 100 }
  ]
}
)";

}  // namespace

TEST(GameStateSerializerSnapshotFakeTest, FakeSnapshotMatchesJsonShapeWithoutRtsGameState) {
    auto unit = std::make_unique<MockGameUnit>(Vector2(10.0f, 20.0f), /*team=*/1);
    FakeSimulationSnapshot snapshot(Vector2(10.0f, 20.0f), {unit.get()});
    GameStateSerializer serializer(&snapshot);

    EXPECT_EQ(serializer.SerializeToString(), kFakeSnapshotJson);
}

TEST(GameStateSerializerSnapshotFakeTest, SetSnapshotUsesInterfacePointer) {
    auto unit = std::make_unique<MockGameUnit>(Vector2(10.0f, 20.0f), 1);
    FakeSimulationSnapshot snapshot(Vector2(10.0f, 20.0f), {unit.get()});
    GameStateSerializer serializer;
    serializer.SetSnapshot(&snapshot);

    EXPECT_EQ(serializer.SerializeToString(), kFakeSnapshotJson);
}
