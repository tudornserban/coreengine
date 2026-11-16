#pragma once

#include "IDeterministicMatch.h"
#include "Latency.h"

#include <map>
#include <memory>
#include <vector>

namespace Session
{

enum class InputAdmit
{
    Stored,
    RolledBack,
    DroppedLate,
    Rejected
};

struct InputAdmitResult
{
    InputAdmit admit = InputAdmit::Rejected;
    TickConfirmMessage correction;
    bool matchComplete = false;
    MatchOutcome outcome;
};

struct AdvanceResult
{
    TickConfirmMessage confirm;
    bool stepped = false;
    bool matchComplete = false;
    MatchOutcome outcome;
};

// One live match. The server calls Advance once per authoritative tick.
// Submit may rewrite a recent tick when the command is inside the rollback window.
class OnlineMatch
{
public:
    OnlineMatch(
        std::unique_ptr<IDeterministicMatch> sim,
        uint64_t seed,
        std::vector<uint32_t> playerIds,
        LatencyProfile profile);

    void NoteRtt(uint32_t playerId, uint32_t rttMs);
    uint32_t InputDelayTicks() const;
    uint32_t InterpolationDelayMs() const;

    InputAdmitResult Submit(const PlayerCommand& command);
    AdvanceResult Advance();
    bool CopySnapshotAfter(uint32_t tick, StateSnapshotMessage& out) const;

    uint32_t DroppedLateInputs() const { return droppedLate_; }
    uint32_t RollbackCount() const { return rollbacks_; }
    uint32_t NextTick() const { return nextTick_; }
    uint64_t ConfirmedHash() const { return latestHash_; }
    bool Finished() const { return finished_; }
    const std::vector<uint32_t>& PlayerIds() const { return playerIds_; }

private:
    struct TickRecord
    {
        std::map<uint32_t, PlayerCommand> inputs;
        std::vector<uint8_t> snapshotBefore;
        uint64_t hashAfter = 0;
    };

    void ApplyInputs(const std::map<uint32_t, PlayerCommand>& inputs);
    bool ResimulateFrom(uint32_t tick, TickConfirmMessage& correction);
    void RememberLatest();
    void RefreshFinished();
    void Trim();
    TickConfirmMessage MakeConfirm(uint32_t tick, bool corrected, bool withSnapshot) const;

    std::unique_ptr<IDeterministicMatch> sim_;
    uint64_t seed_ = 0;
    std::vector<uint32_t> playerIds_;
    LatencyProfile profile_{};
    std::map<uint32_t, uint32_t> rttMs_;
    std::map<uint32_t, TickRecord> history_;
    std::map<uint32_t, std::map<uint32_t, PlayerCommand>> pending_;
    uint32_t nextTick_ = 0;
    uint32_t droppedLate_ = 0;
    uint32_t rollbacks_ = 0;
    uint64_t latestHash_ = 0;
    uint32_t latestTick_ = 0;
    std::vector<uint8_t> latestSnapshot_;
    bool finished_ = false;
};

}
