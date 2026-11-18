#include "TimedActionClock.h"

#include <algorithm>

uint64_t TimedActionClock::Start(uint64_t entityId, uint32_t durationTicks) {
    TimedAction action;
    action.id = nextId_++;
    action.entityId = entityId;
    action.durationTicks = durationTicks;
    action.elapsedTicks = 0;
    action.status = TimedActionStatus::Running;
    actions_.push_back(action);
    return action.id;
}

void TimedActionClock::PushTerminal(const TimedAction& action) {
    pendingEvents_.push_back(TimedActionEvent{
        action.id, action.entityId, action.status, action.elapsedTicks, action.durationTicks});
}

bool TimedActionClock::Cancel(uint64_t id) {
    for (TimedAction& action : actions_) {
        if (action.id != id || action.status != TimedActionStatus::Running) {
            continue;
        }
        action.status = TimedActionStatus::Cancelled;
        PushTerminal(action);
        return true;
    }
    return false;
}

bool TimedActionClock::Interrupt(uint64_t id) {
    for (TimedAction& action : actions_) {
        if (action.id != id || action.status != TimedActionStatus::Running) {
            continue;
        }
        action.status = TimedActionStatus::Interrupted;
        PushTerminal(action);
        return true;
    }
    return false;
}

std::vector<TimedActionEvent> TimedActionClock::Advance() {
    for (TimedAction& action : actions_) {
        if (action.status != TimedActionStatus::Running) {
            continue;
        }
        if (action.elapsedTicks < action.durationTicks) {
            ++action.elapsedTicks;
        }
        if (action.durationTicks == 0 || action.elapsedTicks >= action.durationTicks) {
            action.elapsedTicks = action.durationTicks;
            action.status = TimedActionStatus::Completed;
            PushTerminal(action);
        }
    }
    return TakeEvents();
}

std::vector<TimedActionEvent> TimedActionClock::TakeEvents() {
    std::vector<TimedActionEvent> events;
    events.swap(pendingEvents_);
    return events;
}

const TimedAction* TimedActionClock::Find(uint64_t id) const {
    for (const TimedAction& action : actions_) {
        if (action.id == id) {
            return &action;
        }
    }
    return nullptr;
}

std::vector<TimedAction> TimedActionClock::Capture() const {
    return actions_;
}

void TimedActionClock::Restore(std::vector<TimedAction> actions, uint64_t nextId) {
    actions_ = std::move(actions);
    pendingEvents_.clear();
    nextId_ = std::max<uint64_t>(1, nextId);
    for (const TimedAction& action : actions_) {
        nextId_ = std::max(nextId_, action.id + 1);
    }
}

float TimedActionClock::Progress(const TimedAction& action) {
    if (action.durationTicks == 0) {
        return 1.0f;
    }
    return static_cast<float>(action.elapsedTicks) / static_cast<float>(action.durationTicks);
}

float TimedActionClock::Lerp(float from, float to, float t) {
    return from + (to - from) * t;
}
