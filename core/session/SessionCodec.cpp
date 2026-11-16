#include "SessionCodec.h"

namespace Session
{
namespace
{

class ByteWriter
{
public:
    void U8(uint8_t value) { bytes_.push_back(value); }

    void U16(uint16_t value)
    {
        U8(static_cast<uint8_t>(value & 0xffu));
        U8(static_cast<uint8_t>((value >> 8) & 0xffu));
    }

    void U32(uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8)
        {
            U8(static_cast<uint8_t>((value >> shift) & 0xffu));
        }
    }

    void U64(uint64_t value)
    {
        for (int shift = 0; shift < 64; shift += 8)
        {
            U8(static_cast<uint8_t>((value >> shift) & 0xffu));
        }
    }

    void I32(int32_t value) { U32(static_cast<uint32_t>(value)); }

    bool Str(const std::string& text, uint32_t maxLength)
    {
        if (text.size() > maxLength || text.size() > 0xffffu)
        {
            return false;
        }
        U16(static_cast<uint16_t>(text.size()));
        bytes_.insert(bytes_.end(), text.begin(), text.end());
        return true;
    }

    bool Blob(const std::vector<uint8_t>& data, uint32_t maxLength)
    {
        if (data.size() > maxLength)
        {
            return false;
        }
        U32(static_cast<uint32_t>(data.size()));
        bytes_.insert(bytes_.end(), data.begin(), data.end());
        return true;
    }

    std::vector<uint8_t> Take() { return std::move(bytes_); }
    bool Ok() const { return ok_; }
    void Fail() { ok_ = false; }

private:
    std::vector<uint8_t> bytes_;
    bool ok_ = true;
};

class ByteReader
{
public:
    explicit ByteReader(const std::vector<uint8_t>& bytes)
        : data_(bytes.data())
        , size_(bytes.size())
    {
    }

    uint8_t U8()
    {
        if (offset_ >= size_)
        {
            ok_ = false;
            return 0;
        }
        return data_[offset_++];
    }

    uint16_t U16()
    {
        uint16_t value = U8();
        value |= static_cast<uint16_t>(U8()) << 8;
        return value;
    }

    uint32_t U32()
    {
        uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
        {
            value |= static_cast<uint32_t>(U8()) << shift;
        }
        return value;
    }

    uint64_t U64()
    {
        uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 8)
        {
            value |= static_cast<uint64_t>(U8()) << shift;
        }
        return value;
    }

    int32_t I32() { return static_cast<int32_t>(U32()); }

    std::string Str(uint32_t maxLength)
    {
        const uint16_t length = U16();
        if (!ok_ || length > maxLength || offset_ + length > size_)
        {
            ok_ = false;
            return {};
        }
        std::string text(reinterpret_cast<const char*>(data_ + offset_), length);
        offset_ += length;
        return text;
    }

    std::vector<uint8_t> Blob(uint32_t maxLength)
    {
        const uint32_t length = U32();
        if (!ok_ || length > maxLength || offset_ + length > size_)
        {
            ok_ = false;
            return {};
        }
        std::vector<uint8_t> data(data_ + offset_, data_ + offset_ + length);
        offset_ += length;
        return data;
    }

    bool Finish() const { return ok_ && offset_ == size_; }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t offset_ = 0;
    bool ok_ = true;
};

bool WriteGame(ByteWriter& writer, GameTitle game)
{
    const uint8_t raw = static_cast<uint8_t>(game);
    if (raw < 1 || raw > 3)
    {
        writer.Fail();
        return false;
    }
    writer.U8(raw);
    return true;
}

bool WriteMode(ByteWriter& writer, PlayMode mode)
{
    const uint8_t raw = static_cast<uint8_t>(mode);
    if (raw < 1 || raw > 2)
    {
        writer.Fail();
        return false;
    }
    writer.U8(raw);
    return true;
}

bool ReadGame(ByteReader& reader, GameTitle& game)
{
    const uint8_t raw = reader.U8();
    if (raw < 1 || raw > 3)
    {
        return false;
    }
    game = static_cast<GameTitle>(raw);
    return true;
}

bool ReadMode(ByteReader& reader, PlayMode& mode)
{
    const uint8_t raw = reader.U8();
    if (raw < 1 || raw > 2)
    {
        return false;
    }
    mode = static_cast<PlayMode>(raw);
    return true;
}

bool WriteCommand(ByteWriter& writer, const PlayerCommand& command)
{
    writer.U32(command.playerId);
    writer.U32(command.tick);
    writer.U32(command.action);
    writer.I32(command.x);
    writer.I32(command.y);
    return writer.Ok();
}

bool ReadCommand(ByteReader& reader, PlayerCommand& command)
{
    command.playerId = reader.U32();
    command.tick = reader.U32();
    command.action = reader.U32();
    command.x = reader.I32();
    command.y = reader.I32();
    return true;
}

std::vector<uint8_t> FinishWrite(ByteWriter& writer)
{
    if (!writer.Ok())
    {
        return {};
    }
    return writer.Take();
}

}

std::vector<uint8_t> EncodeEnvelope(const Envelope& message)
{
    ByteWriter writer;
    writer.U16(static_cast<uint16_t>(message.type));
    writer.U32(message.playerId);
    if (!writer.Blob(message.payload, kMaxSnapshotBytes))
    {
        return {};
    }
    return FinishWrite(writer);
}

bool DecodeEnvelope(const std::vector<uint8_t>& bytes, Envelope& message)
{
    ByteReader reader(bytes);
    const uint16_t type = reader.U16();
    message.playerId = reader.U32();
    message.payload = reader.Blob(kMaxSnapshotBytes);
    if (!reader.Finish())
    {
        return false;
    }
    switch (static_cast<MessageType>(type))
    {
    case MessageType::Ping:
    case MessageType::Pong:
    case MessageType::RttReport:
    case MessageType::HashReport:
    case MessageType::CreateLobby:
    case MessageType::JoinLobby:
    case MessageType::LeaveLobby:
    case MessageType::SetReady:
    case MessageType::StartMatch:
    case MessageType::LobbySnapshot:
    case MessageType::Left:
    case MessageType::Enqueue:
    case MessageType::QueueStatus:
    case MessageType::SubmitPlan:
    case MessageType::MatchResult:
    case MessageType::SubmitInput:
    case MessageType::TickConfirm:
    case MessageType::StateSnapshot:
    case MessageType::Reject:
        message.type = static_cast<MessageType>(type);
        return true;
    }
    return false;
}

std::vector<uint8_t> EncodeCreateLobby(const CreateLobbyRequest& request)
{
    ByteWriter writer;
    WriteGame(writer, request.game);
    WriteMode(writer, request.mode);
    writer.U32(request.maxPlayers);
    writer.U64(request.seed);
    return FinishWrite(writer);
}

bool DecodeCreateLobby(const std::vector<uint8_t>& bytes, CreateLobbyRequest& request)
{
    ByteReader reader(bytes);
    if (!ReadGame(reader, request.game) || !ReadMode(reader, request.mode))
    {
        return false;
    }
    request.maxPlayers = reader.U32();
    request.seed = reader.U64();
    return reader.Finish();
}

std::vector<uint8_t> EncodeJoinLobby(const JoinLobbyRequest& request)
{
    ByteWriter writer;
    if (!writer.Str(request.joinCode, kMaxJoinCodeLength))
    {
        return {};
    }
    return FinishWrite(writer);
}

bool DecodeJoinLobby(const std::vector<uint8_t>& bytes, JoinLobbyRequest& request)
{
    ByteReader reader(bytes);
    request.joinCode = reader.Str(kMaxJoinCodeLength);
    return reader.Finish();
}

std::vector<uint8_t> EncodeSetReady(bool ready)
{
    ByteWriter writer;
    writer.U8(ready ? 1u : 0u);
    return FinishWrite(writer);
}

bool DecodeSetReady(const std::vector<uint8_t>& bytes, bool& ready)
{
    ByteReader reader(bytes);
    const uint8_t value = reader.U8();
    if (value > 1)
    {
        return false;
    }
    ready = value == 1;
    return reader.Finish();
}

std::vector<uint8_t> EncodeEnqueue(const EnqueueRequest& request)
{
    ByteWriter writer;
    WriteGame(writer, request.game);
    WriteMode(writer, request.mode);
    writer.U32(request.playersNeeded);
    return FinishWrite(writer);
}

bool DecodeEnqueue(const std::vector<uint8_t>& bytes, EnqueueRequest& request)
{
    ByteReader reader(bytes);
    if (!ReadGame(reader, request.game) || !ReadMode(reader, request.mode))
    {
        return false;
    }
    request.playersNeeded = reader.U32();
    return reader.Finish();
}

std::vector<uint8_t> EncodeLobbyView(const LobbyView& lobby)
{
    ByteWriter writer;
    writer.U32(lobby.lobbyId);
    if (!writer.Str(lobby.joinCode, kMaxJoinCodeLength))
    {
        return {};
    }
    WriteGame(writer, lobby.game);
    WriteMode(writer, lobby.mode);
    writer.U32(lobby.maxPlayers);
    writer.U64(lobby.seed);
    writer.U8(lobby.started ? 1u : 0u);
    writer.U32(lobby.inputDelayTicks);
    writer.U32(lobby.interpolationDelayMs);
    if (!writer.Str(lobby.notice, kMaxNoticeLength))
    {
        return {};
    }
    if (lobby.players.size() > kMaxLobbyPlayers)
    {
        return {};
    }
    writer.U8(static_cast<uint8_t>(lobby.players.size()));
    for (const LobbyPlayer& player : lobby.players)
    {
        writer.U32(player.playerId);
        if (!writer.Str(player.name, kMaxPlayerNameLength))
        {
            return {};
        }
        writer.U8(player.ready ? 1u : 0u);
        writer.U8(player.host ? 1u : 0u);
    }
    return FinishWrite(writer);
}

bool DecodeLobbyView(const std::vector<uint8_t>& bytes, LobbyView& lobby)
{
    ByteReader reader(bytes);
    lobby.lobbyId = reader.U32();
    lobby.joinCode = reader.Str(kMaxJoinCodeLength);
    if (!ReadGame(reader, lobby.game) || !ReadMode(reader, lobby.mode))
    {
        return false;
    }
    lobby.maxPlayers = reader.U32();
    lobby.seed = reader.U64();
    const uint8_t started = reader.U8();
    lobby.inputDelayTicks = reader.U32();
    lobby.interpolationDelayMs = reader.U32();
    lobby.notice = reader.Str(kMaxNoticeLength);
    const uint8_t count = reader.U8();
    if (started > 1 || count > kMaxLobbyPlayers)
    {
        return false;
    }
    lobby.started = started == 1;
    lobby.players.clear();
    for (uint8_t i = 0; i < count; ++i)
    {
        LobbyPlayer player;
        player.playerId = reader.U32();
        player.name = reader.Str(kMaxPlayerNameLength);
        const uint8_t ready = reader.U8();
        const uint8_t host = reader.U8();
        if (ready > 1 || host > 1)
        {
            return false;
        }
        player.ready = ready == 1;
        player.host = host == 1;
        lobby.players.push_back(player);
    }
    return reader.Finish();
}

std::vector<uint8_t> EncodeQueueStatus(const QueueStatusMessage& status)
{
    ByteWriter writer;
    WriteGame(writer, status.game);
    WriteMode(writer, status.mode);
    writer.U32(status.playersNeeded);
    writer.U32(status.ahead);
    return FinishWrite(writer);
}

bool DecodeQueueStatus(const std::vector<uint8_t>& bytes, QueueStatusMessage& status)
{
    ByteReader reader(bytes);
    if (!ReadGame(reader, status.game) || !ReadMode(reader, status.mode))
    {
        return false;
    }
    status.playersNeeded = reader.U32();
    status.ahead = reader.U32();
    return reader.Finish();
}

std::vector<uint8_t> EncodeDecisionPlan(const DecisionPlan& plan)
{
    if (plan.commands.size() > kMaxPlanCommands)
    {
        return {};
    }
    ByteWriter writer;
    writer.U32(plan.playerId);
    writer.U16(static_cast<uint16_t>(plan.commands.size()));
    for (const PlayerCommand& command : plan.commands)
    {
        WriteCommand(writer, command);
    }
    return FinishWrite(writer);
}

bool DecodeDecisionPlan(const std::vector<uint8_t>& bytes, DecisionPlan& plan)
{
    ByteReader reader(bytes);
    plan.playerId = reader.U32();
    const uint16_t count = reader.U16();
    if (count > kMaxPlanCommands)
    {
        return false;
    }
    plan.commands.clear();
    plan.commands.reserve(count);
    for (uint16_t i = 0; i < count; ++i)
    {
        PlayerCommand command;
        ReadCommand(reader, command);
        plan.commands.push_back(command);
    }
    return reader.Finish();
}

std::vector<uint8_t> EncodeMatchOutcome(const MatchOutcome& outcome)
{
    if (outcome.scores.size() > kMaxLobbyPlayers)
    {
        return {};
    }
    ByteWriter writer;
    writer.U8(outcome.complete ? 1u : 0u);
    writer.U8(outcome.forfeit ? 1u : 0u);
    writer.I32(outcome.winnerPlayerId);
    writer.U64(outcome.stateHash);
    writer.U32(outcome.ticksSimulated);
    writer.U8(static_cast<uint8_t>(outcome.scores.size()));
    for (int32_t score : outcome.scores)
    {
        writer.I32(score);
    }
    return FinishWrite(writer);
}

bool DecodeMatchOutcome(const std::vector<uint8_t>& bytes, MatchOutcome& outcome)
{
    ByteReader reader(bytes);
    const uint8_t complete = reader.U8();
    const uint8_t forfeit = reader.U8();
    outcome.winnerPlayerId = reader.I32();
    outcome.stateHash = reader.U64();
    outcome.ticksSimulated = reader.U32();
    const uint8_t count = reader.U8();
    if (complete > 1 || forfeit > 1 || count > kMaxLobbyPlayers)
    {
        return false;
    }
    outcome.complete = complete == 1;
    outcome.forfeit = forfeit == 1;
    outcome.scores.clear();
    for (uint8_t i = 0; i < count; ++i)
    {
        outcome.scores.push_back(reader.I32());
    }
    return reader.Finish();
}

std::vector<uint8_t> EncodePlayerCommand(const PlayerCommand& command)
{
    ByteWriter writer;
    WriteCommand(writer, command);
    return FinishWrite(writer);
}

bool DecodePlayerCommand(const std::vector<uint8_t>& bytes, PlayerCommand& command)
{
    ByteReader reader(bytes);
    ReadCommand(reader, command);
    return reader.Finish();
}

std::vector<uint8_t> EncodeTickConfirm(const TickConfirmMessage& confirm)
{
    ByteWriter writer;
    writer.U32(confirm.tick);
    writer.U64(confirm.stateHash);
    writer.U8(confirm.corrected ? 1u : 0u);
    writer.U32(confirm.inputDelayTicks);
    writer.U32(confirm.interpolationDelayMs);
    if (!writer.Blob(confirm.snapshot, kMaxSnapshotBytes))
    {
        return {};
    }
    return FinishWrite(writer);
}

bool DecodeTickConfirm(const std::vector<uint8_t>& bytes, TickConfirmMessage& confirm)
{
    ByteReader reader(bytes);
    confirm.tick = reader.U32();
    confirm.stateHash = reader.U64();
    const uint8_t corrected = reader.U8();
    confirm.inputDelayTicks = reader.U32();
    confirm.interpolationDelayMs = reader.U32();
    confirm.snapshot = reader.Blob(kMaxSnapshotBytes);
    if (corrected > 1 || !reader.Finish())
    {
        return false;
    }
    confirm.corrected = corrected == 1;
    return true;
}

std::vector<uint8_t> EncodeStateSnapshot(const StateSnapshotMessage& snapshot)
{
    ByteWriter writer;
    writer.U32(snapshot.tick);
    writer.U64(snapshot.stateHash);
    if (!writer.Blob(snapshot.snapshot, kMaxSnapshotBytes))
    {
        return {};
    }
    return FinishWrite(writer);
}

bool DecodeStateSnapshot(const std::vector<uint8_t>& bytes, StateSnapshotMessage& snapshot)
{
    ByteReader reader(bytes);
    snapshot.tick = reader.U32();
    snapshot.stateHash = reader.U64();
    snapshot.snapshot = reader.Blob(kMaxSnapshotBytes);
    return reader.Finish();
}

std::vector<uint8_t> EncodeTimestamp(const TimestampMessage& stamp)
{
    ByteWriter writer;
    writer.U32(stamp.clientTimeMs);
    return FinishWrite(writer);
}

bool DecodeTimestamp(const std::vector<uint8_t>& bytes, TimestampMessage& stamp)
{
    ByteReader reader(bytes);
    stamp.clientTimeMs = reader.U32();
    return reader.Finish();
}

std::vector<uint8_t> EncodeHashReport(const HashReportMessage& report)
{
    ByteWriter writer;
    writer.U32(report.tick);
    writer.U64(report.stateHash);
    return FinishWrite(writer);
}

bool DecodeHashReport(const std::vector<uint8_t>& bytes, HashReportMessage& report)
{
    ByteReader reader(bytes);
    report.tick = reader.U32();
    report.stateHash = reader.U64();
    return reader.Finish();
}

std::vector<uint8_t> EncodeReject(const std::string& reason)
{
    ByteWriter writer;
    if (!writer.Str(reason, kMaxNoticeLength))
    {
        return {};
    }
    return FinishWrite(writer);
}

bool DecodeReject(const std::vector<uint8_t>& bytes, std::string& reason)
{
    ByteReader reader(bytes);
    reason = reader.Str(kMaxNoticeLength);
    return reader.Finish();
}

}
