#pragma once

#include "SessionTypes.h"

#include <vector>

namespace Session
{

// One connected player. Queue methods, then let the host move envelopes.
// Lobby() and Result() point at this object and change on the next Receive.
class SessionClient
{
public:
    SessionClient() = default;
    SessionClient(const SessionClient&) = delete;
    SessionClient& operator=(const SessionClient&) = delete;

    void Bind(uint32_t playerId, const std::string& name);
    uint32_t PlayerId() const { return playerId_; }
    const std::string& Name() const { return name_; }

    void CreateLobby(GameTitle game, PlayMode mode, uint32_t maxPlayers, uint64_t seed);
    void JoinLobby(const std::string& joinCode);
    void SetReady(bool ready);
    void StartMatch();
    void Leave();
    void Enqueue(GameTitle game, PlayMode mode, uint32_t playersNeeded = 2);
    void SubmitPlan(const DecisionPlan& plan);
    void SubmitInput(const PlayerCommand& command);
    void SendPing(uint32_t clientTimeMs);
    void CompletePing(uint32_t nowMs);
    void ReportLocalHash(uint32_t tick, uint64_t hash);
    void MarkDisconnected();

    void Receive(const Envelope& message);
    std::vector<Envelope> TakeOutbound();

    const LobbyView* Lobby() const;
    const MatchOutcome* Result() const;
    bool WaitingInQueue() const { return waitingInQueue_; }
    uint32_t QueueAhead() const { return queueAhead_; }
    const std::string& LastReject() const { return lastReject_; }
    bool HasConfirmedTick() const { return hasConfirm_; }
    uint32_t LastConfirmedTick() const { return lastTick_; }
    uint64_t ConfirmedHash() const { return confirmedHash_; }
    uint32_t InputDelayTicks() const { return inputDelayTicks_; }
    uint32_t RoundTripMs() const { return rttMs_; }
    uint32_t InterpolationDelayMs() const { return interpolationDelayMs_; }
    bool HashMismatch() const { return hashMismatch_; }
    bool HasAuthoritativeSnapshot() const { return hasResync_; }
    const std::vector<uint8_t>* AuthoritativeSnapshot() const;
    uint32_t ResyncTick() const { return resyncTick_; }

private:
    void Queue(MessageType type, const std::vector<uint8_t>& payload);
    void StoreSnapshot(uint32_t tick, uint64_t hash, const std::vector<uint8_t>& snapshot, bool overwriteConfirm);

    uint32_t playerId_ = 0;
    std::string name_;
    std::vector<Envelope> outbound_;
    bool hasLobby_ = false;
    LobbyView lobby_{};
    bool hasResult_ = false;
    MatchOutcome result_{};
    bool waitingInQueue_ = false;
    uint32_t queueAhead_ = 0;
    std::string lastReject_;
    bool hasConfirm_ = false;
    uint32_t lastTick_ = 0;
    uint64_t confirmedHash_ = 0;
    uint32_t inputDelayTicks_ = 0;
    uint32_t rttMs_ = 0;
    uint32_t interpolationDelayMs_ = 0;
    bool pingOutstanding_ = false;
    uint32_t pingTimeMs_ = 0;
    bool pongWaiting_ = false;
    bool hashMismatch_ = false;
    bool hasResync_ = false;
    uint32_t resyncTick_ = 0;
    std::vector<uint8_t> resync_;
};

}
