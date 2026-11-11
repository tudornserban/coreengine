#include "GameStateSerializer.h"

#include "core/gamestate/RTSGameState.h"
#include "core/interfaces/ISimulationSnapshot.h"

#include <sstream>

// NOTE: Full serialization requires a JSON library (e.g. nlohmann/json).
// This implementation provides the minimal scaffold; extend as needed.
// The serialize/deserialize cycle must be lossless for deterministic replays.

GameStateSerializer::GameStateSerializer(const RTSGameState* state) : snapshot_(state) {}

GameStateSerializer::GameStateSerializer(const ISimulationSnapshot* snapshot) : snapshot_(snapshot) {}

void GameStateSerializer::SetState(const RTSGameState* state)
{
    snapshot_ = state;
}

void GameStateSerializer::SetSnapshot(const ISimulationSnapshot* snapshot)
{
    snapshot_ = snapshot;
}

std::string GameStateSerializer::SerializeToString() const
{
    if (!snapshot_) return "{}";

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"version\": " << FORMAT_VERSION << ",\n";
    oss << "  \"format\": \"" << GetFormatName() << "\",\n";

    Vector2 mapSize = snapshot_->GetMapSize();
    oss << "  \"map\": { \"width\": " << mapSize.x << ", \"height\": " << mapSize.y << " },\n";

    auto units = snapshot_->GetAllUnits();
    oss << "  \"units\": [\n";
    for (size_t i = 0; i < units.size(); ++i) {
        const IGameUnit* u = units[i];
        oss << "    { \"team\": " << u->GetTeam()
            << ", \"x\": " << u->GetPosition().x
            << ", \"y\": " << u->GetPosition().y
            << ", \"vx\": " << u->GetVelocity().x
            << ", \"vy\": " << u->GetVelocity().y
            << ", \"hp\": " << u->GetCurrentHP()
            << " }";
        if (i + 1 < units.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    return oss.str();
}

std::vector<uint8_t> GameStateSerializer::Serialize() const
{
    std::string s = SerializeToString();
    return std::vector<uint8_t>(s.begin(), s.end());
}

bool GameStateSerializer::DeserializeFromString(const std::string& /*data*/)
{
    // Full deserialization requires a JSON parser.
    // Implement using nlohmann/json or similar when needed for replays.
    return false;
}

bool GameStateSerializer::Deserialize(const std::vector<uint8_t>& data)
{
    return DeserializeFromString(std::string(data.begin(), data.end()));
}
