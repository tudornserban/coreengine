#pragma once



// Per-player "think time" bank. When active, the entire game simulation
// freezes while the bank drains in real time.
// Raylib-free — suitable for unit testing and Unreal portability.
class ThinkTimer {
private:
    float bankRemaining;
    bool  active = false;

public:
    explicit ThinkTimer(float initialBank) : bankRemaining(initialBank) {}

    // Start draining the bank. No-op if already active or exhausted.
    void Activate();

    // Stop draining. No-op if already inactive.
    void Deactivate();

    // Drain bank by dt seconds while active. Auto-deactivates on exhaustion.
    void Update(float dt);

    [[nodiscard]] bool  IsActive()        const { return active; }
    [[nodiscard]] float GetBankRemaining() const { return bankRemaining; }
    [[nodiscard]] bool  IsExhausted()     const { return bankRemaining <= 0.0f; }
};


