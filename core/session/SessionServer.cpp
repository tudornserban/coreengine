#include "SessionServer.h"

#include "OfflineResolver.h"
#include "SessionCodec.h"

#include <algorithm>

namespace Session
{
namespace
{

uint64_t MixByte(uint64_t hash, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
    {
        hash ^= (value >> shift) & 0xffu;
        hash *= 1099511628211ull;
    }
    return hash;
}

uint32_t ClampPlayers(uint32_t count)
{
    if (count < 2)
    {
        return 2;
    }
    if (count > kMaxLobbyPlayers)
    {
        return kMaxLobbyPlayers;
    }
    return count;
}

}

SessionServer::SessionServer(std::unique_ptr<IMatchFactory> rules)
    : rules_(std::move(rules))
{
}

void SessionServer::SetLatencyProfile(const LatencyProfile& profile)
{
    profile_ = profile;
}

uint32_t SessionServer::Connect(const std::string& name)
{
    if (name.empty() || name.size() > kMaxPlayerNameLength)
    {
        return 0;
    }
    ServerPlayer player;
    player.id = nextPlayerId_++;
    player.name = name;
    player.connected = true;
    const uint32_t id = player.id;
    players_.emplace(id, std::move(player));
    return id;
}

void SessionServer::Disconnect(uint32_t playerId)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr || !player->connected)
    {
        return;
    }
    player->connected = false;
    HandleLeave(playerId);
    players_.erase(playerId);
}

void SessionServer::Receive(uint32_t playerId, const Envelope& message)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr || !player->connected)
    {
        return;
    }

    switch (message.type)
    {
    case MessageType::Ping:
    {
        TimestampMessage stamp;
        if (!DecodeTimestamp(message.payload, stamp))
        {
            Reject(playerId, "bad-message");
            return;
        }
        Send(playerId, MessageType::Pong, EncodeTimestamp(stamp));
        return;
    }
    case MessageType::RttReport:
    {
        TimestampMessage stamp;
        if (!DecodeTimestamp(message.payload, stamp))
        {
            Reject(playerId, "bad-message");
            return;
        }
        player->rttMs = stamp.clientTimeMs;
        if (LobbyRecord* lobby = LobbyOf(playerId))
        {
            if (lobby->online)
            {
                lobby->online->NoteRtt(playerId, player->rttMs);
            }
        }
        return;
    }
    case MessageType::HashReport:
    {
        HashReportMessage report;
        if (!DecodeHashReport(message.payload, report))
        {
            Reject(playerId, "bad-message");
            return;
        }
        HandleHash(playerId, report);
        return;
    }
    case MessageType::CreateLobby:
    {
        CreateLobbyRequest request;
        if (!DecodeCreateLobby(message.payload, request))
        {
            Reject(playerId, "bad-message");
            return;
        }
        if (player->lobbyId != 0)
        {
            Reject(playerId, "already-in-lobby");
            return;
        }
        if (player->queued)
        {
            Reject(playerId, "already-queued");
            return;
        }
        const uint32_t lobbyId = nextLobbyId_++;
        LobbyRecord& lobby = lobbies_[lobbyId];
        lobby.view.lobbyId = lobbyId;
        lobby.view.joinCode = MakeJoinCode(lobbyId);
        lobby.view.game = request.game;
        lobby.view.mode = request.mode;
        lobby.view.maxPlayers = ClampPlayers(request.maxPlayers);
        lobby.view.seed = request.seed;
        lobby.view.notice = "created";
        LobbyPlayer host;
        host.playerId = player->id;
        host.name = player->name;
        host.host = true;
        lobby.view.players.push_back(host);
        player->lobbyId = lobbyId;
        BroadcastLobby(lobby);
        return;
    }
    case MessageType::JoinLobby:
    {
        JoinLobbyRequest request;
        if (!DecodeJoinLobby(message.payload, request))
        {
            Reject(playerId, "bad-message");
            return;
        }
        if (player->lobbyId != 0)
        {
            Reject(playerId, "already-in-lobby");
            return;
        }
        if (player->queued)
        {
            Reject(playerId, "already-queued");
            return;
        }
        LobbyRecord* found = nullptr;
        for (auto& entry : lobbies_)
        {
            if (entry.second.view.joinCode == request.joinCode)
            {
                found = &entry.second;
                break;
            }
        }
        if (found == nullptr)
        {
            Reject(playerId, "lobby-not-found");
            return;
        }
        if (found->view.started)
        {
            Reject(playerId, "match-started");
            return;
        }
        if (found->view.players.size() >= found->view.maxPlayers)
        {
            Reject(playerId, "lobby-full");
            return;
        }
        LobbyPlayer member;
        member.playerId = player->id;
        member.name = player->name;
        found->view.players.push_back(member);
        found->view.notice = "joined";
        player->lobbyId = found->view.lobbyId;
        BroadcastLobby(*found);
        return;
    }
    case MessageType::LeaveLobby:
        HandleLeave(playerId);
        return;
    case MessageType::SetReady:
    {
        bool ready = false;
        if (!DecodeSetReady(message.payload, ready))
        {
            Reject(playerId, "bad-message");
            return;
        }
        LobbyRecord* lobby = LobbyOf(playerId);
        if (lobby == nullptr)
        {
            Reject(playerId, "not-in-lobby");
            return;
        }
        if (lobby->view.started)
        {
            Reject(playerId, "match-started");
            return;
        }
        for (LobbyPlayer& member : lobby->view.players)
        {
            if (member.playerId == playerId)
            {
                member.ready = ready;
            }
        }
        lobby->view.notice = "ready";
        BroadcastLobby(*lobby);
        return;
    }
    case MessageType::StartMatch:
    {
        LobbyRecord* lobby = LobbyOf(playerId);
        if (lobby == nullptr)
        {
            Reject(playerId, "not-in-lobby");
            return;
        }
        if (lobby->view.started)
        {
            Reject(playerId, "match-started");
            return;
        }
        bool host = false;
        bool allReady = !lobby->view.players.empty();
        for (const LobbyPlayer& member : lobby->view.players)
        {
            if (member.playerId == playerId && member.host)
            {
                host = true;
            }
            if (!member.ready)
            {
                allReady = false;
            }
        }
        if (!host)
        {
            Reject(playerId, "not-host");
            return;
        }
        if (lobby->view.players.size() < 2)
        {
            Reject(playerId, "need-players");
            return;
        }
        if (!allReady)
        {
            Reject(playerId, "not-ready");
            return;
        }
        if (!StartLobby(*lobby))
        {
            Reject(playerId, "no-rules");
        }
        return;
    }
    case MessageType::Enqueue:
    {
        EnqueueRequest request;
        if (!DecodeEnqueue(message.payload, request))
        {
            Reject(playerId, "bad-message");
            return;
        }
        HandleEnqueue(playerId, request);
        return;
    }
    case MessageType::SubmitPlan:
    {
        DecisionPlan plan;
        if (!DecodeDecisionPlan(message.payload, plan))
        {
            Reject(playerId, "bad-message");
            return;
        }
        HandlePlan(playerId, std::move(plan));
        return;
    }
    case MessageType::SubmitInput:
    {
        PlayerCommand command;
        if (!DecodePlayerCommand(message.payload, command))
        {
            Reject(playerId, "bad-message");
            return;
        }
        HandleInput(playerId, command);
        return;
    }
    case MessageType::Pong:
    case MessageType::LobbySnapshot:
    case MessageType::Left:
    case MessageType::QueueStatus:
    case MessageType::MatchResult:
    case MessageType::TickConfirm:
    case MessageType::StateSnapshot:
    case MessageType::Reject:
        return;
    }
}

std::vector<Envelope> SessionServer::TakeOutbound()
{
    std::vector<Envelope> out;
    out.swap(outbound_);
    return out;
}

void SessionServer::AdvanceOnlineMatches()
{
    for (auto& entry : lobbies_)
    {
        LobbyRecord& lobby = entry.second;
        if (!lobby.online || lobby.resultSent)
        {
            continue;
        }
        const AdvanceResult advanced = lobby.online->Advance();
        if (!advanced.stepped)
        {
            continue;
        }
        const std::vector<uint8_t> bytes = EncodeTickConfirm(advanced.confirm);
        for (const LobbyPlayer& member : lobby.view.players)
        {
            Send(member.playerId, MessageType::TickConfirm, bytes);
        }
        if (advanced.matchComplete)
        {
            lobby.resultSent = true;
            lobby.view.notice = "resolved";
            SendResult(lobby, advanced.outcome);
        }
    }
}

void SessionServer::Send(uint32_t playerId, MessageType type, const std::vector<uint8_t>& payload)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr || !player->connected || payload.empty())
    {
        return;
    }
    Envelope message;
    message.type = type;
    message.playerId = playerId;
    message.payload = payload;
    outbound_.push_back(std::move(message));
}

void SessionServer::Reject(uint32_t playerId, const std::string& reason)
{
    Send(playerId, MessageType::Reject, EncodeReject(reason));
}

void SessionServer::BroadcastLobby(LobbyRecord& lobby)
{
    if (lobby.view.notice.size() > kMaxNoticeLength)
    {
        lobby.view.notice.resize(kMaxNoticeLength);
    }
    const std::vector<uint8_t> bytes = EncodeLobbyView(lobby.view);
    for (const LobbyPlayer& member : lobby.view.players)
    {
        Send(member.playerId, MessageType::LobbySnapshot, bytes);
    }
}

void SessionServer::SendLeft(uint32_t playerId)
{
    Send(playerId, MessageType::Left, EncodeReject("left"));
}

void SessionServer::SendResult(const LobbyRecord& lobby, const MatchOutcome& outcome)
{
    const std::vector<uint8_t> bytes = EncodeMatchOutcome(outcome);
    for (uint32_t id : lobby.simPlayers)
    {
        Send(id, MessageType::MatchResult, bytes);
    }
}

SessionServer::ServerPlayer* SessionServer::FindPlayer(uint32_t playerId)
{
    const auto it = players_.find(playerId);
    if (it == players_.end())
    {
        return nullptr;
    }
    return &it->second;
}

SessionServer::LobbyRecord* SessionServer::FindLobby(uint32_t lobbyId)
{
    const auto it = lobbies_.find(lobbyId);
    if (it == lobbies_.end())
    {
        return nullptr;
    }
    return &it->second;
}

SessionServer::LobbyRecord* SessionServer::LobbyOf(uint32_t playerId)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr || player->lobbyId == 0)
    {
        return nullptr;
    }
    return FindLobby(player->lobbyId);
}

bool SessionServer::IsDeparted(const LobbyRecord& lobby, uint32_t playerId) const
{
    return std::find(lobby.departed.begin(), lobby.departed.end(), playerId) != lobby.departed.end();
}

bool SessionServer::StartLobby(LobbyRecord& lobby)
{
    if (lobby.view.started)
    {
        return false;
    }
    std::unique_ptr<IDeterministicMatch> sim = rules_ ? rules_->Create(lobby.view.game) : nullptr;
    if (!sim)
    {
        return false;
    }

    std::vector<uint32_t> ids;
    ids.reserve(lobby.view.players.size());
    for (const LobbyPlayer& member : lobby.view.players)
    {
        ids.push_back(member.playerId);
    }
    std::sort(ids.begin(), ids.end());
    lobby.simPlayers = ids;
    lobby.view.started = true;
    lobby.view.notice = lobby.view.notice == "matched" ? "matched" : "started";

    if (lobby.view.mode == PlayMode::OfflineResolution)
    {
        auto runtime = std::make_unique<OfflineRuntime>();
        runtime->sim = std::move(sim);
        runtime->seed = lobby.view.seed;
        runtime->players = ids;
        lobby.offline = std::move(runtime);
        lobby.view.inputDelayTicks = 0;
        lobby.view.interpolationDelayMs = 0;
    }
    else
    {
        lobby.online = std::make_unique<OnlineMatch>(std::move(sim), lobby.view.seed, ids, profile_);
        for (uint32_t id : ids)
        {
            if (ServerPlayer* member = FindPlayer(id))
            {
                lobby.online->NoteRtt(id, member->rttMs);
            }
        }
        lobby.view.inputDelayTicks = lobby.online->InputDelayTicks();
        lobby.view.interpolationDelayMs = lobby.online->InterpolationDelayMs();
    }

    BroadcastLobby(lobby);
    if (lobby.online && lobby.online->Finished() && !lobby.resultSent)
    {
        const AdvanceResult advanced = lobby.online->Advance();
        lobby.resultSent = true;
        SendResult(lobby, advanced.outcome);
    }
    return true;
}

void SessionServer::HandleLeave(uint32_t playerId)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr)
    {
        return;
    }
    if (player->queued)
    {
        queue_.erase(
            std::remove_if(
                queue_.begin(),
                queue_.end(),
                [playerId](const QueueTicket& ticket) { return ticket.playerId == playerId; }),
            queue_.end());
        player->queued = false;
    }

    LobbyRecord* lobby = LobbyOf(playerId);
    if (lobby == nullptr)
    {
        SendLeft(playerId);
        return;
    }

    if (!lobby->view.started)
    {
        RemoveFromView(*lobby, playerId);
        player->lobbyId = 0;
        if (lobby->view.players.empty())
        {
            lobbies_.erase(lobby->view.lobbyId);
        }
        else
        {
            bool anyHost = false;
            for (const LobbyPlayer& member : lobby->view.players)
            {
                if (member.host)
                {
                    anyHost = true;
                }
            }
            if (!anyHost && !lobby->view.players.empty())
            {
                lobby->view.players.front().host = true;
            }
            lobby->view.notice = "player-left";
            BroadcastLobby(*lobby);
        }
        SendLeft(playerId);
        return;
    }

    if (!IsDeparted(*lobby, playerId))
    {
        lobby->departed.push_back(playerId);
    }
    RemoveFromView(*lobby, playerId);
    player->lobbyId = 0;
    SendLeft(playerId);

    if (lobby->offline && !lobby->resultSent)
    {
        if (lobby->view.players.size() < 2)
        {
            ForfeitOffline(*lobby);
            return;
        }
        bool allSubmitted = true;
        for (const LobbyPlayer& member : lobby->view.players)
        {
            if (lobby->offline->plans.find(member.playerId) == lobby->offline->plans.end())
            {
                allSubmitted = false;
            }
        }
        if (allSubmitted)
        {
            ResolveOfflineLobby(*lobby);
        }
        else
        {
            lobby->view.notice = "player-left";
            BroadcastLobby(*lobby);
        }
        return;
    }

    if (lobby->online)
    {
        lobby->view.notice = "player-left";
        BroadcastLobby(*lobby);
    }
}

void SessionServer::HandleEnqueue(uint32_t playerId, const EnqueueRequest& request)
{
    ServerPlayer* player = FindPlayer(playerId);
    if (player == nullptr)
    {
        return;
    }
    if (player->lobbyId != 0)
    {
        Reject(playerId, "already-in-lobby");
        return;
    }
    if (player->queued)
    {
        Reject(playerId, "already-queued");
        return;
    }

    QueueTicket ticket;
    ticket.playerId = playerId;
    ticket.game = request.game;
    ticket.mode = request.mode;
    ticket.playersNeeded = ClampPlayers(request.playersNeeded);
    queue_.push_back(ticket);
    player->queued = true;

    std::vector<size_t> matchIdx;
    for (size_t i = 0; i < queue_.size(); ++i)
    {
        const QueueTicket& queued = queue_[i];
        if (queued.game == ticket.game && queued.mode == ticket.mode && queued.playersNeeded == ticket.playersNeeded)
        {
            matchIdx.push_back(i);
        }
    }
    if (matchIdx.size() < ticket.playersNeeded)
    {
        QueueStatusMessage status;
        status.game = ticket.game;
        status.mode = ticket.mode;
        status.playersNeeded = ticket.playersNeeded;
        status.ahead = static_cast<uint32_t>(matchIdx.size() - 1);
        Send(playerId, MessageType::QueueStatus, EncodeQueueStatus(status));
        return;
    }

    std::vector<QueueTicket> matched;
    matched.reserve(ticket.playersNeeded);
    for (uint32_t n = 0; n < ticket.playersNeeded; ++n)
    {
        matched.push_back(queue_[matchIdx[n]]);
    }
    for (uint32_t n = ticket.playersNeeded; n-- > 0;)
    {
        queue_.erase(queue_.begin() + static_cast<std::ptrdiff_t>(matchIdx[n]));
    }

    std::vector<uint32_t> ids;
    ids.reserve(matched.size());
    for (const QueueTicket& entry : matched)
    {
        ids.push_back(entry.playerId);
    }
    const uint32_t lobbyId = nextLobbyId_++;
    LobbyRecord& lobby = lobbies_[lobbyId];
    lobby.view.lobbyId = lobbyId;
    lobby.view.joinCode = MakeJoinCode(lobbyId);
    lobby.view.game = ticket.game;
    lobby.view.mode = ticket.mode;
    lobby.view.maxPlayers = ticket.playersNeeded;
    lobby.view.notice = "matched";
    std::vector<uint32_t> sorted = ids;
    std::sort(sorted.begin(), sorted.end());
    lobby.view.seed = MatchSeed(ticket.game, ticket.mode, lobbyId, sorted);

    for (size_t i = 0; i < matched.size(); ++i)
    {
        ServerPlayer* member = FindPlayer(matched[i].playerId);
        if (member == nullptr)
        {
            continue;
        }
        member->queued = false;
        member->lobbyId = lobbyId;
        LobbyPlayer view;
        view.playerId = member->id;
        view.name = member->name;
        view.ready = true;
        view.host = i == 0;
        lobby.view.players.push_back(view);
    }

    if (lobby.view.players.size() < ticket.playersNeeded || !StartLobby(lobby))
    {
        for (uint32_t id : ids)
        {
            if (ServerPlayer* member = FindPlayer(id))
            {
                member->lobbyId = 0;
                member->queued = false;
            }
            Reject(id, "no-rules");
        }
        lobbies_.erase(lobbyId);
    }
}

void SessionServer::HandlePlan(uint32_t playerId, DecisionPlan plan)
{
    LobbyRecord* lobby = LobbyOf(playerId);
    if (lobby == nullptr || !lobby->offline || !lobby->view.started || IsDeparted(*lobby, playerId))
    {
        Reject(playerId, "not-in-match");
        return;
    }
    if (lobby->offline->plans.find(playerId) != lobby->offline->plans.end())
    {
        Reject(playerId, "plan-already-submitted");
        return;
    }
    if (plan.commands.size() > kMaxPlanCommands)
    {
        Reject(playerId, "bad-message");
        return;
    }
    plan.playerId = playerId;
    for (PlayerCommand& command : plan.commands)
    {
        command.playerId = playerId;
    }
    lobby->offline->plans.emplace(playerId, std::move(plan));

    if (lobby->view.players.size() < 2)
    {
        return;
    }
    for (const LobbyPlayer& member : lobby->view.players)
    {
        if (lobby->offline->plans.find(member.playerId) == lobby->offline->plans.end())
        {
            return;
        }
    }
    ResolveOfflineLobby(*lobby);
}

void SessionServer::HandleInput(uint32_t playerId, PlayerCommand command)
{
    LobbyRecord* lobby = LobbyOf(playerId);
    if (lobby == nullptr || !lobby->online || IsDeparted(*lobby, playerId))
    {
        Reject(playerId, "not-in-match");
        return;
    }
    command.playerId = playerId;
    const InputAdmitResult admitted = lobby->online->Submit(command);
    if (admitted.admit == InputAdmit::Stored)
    {
        return;
    }
    if (admitted.admit == InputAdmit::RolledBack)
    {
        const std::vector<uint8_t> bytes = EncodeTickConfirm(admitted.correction);
        for (const LobbyPlayer& member : lobby->view.players)
        {
            Send(member.playerId, MessageType::TickConfirm, bytes);
        }
        if (admitted.matchComplete && !lobby->resultSent)
        {
            lobby->resultSent = true;
            SendResult(*lobby, admitted.outcome);
        }
        return;
    }
    if (admitted.admit == InputAdmit::DroppedLate)
    {
        Reject(playerId, "input-late");
        return;
    }
    Reject(playerId, "bad-input");
}

void SessionServer::HandleHash(uint32_t playerId, const HashReportMessage& report)
{
    LobbyRecord* lobby = LobbyOf(playerId);
    if (lobby == nullptr || !lobby->online)
    {
        Reject(playerId, "not-in-match");
        return;
    }
    StateSnapshotMessage snapshot;
    if (!lobby->online->CopySnapshotAfter(report.tick, snapshot))
    {
        Reject(playerId, "hash-unknown");
        return;
    }
    if (snapshot.stateHash != report.stateHash)
    {
        Send(playerId, MessageType::StateSnapshot, EncodeStateSnapshot(snapshot));
    }
}

void SessionServer::ResolveOfflineLobby(LobbyRecord& lobby)
{
    if (lobby.resultSent || !lobby.offline)
    {
        return;
    }
    std::vector<DecisionPlan> plans;
    plans.reserve(lobby.offline->plans.size());
    for (const auto& entry : lobby.offline->plans)
    {
        plans.push_back(entry.second);
    }
    const MatchOutcome outcome = ResolveOffline(
        *lobby.offline->sim,
        lobby.offline->seed,
        lobby.offline->players,
        plans);
    lobby.resultSent = true;
    lobby.view.notice = "resolved";
    SendResult(lobby, outcome);
    BroadcastLobby(lobby);
}

void SessionServer::ForfeitOffline(LobbyRecord& lobby)
{
    if (lobby.resultSent || !lobby.offline)
    {
        return;
    }
    MatchOutcome outcome;
    outcome.complete = true;
    outcome.forfeit = true;
    outcome.winnerPlayerId = lobby.view.players.empty()
        ? -1
        : static_cast<int32_t>(lobby.view.players.front().playerId);
    lobby.resultSent = true;
    lobby.view.notice = "forfeit";
    SendResult(lobby, outcome);
}

void SessionServer::RemoveFromView(LobbyRecord& lobby, uint32_t playerId)
{
    auto& members = lobby.view.players;
    members.erase(
        std::remove_if(
            members.begin(),
            members.end(),
            [playerId](const LobbyPlayer& member) { return member.playerId == playerId; }),
        members.end());
}

std::string SessionServer::MakeJoinCode(uint32_t lobbyId)
{
    static const char* alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::string code(6, 'A');
    uint32_t value = lobbyId;
    for (int index = 5; index >= 0; --index)
    {
        code[static_cast<size_t>(index)] = alphabet[value & 31u];
        value >>= 5;
    }
    return code;
}

uint64_t SessionServer::MatchSeed(
    GameTitle game,
    PlayMode mode,
    uint32_t lobbyId,
    const std::vector<uint32_t>& players)
{
    uint64_t hash = 14695981039346656037ull;
    hash = MixByte(hash, static_cast<uint64_t>(game));
    hash = MixByte(hash, static_cast<uint64_t>(mode));
    hash = MixByte(hash, lobbyId);
    for (uint32_t playerId : players)
    {
        hash = MixByte(hash, playerId);
    }
    return hash;
}

}
