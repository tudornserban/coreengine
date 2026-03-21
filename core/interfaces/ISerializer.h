#pragma once

#include <string>
#include <vector>
#include <cstdint>

/// Serialize/deserialize game state to/from a portable byte buffer.
/// Enables: server-side validation, deterministic replays, and FFI data exchange.
///
/// Implement with JSON (nlohmann/json), MessagePack, FlatBuffers, or any
/// other format — the core engine depends only on this interface.
class ISerializer {
public:
    virtual ~ISerializer() = default;

    // --- Serialization ---
    virtual std::vector<uint8_t> Serialize() const = 0;
    virtual std::string SerializeToString() const = 0;

    // --- Deserialization ---
    virtual bool Deserialize(const std::vector<uint8_t>& data) = 0;
    virtual bool DeserializeFromString(const std::string& data) = 0;

    // --- Metadata ---
    virtual const char* GetFormatName() const = 0;
    virtual uint32_t GetVersion() const = 0;
};
