#include "ThinkTimer.h"

void ThinkTimer::Activate() {
    if (!IsExhausted()) {
        active = true;
    }
}

void ThinkTimer::Deactivate() {
    active = false;
}

void ThinkTimer::Update(float dt) {
    if (!active) return;
    bankRemaining -= dt;
    if (bankRemaining <= 0.0f) {
        bankRemaining = 0.0f;
        active = false;
    }
}
