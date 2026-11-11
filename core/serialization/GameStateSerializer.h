#pragma once

#include "core/interfaces/ISerializer.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Forward declarations
class RTSGameState;
class ISimulationSnapshot;

/// JSON serializer for RTSGameState.
///
/// Serializes the complete simulation state to/from JSON, enabling:
///   - Server-side validation (DynamicGO server mode)
///   - Deterministic replay (load seed + command log, re-simulate)
///   - FFI data exchange (call GetStateJSON() from any language via C API)
///   - Network sync (send/receive match snapshots)
///
/// Format version is embedded in every payload for forward compatibility.
class GameStateSerializer : public ISerializer {
private:
    const ISimulationSnapshot* snapshot_;
    static constexpr uint32_t FORMAT_VERSION = 1;

public:
    explicit GameStateSerializer(const RTSGameState* state = nullptr);
    explicit GameStateSerializer(const ISimulationSnapshot* snapshot);
    explicit GameStateSerializer(std::nullptr_t) : snapshot_(nullptr) {}

    void SetState(const RTSGameState* state);
    void SetSnapshot(const ISimulationSnapshot* snapshot);

    // --- ISerializer ---
    std::vector<uint8_t> Serialize() const override;
    std::string SerializeToString() const override;
    bool Deserialize(const std::vector<uint8_t>& data) override;
    bool DeserializeFromString(const std::string& data) override;
    const char* GetFormatName() const override { return "CoreEngine-JSON-v1"; }
    uint32_t GetVersion() const override { return FORMAT_VERSION; }

    // --- Convenience ---
    /// Returns a JSON string snapshot of the game state (for FFI/debug).
    std::string GetStateJSON() const { return SerializeToString(); }
};
