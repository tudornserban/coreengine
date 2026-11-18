#pragma once

#include <cstdint>
#include <vector>

enum class TimedActionStatus : uint8_t {
    Running = 0,
    Completed = 1,
    Cancelled = 2,
    Interrupted = 3
};

struct TimedAction {
    uint64_t id = 0;
    uint64_t entityId = 0;
    uint32_t durationTicks = 0;
    uint32_t elapsedTicks = 0;
    TimedActionStatus status = TimedActionStatus::Running;
};

struct TimedActionEvent {
    uint64_t id = 0;
    uint64_t entityId = 0;
    TimedActionStatus status = TimedActionStatus::Running;
    uint32_t elapsedTicks = 0;
    uint32_t durationTicks = 0;
};

/// Tick-counted actions. Completion fires on the tick `elapsedTicks` reaches `durationTicks`.
/// Games attach costs and refunds; this clock only reports progress.
class TimedActionClock {
public:
    uint64_t Start(uint64_t entityId, uint32_t durationTicks);
    bool Cancel(uint64_t id);
    bool Interrupt(uint64_t id);

    /// Advance every running action by one tick. Returns each new terminal event once,
    /// including cancellations and interruptions requested since the previous drain.
    std::vector<TimedActionEvent> Advance();

    /// Terminal events queued by Cancel, Interrupt, or a previous Advance that was not drained.
    /// The same event is not returned again.
    std::vector<TimedActionEvent> TakeEvents();

    const TimedAction* Find(uint64_t id) const;
    uint64_t NextId() const { return nextId_; }
    std::vector<TimedAction> Capture() const;
    void Restore(std::vector<TimedAction> actions, uint64_t nextId);

    static float Progress(const TimedAction& action);
    static float Lerp(float from, float to, float t);

private:
    void PushTerminal(const TimedAction& action);

    std::vector<TimedAction> actions_;
    std::vector<TimedActionEvent> pendingEvents_;
    uint64_t nextId_ = 1;
};
