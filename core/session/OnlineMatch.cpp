#include "OnlineMatch.h"

#include <algorithm>

namespace Session
{
namespace
{

MatchOutcome OutcomeFromSim(const IDeterministicMatch& sim, uint32_t steps, bool finished)
{
    MatchOutcome outcome = sim.Outcome();
    outcome.stateHash = sim.StateHash();
    outcome.ticksSimulated = steps;
    outcome.complete = finished;
    outcome.forfeit = false;
    return outcome;
}

}

OnlineMatch::OnlineMatch(
    std::unique_ptr<IDeterministicMatch> sim,
    uint64_t seed,
    std::vector<uint32_t> playerIds,
    LatencyProfile profile)
    : sim_(std::move(sim))
    , seed_(seed)
    , playerIds_(std::move(playerIds))
    , profile_(profile)
{
    std::sort(playerIds_.begin(), playerIds_.end());
    playerIds_.erase(std::unique(playerIds_.begin(), playerIds_.end()), playerIds_.end());
    if (!sim_)
    {
        finished_ = true;
        return;
    }
    sim_->Reset(seed_, playerIds_);
    latestHash_ = sim_->StateHash();
    latestSnapshot_ = sim_->Snapshot();
    RefreshFinished();
}

void OnlineMatch::NoteRtt(uint32_t playerId, uint32_t rttMs)
{
    if (std::find(playerIds_.begin(), playerIds_.end(), playerId) == playerIds_.end())
    {
        return;
    }
    rttMs_[playerId] = rttMs;
}

uint32_t OnlineMatch::InputDelayTicks() const
{
    uint32_t worst = 0;
    for (uint32_t playerId : playerIds_)
    {
        const auto it = rttMs_.find(playerId);
        if (it != rttMs_.end() && it->second > worst)
        {
            worst = it->second;
        }
    }
    return InputDelayTicksForRtt(worst, profile_);
}

uint32_t OnlineMatch::InterpolationDelayMs() const
{
    uint32_t worst = 0;
    for (uint32_t playerId : playerIds_)
    {
        const auto it = rttMs_.find(playerId);
        if (it != rttMs_.end() && it->second > worst)
        {
            worst = it->second;
        }
    }
    return Session::InterpolationDelayMs(worst);
}

void OnlineMatch::ApplyInputs(const std::map<uint32_t, PlayerCommand>& inputs)
{
    for (uint32_t playerId : playerIds_)
    {
        const auto it = inputs.find(playerId);
        if (it == inputs.end())
        {
            continue;
        }
        sim_->Apply(it->second);
    }
}

void OnlineMatch::RememberLatest()
{
    if (!sim_)
    {
        return;
    }
    latestHash_ = sim_->StateHash();
    latestSnapshot_ = sim_->Snapshot();
    if (nextTick_ > 0)
    {
        latestTick_ = nextTick_ - 1;
    }
}

void OnlineMatch::RefreshFinished()
{
    if (!sim_)
    {
        finished_ = true;
        return;
    }
    finished_ = sim_->IsComplete() || nextTick_ >= sim_->MaxTicks();
}

void OnlineMatch::Trim()
{
    if (profile_.rollbackWindowTicks < nextTick_)
    {
        const uint32_t oldest = nextTick_ - profile_.rollbackWindowTicks;
        for (auto it = history_.begin(); it != history_.end();)
        {
            if (it->first < oldest)
            {
                it = history_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (auto it = pending_.begin(); it != pending_.end();)
    {
        if (it->first < nextTick_)
        {
            it = pending_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

TickConfirmMessage OnlineMatch::MakeConfirm(uint32_t tick, bool corrected, bool withSnapshot) const
{
    TickConfirmMessage confirm;
    confirm.tick = tick;
    confirm.stateHash = latestHash_;
    confirm.corrected = corrected;
    confirm.inputDelayTicks = InputDelayTicks();
    confirm.interpolationDelayMs = InterpolationDelayMs();
    if (withSnapshot)
    {
        confirm.snapshot = latestSnapshot_;
    }
    return confirm;
}

bool OnlineMatch::ResimulateFrom(uint32_t tick, TickConfirmMessage& correction)
{
    if (!sim_ || nextTick_ == 0 || tick >= nextTick_)
    {
        return false;
    }
    const auto start = history_.find(tick);
    if (start == history_.end())
    {
        return false;
    }
    if (!sim_->Restore(start->second.snapshotBefore))
    {
        return false;
    }
    for (uint32_t cursor = tick; cursor < nextTick_; ++cursor)
    {
        auto record = history_.find(cursor);
        if (record == history_.end())
        {
            return false;
        }
        ApplyInputs(record->second.inputs);
        sim_->Step();
        record->second.hashAfter = sim_->StateHash();
        if (cursor + 1 < nextTick_)
        {
            history_[cursor + 1].snapshotBefore = sim_->Snapshot();
        }
    }
    ++rollbacks_;
    RememberLatest();
    RefreshFinished();
    correction = MakeConfirm(nextTick_ - 1, true, true);
    return true;
}

InputAdmitResult OnlineMatch::Submit(const PlayerCommand& command)
{
    InputAdmitResult result;
    if (!sim_ || finished_)
    {
        if (finished_)
        {
            ++droppedLate_;
            result.admit = InputAdmit::DroppedLate;
        }
        return result;
    }
    if (std::find(playerIds_.begin(), playerIds_.end(), command.playerId) == playerIds_.end())
    {
        return result;
    }
    if (command.tick >= nextTick_ && command.tick - nextTick_ > kMaxInputLeadTicks)
    {
        return result;
    }

    if (command.tick >= nextTick_)
    {
        pending_[command.tick][command.playerId] = command;
        result.admit = InputAdmit::Stored;
        return result;
    }

    const uint32_t age = nextTick_ - command.tick;
    if (age > profile_.rollbackWindowTicks)
    {
        ++droppedLate_;
        result.admit = InputAdmit::DroppedLate;
        return result;
    }

    auto record = history_.find(command.tick);
    if (record == history_.end())
    {
        ++droppedLate_;
        result.admit = InputAdmit::DroppedLate;
        return result;
    }
    const auto previous = record->second.inputs.find(command.playerId);
    const bool hadPrevious = previous != record->second.inputs.end();
    PlayerCommand previousCommand;
    if (hadPrevious)
    {
        previousCommand = previous->second;
    }
    record->second.inputs[command.playerId] = command;
    if (!ResimulateFrom(command.tick, result.correction))
    {
        if (hadPrevious)
        {
            record->second.inputs[command.playerId] = previousCommand;
        }
        else
        {
            record->second.inputs.erase(command.playerId);
        }
        ++droppedLate_;
        result.admit = InputAdmit::DroppedLate;
        return result;
    }
    result.admit = InputAdmit::RolledBack;
    result.matchComplete = finished_;
    if (finished_)
    {
        result.outcome = OutcomeFromSim(*sim_, nextTick_, true);
    }
    return result;
}

AdvanceResult OnlineMatch::Advance()
{
    AdvanceResult result;
    if (!sim_ || finished_)
    {
        result.matchComplete = finished_;
        if (sim_ && finished_)
        {
            result.outcome = OutcomeFromSim(*sim_, nextTick_, true);
            result.confirm = MakeConfirm(latestTick_, false, false);
        }
        return result;
    }

    TickRecord record;
    record.snapshotBefore = sim_->Snapshot();
    const auto pending = pending_.find(nextTick_);
    if (pending != pending_.end())
    {
        record.inputs = pending->second;
        pending_.erase(pending);
    }
    ApplyInputs(record.inputs);
    sim_->Step();
    record.hashAfter = sim_->StateHash();
    const uint32_t tick = nextTick_;
    history_[tick] = std::move(record);
    ++nextTick_;
    RememberLatest();
    RefreshFinished();
    Trim();

    result.stepped = true;
    result.matchComplete = finished_;
    result.confirm = MakeConfirm(tick, false, false);
    if (finished_)
    {
        result.outcome = OutcomeFromSim(*sim_, nextTick_, true);
    }
    return result;
}

bool OnlineMatch::CopySnapshotAfter(uint32_t tick, StateSnapshotMessage& out) const
{
    if (!sim_ || nextTick_ == 0 || tick >= nextTick_)
    {
        return false;
    }
    if (tick + 1 == nextTick_)
    {
        out.tick = tick;
        out.stateHash = latestHash_;
        out.snapshot = latestSnapshot_;
        return true;
    }
    const auto current = history_.find(tick);
    const auto next = history_.find(tick + 1);
    if (current == history_.end() || next == history_.end())
    {
        return false;
    }
    out.tick = tick;
    out.stateHash = current->second.hashAfter;
    out.snapshot = next->second.snapshotBefore;
    return true;
}

}
