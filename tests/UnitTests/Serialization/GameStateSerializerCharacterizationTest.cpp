#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/math/Vector2.h"
#include "core/serialization/GameStateSerializer.h"

namespace {

constexpr uint64_t kSeed = 0;

const char* kOneUnitJson = R"({
  "version": 1,
  "format": "CoreEngine-JSON-v1",
  "map": { "width": 100, "height": 100 },
  "units": [
    { "team": 1, "x": 10, "y": 20, "vx": 0, "vy": 0, "hp": 100 }
  ]
}
)";

RTSGameState MakeMap100() {
    return RTSGameState(Vector2(100.0f, 100.0f), kSeed);
}

}  // namespace

TEST(GameStateSerializerCharacterizationTest, NullStateSerializesEmptyObject) {
    GameStateSerializer serializer(nullptr);
    EXPECT_EQ(serializer.SerializeToString(), "{}");
}

TEST(GameStateSerializerCharacterizationTest, DeserializeAlwaysReturnsFalse) {
    GameStateSerializer serializer;
    EXPECT_FALSE(serializer.DeserializeFromString("{}"));
    EXPECT_FALSE(serializer.Deserialize(std::vector<uint8_t>{'{', '}'}));
}

TEST(GameStateSerializerCharacterizationTest, OneRtsUnitExactJsonPin) {
    RTSGameState state = MakeMap100();
    auto unit = std::make_unique<RTSUnit>(Vector2(10.0f, 20.0f), /*team=*/1);
    state.AddUnit(std::move(unit));

    GameStateSerializer serializer(&state);
    EXPECT_EQ(serializer.SerializeToString(), kOneUnitJson);
}

TEST(GameStateSerializerCharacterizationTest, SecondUnitAppearsSecondWithComma) {
    RTSGameState state = MakeMap100();
    state.AddUnit(std::make_unique<RTSUnit>(Vector2(10.0f, 20.0f), 1));
    state.AddUnit(std::make_unique<RTSUnit>(Vector2(30.0f, 40.0f), 0));

    const std::string json = GameStateSerializer(&state).SerializeToString();
    const std::string expectedFirstObject =
        R"(    { "team": 1, "x": 10, "y": 20, "vx": 0, "vy": 0, "hp": 100 })";
    const std::string expectedSecondObject =
        R"(    { "team": 0, "x": 30, "y": 40, "vx": 0, "vy": 0, "hp": 100 })";

    const auto firstPos = json.find(expectedFirstObject);
    const auto secondPos = json.find(expectedSecondObject);
    ASSERT_NE(firstPos, std::string::npos);
    ASSERT_NE(secondPos, std::string::npos);
    EXPECT_LT(firstPos, secondPos);
    EXPECT_EQ(json[firstPos + expectedFirstObject.size()], ',');
}
