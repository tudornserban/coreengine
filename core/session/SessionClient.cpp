#include "SessionClient.h"

#include "Latency.h"
#include "SessionCodec.h"

namespace Session
{

void SessionClient::Bind(uint32_t playerId, const std::string& name)
{
    playerId_ = playerId;
    name_ = name;
}

void SessionClient::Queue(MessageType type, const std::vector<uint8_t>& payload)
{
    if (payload.empty() && type != MessageType::LeaveLobby && type != MessageType::StartMatch && type != MessageType::Left)
    {
        return;
    }
    Envelope message;
    message.type = type;
    message.playerId = playerId_;
    message.payload = payload;
    outbound_.push_back(std::move(message));
}

void SessionClient::CreateLobby(GameTitle game, PlayMode mode, uint32_t maxPlayers, uint64_t seed)
{
    CreateLobbyRequest request;
    request.game = game;
    request.mode = mode;
    request.maxPlayers = maxPlayers;
    request.seed = seed;
    Queue(MessageType::CreateLobby, EncodeCreateLobby(request));
}

void SessionClient::JoinLobby(const std::string& joinCode)
{
    JoinLobbyRequest request;
    request.joinCode = joinCode;
    Queue(MessageType::JoinLobby, EncodeJoinLobby(request));
}

void SessionClient::SetReady(bool ready)
{
    Queue(MessageType::SetReady, EncodeSetReady(ready));
}

void SessionClient::StartMatch()
{
    Queue(MessageType::StartMatch, {});
}

void SessionClient::Leave()
{
    Queue(MessageType::LeaveLobby, {});
}

void SessionClient::Enqueue(GameTitle game, PlayMode mode, uint32_t playersNeeded)
{
    EnqueueRequest request;
    request.game = game;
    request.mode = mode;
    request.playersNeeded = playersNeeded;
    Queue(MessageType::Enqueue, EncodeEnqueue(request));
}

void SessionClient::SubmitPlan(const DecisionPlan& plan)
{
    DecisionPlan stamped = plan;
    stamped.playerId = playerId_;
    for (PlayerCommand& command : stamped.commands)
    {
        command.playerId = playerId_;
    }
    Queue(MessageType::SubmitPlan, EncodeDecisionPlan(stamped));
}

void SessionClient::SubmitInput(const PlayerCommand& command)
{
    PlayerCommand stamped = command;
    stamped.playerId = playerId_;
    Queue(MessageType::SubmitInput, EncodePlayerCommand(stamped));
}

void SessionClient::SendPing(uint32_t clientTimeMs)
{
    pingOutstanding_ = true;
    pingTimeMs_ = clientTimeMs;
    pongWaiting_ = false;
    TimestampMessage stamp;
    stamp.clientTimeMs = clientTimeMs;
    Queue(MessageType::Ping, EncodeTimestamp(stamp));
}

void SessionClient::CompletePing(uint32_t nowMs)
{
    if (!pongWaiting_ || nowMs < pingTimeMs_)
    {
        return;
    }
    pongWaiting_ = false;
    rttMs_ = nowMs - pingTimeMs_;
    interpolationDelayMs_ = Session::InterpolationDelayMs(rttMs_);
    TimestampMessage stamp;
    stamp.clientTimeMs = rttMs_;
    Queue(MessageType::RttReport, EncodeTimestamp(stamp));
}

void SessionClient::MarkDisconnected()
{
    hasLobby_ = false;
    waitingInQueue_ = false;
    pingOutstanding_ = false;
    pongWaiting_ = false;
}

void SessionClient::ReportLocalHash(uint32_t tick, uint64_t hash)
{
    if (hasConfirm_ && tick == lastTick_ && hash != confirmedHash_)
    {
        hashMismatch_ = true;
    }
    HashReportMessage report;
    report.tick = tick;
    report.stateHash = hash;
    Queue(MessageType::HashReport, EncodeHashReport(report));
}

void SessionClient::Receive(const Envelope& message)
{
    switch (message.type)
    {
    case MessageType::Pong:
    {
        TimestampMessage stamp;
        if (!DecodeTimestamp(message.payload, stamp))
        {
            return;
        }
        if (pingOutstanding_ && stamp.clientTimeMs == pingTimeMs_)
        {
            pingOutstanding_ = false;
            pongWaiting_ = true;
        }
        return;
    }
    case MessageType::LobbySnapshot:
    {
        LobbyView view;
        if (!DecodeLobbyView(message.payload, view))
        {
            return;
        }
        lobby_ = std::move(view);
        hasLobby_ = true;
        waitingInQueue_ = false;
        inputDelayTicks_ = lobby_.inputDelayTicks;
        interpolationDelayMs_ = lobby_.interpolationDelayMs;
        return;
    }
    case MessageType::Left:
        hasLobby_ = false;
        waitingInQueue_ = false;
        return;
    case MessageType::QueueStatus:
    {
        QueueStatusMessage status;
        if (!DecodeQueueStatus(message.payload, status))
        {
            return;
        }
        waitingInQueue_ = true;
        queueAhead_ = status.ahead;
        hasLobby_ = false;
        return;
    }
    case MessageType::MatchResult:
    {
        MatchOutcome outcome;
        if (!DecodeMatchOutcome(message.payload, outcome))
        {
            return;
        }
        result_ = std::move(outcome);
        hasResult_ = true;
        return;
    }
    case MessageType::TickConfirm:
    {
        TickConfirmMessage confirm;
        if (!DecodeTickConfirm(message.payload, confirm))
        {
            return;
        }
        hasConfirm_ = true;
        lastTick_ = confirm.tick;
        confirmedHash_ = confirm.stateHash;
        inputDelayTicks_ = confirm.inputDelayTicks;
        interpolationDelayMs_ = confirm.interpolationDelayMs;
        if (!confirm.snapshot.empty())
        {
            StoreSnapshot(confirm.tick, confirm.stateHash, confirm.snapshot, true);
        }
        return;
    }
    case MessageType::StateSnapshot:
    {
        StateSnapshotMessage snapshot;
        if (!DecodeStateSnapshot(message.payload, snapshot))
        {
            return;
        }
        StoreSnapshot(snapshot.tick, snapshot.stateHash, snapshot.snapshot, false);
        if (!hasConfirm_ || snapshot.tick == lastTick_)
        {
            hasConfirm_ = true;
            lastTick_ = snapshot.tick;
            confirmedHash_ = snapshot.stateHash;
        }
        return;
    }
    case MessageType::Reject:
    {
        std::string reason;
        if (!DecodeReject(message.payload, reason))
        {
            return;
        }
        lastReject_ = std::move(reason);
        return;
    }
    case MessageType::Ping:
    case MessageType::RttReport:
    case MessageType::HashReport:
    case MessageType::CreateLobby:
    case MessageType::JoinLobby:
    case MessageType::LeaveLobby:
    case MessageType::SetReady:
    case MessageType::StartMatch:
    case MessageType::Enqueue:
    case MessageType::SubmitPlan:
    case MessageType::SubmitInput:
        return;
    }
}

std::vector<Envelope> SessionClient::TakeOutbound()
{
    std::vector<Envelope> out;
    out.swap(outbound_);
    return out;
}

const LobbyView* SessionClient::Lobby() const
{
    return hasLobby_ ? &lobby_ : nullptr;
}

const MatchOutcome* SessionClient::Result() const
{
    return hasResult_ ? &result_ : nullptr;
}

const std::vector<uint8_t>* SessionClient::AuthoritativeSnapshot() const
{
    return hasResync_ ? &resync_ : nullptr;
}

void SessionClient::StoreSnapshot(
    uint32_t tick,
    uint64_t hash,
    const std::vector<uint8_t>& snapshot,
    bool overwriteConfirm)
{
    resync_ = snapshot;
    resyncTick_ = tick;
    hasResync_ = true;
    hashMismatch_ = false;
    if (overwriteConfirm)
    {
        hasConfirm_ = true;
        lastTick_ = tick;
        confirmedHash_ = hash;
    }
}

}
