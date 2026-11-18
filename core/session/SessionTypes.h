#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Session protocol shared by the lobby server and its clients.
// Sockets stay out of core/; these values are the bytes a platform transport carries.
namespace Session
{

constexpr uint32_t kMaxLobbyPlayers = 8;
constexpr uint32_t kMaxPlayerNameLength = 24;
constexpr uint32_t kMaxJoinCodeLength = 8;
constexpr uint32_t kMaxPlanCommands = 4096;
constexpr uint32_t kMaxSnapshotBytes = 262144;
constexpr uint32_t kMaxNoticeLength = 128;
constexpr uint32_t kMaxInputLeadTicks = 64;
constexpr uint32_t kMatchSetupSchemaVersion = 1;
constexpr uint32_t kMaxSetupPayloadBytes = 4096;
constexpr uint32_t kMaxSetupReferenceLength = 128;

enum class GameTitle : uint8_t
{
    DynamicGo = 1,
    AscendantExpanse = 2,
    ZodiacWars = 3,
    VenusWars = 4
};

enum class PlayMode : uint8_t
{
    // Clients submit a full decision plan. The server runs the deterministic
    // sim once and sends the outcome back. No live tick sync.
    OfflineResolution = 1,
    // Server-authoritative lockstep. Clients lead by an RTT-sized input delay
    // and the server rolls back a short window when a command arrives late.
    OnlineSync = 2
};

enum class MessageType : uint16_t
{
    Ping = 1,
    Pong = 2,
    RttReport = 3,
    HashReport = 4,
    CreateLobby = 10,
    JoinLobby = 11,
    LeaveLobby = 12,
    SetReady = 13,
    StartMatch = 14,
    LobbySnapshot = 15,
    Left = 16,
    Enqueue = 20,
    QueueStatus = 21,
    SubmitPlan = 30,
    MatchResult = 31,
    SubmitSetup = 32,
    SubmitInput = 40,
    TickConfirm = 41,
    StateSnapshot = 42,
    Reject = 50
};

struct PlayerCommand
{
    uint32_t playerId = 0;
    uint32_t tick = 0;
    uint32_t action = 0;
    int32_t x = 0;
    int32_t y = 0;
};

struct DecisionPlan
{
    uint32_t playerId = 0;
    std::vector<PlayerCommand> commands;
};

struct MatchOutcome
{
    bool complete = false;
    bool forfeit = false;
    int32_t winnerPlayerId = -1;
    std::vector<int32_t> scores;
    uint64_t stateHash = 0;
    uint32_t ticksSimulated = 0;
};

struct LobbyPlayer
{
    uint32_t playerId = 0;
    std::string name;
    bool ready = false;
    bool host = false;
};

struct LobbyView
{
    uint32_t lobbyId = 0;
    std::string joinCode;
    GameTitle game = GameTitle::DynamicGo;
    PlayMode mode = PlayMode::OfflineResolution;
    uint32_t maxPlayers = 2;
    uint64_t seed = 0;
    bool started = false;
    uint32_t inputDelayTicks = 0;
    uint32_t interpolationDelayMs = 0;
    std::string notice;
    std::vector<LobbyPlayer> players;
};

struct Envelope
{
    MessageType type = MessageType::Reject;
    uint32_t playerId = 0;
    std::vector<uint8_t> payload;
};

// Opaque match configuration carried beside the lobby. The session layer checks
// schema, size, and reference shape. The game adapter checks the payload.
struct MatchSetup
{
    uint32_t schemaVersion = kMatchSetupSchemaVersion;
    std::string contentReference;
    std::vector<uint8_t> payload;
};

enum class SetupReject : uint8_t
{
    Ok = 0,
    UnsupportedVersion = 1,
    Oversized = 2,
    InvalidReference = 3
};

SetupReject ValidateMatchSetup(const MatchSetup& setup);

struct CreateLobbyRequest
{
    GameTitle game = GameTitle::DynamicGo;
    PlayMode mode = PlayMode::OfflineResolution;
    uint32_t maxPlayers = 2;
    uint64_t seed = 0;
};

struct JoinLobbyRequest
{
    std::string joinCode;
};

struct EnqueueRequest
{
    GameTitle game = GameTitle::DynamicGo;
    PlayMode mode = PlayMode::OfflineResolution;
    uint32_t playersNeeded = 2;
};

struct QueueStatusMessage
{
    GameTitle game = GameTitle::DynamicGo;
    PlayMode mode = PlayMode::OfflineResolution;
    uint32_t playersNeeded = 2;
    uint32_t ahead = 0;
};

struct TimestampMessage
{
    uint32_t clientTimeMs = 0;
};

struct HashReportMessage
{
    uint32_t tick = 0;
    uint64_t stateHash = 0;
};

struct TickConfirmMessage
{
    uint32_t tick = 0;
    uint64_t stateHash = 0;
    bool corrected = false;
    uint32_t inputDelayTicks = 0;
    uint32_t interpolationDelayMs = 0;
    std::vector<uint8_t> snapshot;
};

struct StateSnapshotMessage
{
    uint32_t tick = 0;
    uint64_t stateHash = 0;
    std::vector<uint8_t> snapshot;
};

const char* ToString(GameTitle game);
const char* ToString(PlayMode mode);
bool TryParseGameTitle(const std::string& text, GameTitle& game);
bool TryParsePlayMode(const std::string& text, PlayMode& mode);

}
