#pragma once

#include "Latency.h"
#include "OnlineMatch.h"

#include <map>
#include <memory>
#include <vector>

namespace Session
{

// Authoritative lobby, matchmaking, offline resolution, and online ticks.
// Call AdvanceOnlineMatches on the server clock (default 30 Hz). Clients do
// not advance time; a client-driven clock would let one peer speed the match.
class SessionServer
{
public:
    explicit SessionServer(std::unique_ptr<IMatchFactory> rules);
    SessionServer(const SessionServer&) = delete;
    SessionServer& operator=(const SessionServer&) = delete;

    void SetLatencyProfile(const LatencyProfile& profile);
    const LatencyProfile& GetLatencyProfile() const { return profile_; }

    uint32_t Connect(const std::string& name);
    void Disconnect(uint32_t playerId);
    void Receive(uint32_t playerId, const Envelope& message);
    std::vector<Envelope> TakeOutbound();
    void AdvanceOnlineMatches();

    uint32_t LobbyCount() const { return static_cast<uint32_t>(lobbies_.size()); }
    uint32_t QueueSize() const { return static_cast<uint32_t>(queue_.size()); }
    const MatchSetup* SetupForPlayer(uint32_t playerId) const;

private:
    struct OfflineRuntime
    {
        std::unique_ptr<IDeterministicMatch> sim;
        uint64_t seed = 0;
        std::vector<uint32_t> players;
        std::map<uint32_t, DecisionPlan> plans;
    };

    struct LobbyRecord
    {
        LobbyView view;
        std::vector<uint32_t> departed;
        std::vector<uint32_t> simPlayers;
        std::unique_ptr<OfflineRuntime> offline;
        std::unique_ptr<OnlineMatch> online;
        bool resultSent = false;
        bool hasSetup = false;
        MatchSetup setup;
    };

    struct ServerPlayer
    {
        uint32_t id = 0;
        std::string name;
        bool connected = false;
        uint32_t lobbyId = 0;
        bool queued = false;
        uint32_t rttMs = 0;
    };

    struct QueueTicket
    {
        uint32_t playerId = 0;
        GameTitle game = GameTitle::DynamicGo;
        PlayMode mode = PlayMode::OfflineResolution;
        uint32_t playersNeeded = 2;
    };

    void Send(uint32_t playerId, MessageType type, const std::vector<uint8_t>& payload);
    void Reject(uint32_t playerId, const std::string& reason);
    void BroadcastLobby(LobbyRecord& lobby);
    void SendLeft(uint32_t playerId);
    void SendResult(const LobbyRecord& lobby, const MatchOutcome& outcome);
    ServerPlayer* FindPlayer(uint32_t playerId);
    LobbyRecord* FindLobby(uint32_t lobbyId);
    LobbyRecord* LobbyOf(uint32_t playerId);
    bool IsDeparted(const LobbyRecord& lobby, uint32_t playerId) const;
    bool StartLobby(LobbyRecord& lobby);
    void HandleLeave(uint32_t playerId);
    void HandleEnqueue(uint32_t playerId, const EnqueueRequest& request);
    void HandlePlan(uint32_t playerId, DecisionPlan plan);
    void HandleInput(uint32_t playerId, PlayerCommand command);
    void HandleHash(uint32_t playerId, const HashReportMessage& report);
    void ResolveOfflineLobby(LobbyRecord& lobby);
    void ForfeitOffline(LobbyRecord& lobby);
    void RemoveFromView(LobbyRecord& lobby, uint32_t playerId);
    static std::string MakeJoinCode(uint32_t lobbyId);
    static uint64_t MatchSeed(
        GameTitle game,
        PlayMode mode,
        uint32_t lobbyId,
        const std::vector<uint32_t>& players);

    std::unique_ptr<IMatchFactory> rules_;
    LatencyProfile profile_{};
    uint32_t nextPlayerId_ = 1;
    uint32_t nextLobbyId_ = 1;
    std::map<uint32_t, ServerPlayer> players_;
    std::map<uint32_t, LobbyRecord> lobbies_;
    std::vector<QueueTicket> queue_;
    std::vector<Envelope> outbound_;
};

}
