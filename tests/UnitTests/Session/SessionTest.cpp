#include <gtest/gtest.h>

#include "core/session/IDeterministicMatch.h"
#include "core/session/ISessionTransport.h"
#include "core/session/InProcessHost.h"
#include "core/session/Latency.h"
#include "core/session/OfflineResolver.h"
#include "core/session/OnlineMatch.h"
#include "core/session/SessionClient.h"
#include "core/session/SessionCodec.h"
#include "core/session/SessionServer.h"
#include "core/session/SessionTypes.h"

#include <map>
#include <memory>
#include <vector>

using namespace Session;

namespace
{

class ScoreRace final : public IDeterministicMatch
{
public:
    void Reset(uint64_t seed, const std::vector<uint32_t>& playerIds) override
    {
        seed_ = seed;
        tick_ = 0;
        order_ = playerIds;
        scores_.clear();
        for (uint32_t id : order_)
        {
            scores_[id] = 0;
        }
    }

    void Apply(const PlayerCommand& command) override
    {
        const auto it = scores_.find(command.playerId);
        if (it == scores_.end())
        {
            return;
        }
        it->second += static_cast<int32_t>(command.action);
        it->second += command.x;
    }

    void Step() override { ++tick_; }

    uint64_t StateHash() const override
    {
        uint64_t hash = 14695981039346656037ull;
        auto mix = [&hash](uint64_t value)
        {
            for (int shift = 0; shift < 64; shift += 8)
            {
                hash ^= (value >> shift) & 0xffu;
                hash *= 1099511628211ull;
            }
        };
        mix(seed_);
        mix(tick_);
        for (uint32_t id : order_)
        {
            mix(id);
            mix(static_cast<uint32_t>(scores_.at(id)));
        }
        return hash;
    }

    bool IsComplete() const override { return tick_ >= kMaxTicks; }

    MatchOutcome Outcome() const override
    {
        MatchOutcome outcome;
        outcome.complete = IsComplete();
        bool any = false;
        bool tie = false;
        int32_t best = 0;
        int32_t winner = -1;
        for (uint32_t id : order_)
        {
            const int32_t score = scores_.at(id);
            outcome.scores.push_back(score);
            if (!any || score > best)
            {
                any = true;
                best = score;
                winner = static_cast<int32_t>(id);
                tie = false;
            }
            else if (score == best)
            {
                tie = true;
            }
        }
        outcome.winnerPlayerId = tie ? -1 : winner;
        outcome.stateHash = StateHash();
        outcome.ticksSimulated = tick_;
        return outcome;
    }

    uint32_t MaxTicks() const override { return kMaxTicks; }

    std::vector<uint8_t> Snapshot() const override
    {
        std::vector<uint8_t> bytes;
        auto put32 = [&bytes](uint32_t value)
        {
            for (int shift = 0; shift < 32; shift += 8)
            {
                bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xffu));
            }
        };
        auto put64 = [&bytes](uint64_t value)
        {
            for (int shift = 0; shift < 64; shift += 8)
            {
                bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xffu));
            }
        };
        put64(seed_);
        put32(tick_);
        put32(static_cast<uint32_t>(order_.size()));
        for (uint32_t id : order_)
        {
            put32(id);
            put32(static_cast<uint32_t>(scores_.at(id)));
        }
        return bytes;
    }

    bool Restore(const std::vector<uint8_t>& bytes) override
    {
        if (bytes.size() < 16)
        {
            return false;
        }
        size_t offset = 0;
        auto get32 = [&](uint32_t& value) -> bool
        {
            if (offset + 4 > bytes.size())
            {
                return false;
            }
            value = 0;
            for (int shift = 0; shift < 32; shift += 8)
            {
                value |= static_cast<uint32_t>(bytes[offset++]) << shift;
            }
            return true;
        };
        auto get64 = [&](uint64_t& value) -> bool
        {
            if (offset + 8 > bytes.size())
            {
                return false;
            }
            value = 0;
            for (int shift = 0; shift < 64; shift += 8)
            {
                value |= static_cast<uint64_t>(bytes[offset++]) << shift;
            }
            return true;
        };
        uint64_t seed = 0;
        uint32_t tick = 0;
        uint32_t count = 0;
        if (!get64(seed) || !get32(tick) || !get32(count) || count > 8)
        {
            return false;
        }
        if (bytes.size() != 16u + static_cast<size_t>(count) * 8u)
        {
            return false;
        }
        std::vector<uint32_t> order;
        std::map<uint32_t, int32_t> scores;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t id = 0;
            uint32_t score = 0;
            if (!get32(id) || !get32(score))
            {
                return false;
            }
            order.push_back(id);
            scores[id] = static_cast<int32_t>(score);
        }
        seed_ = seed;
        tick_ = tick;
        order_ = std::move(order);
        scores_ = std::move(scores);
        return true;
    }

private:
    static constexpr uint32_t kMaxTicks = 8;
    uint64_t seed_ = 0;
    uint32_t tick_ = 0;
    std::vector<uint32_t> order_;
    std::map<uint32_t, int32_t> scores_;
};

class ScoreRaceFactory final : public IMatchFactory
{
public:
    std::unique_ptr<IDeterministicMatch> Create(GameTitle) override
    {
        return std::make_unique<ScoreRace>();
    }
};

class DynamicGoOnlyFactory final : public IMatchFactory
{
public:
    std::unique_ptr<IDeterministicMatch> Create(GameTitle game) override
    {
        if (game != GameTitle::DynamicGo)
        {
            return nullptr;
        }
        return std::make_unique<ScoreRace>();
    }
};

DecisionPlan PlanFor(uint32_t playerId, const std::vector<PlayerCommand>& commands)
{
    DecisionPlan plan;
    plan.playerId = playerId;
    plan.commands = commands;
    return plan;
}

PlayerCommand Command(uint32_t playerId, uint32_t tick, uint32_t action, int32_t x = 0)
{
    PlayerCommand command;
    command.playerId = playerId;
    command.tick = tick;
    command.action = action;
    command.x = x;
    return command;
}

bool StartReadyMatch(InProcessHost& host, SessionClient& hostPlayer, SessionClient& guest, PlayMode mode, uint64_t seed)
{
    hostPlayer.CreateLobby(GameTitle::DynamicGo, mode, 2, seed);
    host.Pump();
    if (hostPlayer.Lobby() == nullptr)
    {
        return false;
    }
    guest.JoinLobby(hostPlayer.Lobby()->joinCode);
    host.Pump();
    hostPlayer.SetReady(true);
    guest.SetReady(true);
    host.Pump();
    hostPlayer.StartMatch();
    host.Pump();
    return hostPlayer.Lobby() != nullptr && hostPlayer.Lobby()->started;
}

}

TEST(SessionTypes, ParsesGameTitlesAndModes)
{
    GameTitle game = GameTitle::ZodiacWars;
    EXPECT_TRUE(TryParseGameTitle("DynamicGO", game));
    EXPECT_EQ(game, GameTitle::DynamicGo);
    EXPECT_TRUE(TryParseGameTitle("zodiacWars", game));
    EXPECT_EQ(game, GameTitle::ZodiacWars);
    EXPECT_STREQ(ToString(GameTitle::AscendantExpanse), "AscendantExpanse");

    PlayMode mode = PlayMode::OnlineSync;
    EXPECT_TRUE(TryParsePlayMode("offline", mode));
    EXPECT_EQ(mode, PlayMode::OfflineResolution);
    EXPECT_FALSE(TryParseGameTitle("venus", game));
}

TEST(SessionLatency, DelayCoversOneWayTripPlusJitter)
{
    LatencyProfile profile;
    EXPECT_EQ(InputDelayTicksForRtt(0, profile), 1u);
    EXPECT_EQ(InputDelayTicksForRtt(100, profile), 3u);
    EXPECT_EQ(InterpolationDelayMs(100), 50u);

    profile.maxInputDelayTicks = 2;
    EXPECT_EQ(InputDelayTicksForRtt(500, profile), 2u);
}

TEST(SessionCodec, PingEnvelopeIsStableLittleEndian)
{
    Envelope message;
    message.type = MessageType::Ping;
    message.playerId = 7;
    TimestampMessage stamp;
    stamp.clientTimeMs = 0x01020304u;
    message.payload = EncodeTimestamp(stamp);

    const std::vector<uint8_t> bytes = EncodeEnvelope(message);
    const std::vector<uint8_t> expected = {
        0x01, 0x00,
        0x07, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x04, 0x03, 0x02, 0x01
    };
    EXPECT_EQ(bytes, expected);

    Envelope decoded;
    ASSERT_TRUE(DecodeEnvelope(bytes, decoded));
    EXPECT_EQ(decoded.type, MessageType::Ping);
    EXPECT_EQ(decoded.playerId, 7u);
    TimestampMessage roundTrip;
    ASSERT_TRUE(DecodeTimestamp(decoded.payload, roundTrip));
    EXPECT_EQ(roundTrip.clientTimeMs, 0x01020304u);

    EXPECT_FALSE(DecodeEnvelope({0x01, 0x00}, decoded));
}

TEST(SessionCodec, NegativeCommandAndOutcomeRoundTrip)
{
    PlayerCommand command = Command(4, 2, 9, -3);
    command.y = -8;
    const std::vector<uint8_t> encoded = EncodePlayerCommand(command);
    PlayerCommand decoded;
    ASSERT_TRUE(DecodePlayerCommand(encoded, decoded));
    EXPECT_EQ(decoded.playerId, 4u);
    EXPECT_EQ(decoded.tick, 2u);
    EXPECT_EQ(decoded.action, 9u);
    EXPECT_EQ(decoded.x, -3);
    EXPECT_EQ(decoded.y, -8);

    MatchOutcome outcome;
    outcome.complete = true;
    outcome.forfeit = false;
    outcome.winnerPlayerId = -1;
    outcome.stateHash = 0x1122334455667788ull;
    outcome.ticksSimulated = 8;
    outcome.scores = {3, -2};
    MatchOutcome back;
    ASSERT_TRUE(DecodeMatchOutcome(EncodeMatchOutcome(outcome), back));
    EXPECT_TRUE(back.complete);
    EXPECT_FALSE(back.forfeit);
    EXPECT_EQ(back.winnerPlayerId, -1);
    EXPECT_EQ(back.stateHash, outcome.stateHash);
    EXPECT_EQ(back.scores, outcome.scores);
}

TEST(SessionOffline, ResolveIgnoresPlanOrder)
{
    const std::vector<uint32_t> ids = {2, 1};
    DecisionPlan first = PlanFor(1, {Command(1, 0, 2), Command(1, 3, 4, -1)});
    DecisionPlan second = PlanFor(2, {Command(9, 1, 5)});
    ScoreRace left;
    ScoreRace right;
    const MatchOutcome forward = ResolveOffline(left, 42, ids, {first, second});
    const MatchOutcome backward = ResolveOffline(right, 42, {1, 2}, {second, first});
    EXPECT_TRUE(forward.complete);
    EXPECT_EQ(forward.ticksSimulated, 8u);
    EXPECT_EQ(forward.stateHash, backward.stateHash);
    EXPECT_EQ(forward.scores, backward.scores);
    EXPECT_EQ(forward.winnerPlayerId, backward.winnerPlayerId);
    EXPECT_FALSE(forward.forfeit);
}

TEST(SessionOnline, LateInputInsideWindowRewritesHash)
{
    LatencyProfile profile;
    profile.rollbackWindowTicks = 6;
    OnlineMatch match(std::make_unique<ScoreRace>(), 7, {1, 2}, profile);
    for (int step = 0; step < 3; ++step)
    {
        EXPECT_TRUE(match.Advance().stepped);
    }
    const uint64_t before = match.ConfirmedHash();
    const InputAdmitResult admitted = match.Submit(Command(2, 1, 5));
    EXPECT_EQ(admitted.admit, InputAdmit::RolledBack);
    EXPECT_EQ(match.RollbackCount(), 1u);
    EXPECT_NE(match.ConfirmedHash(), before);
    EXPECT_TRUE(admitted.correction.corrected);
    EXPECT_FALSE(admitted.correction.snapshot.empty());

    ScoreRace expected;
    const MatchOutcome outcome = ResolveOffline(
        expected,
        7,
        {1, 2},
        {PlanFor(2, {Command(2, 1, 5)})});
    ScoreRace replay;
    replay.Reset(7, {1, 2});
    replay.Step();
    replay.Apply(Command(2, 1, 5));
    replay.Step();
    replay.Step();
    EXPECT_EQ(match.ConfirmedHash(), replay.StateHash());
    EXPECT_EQ(admitted.correction.snapshot, replay.Snapshot());
    EXPECT_NE(match.ConfirmedHash(), outcome.stateHash);
}

TEST(SessionOnline, LateInputOutsideWindowIsDropped)
{
    LatencyProfile profile;
    profile.rollbackWindowTicks = 1;
    OnlineMatch match(std::make_unique<ScoreRace>(), 3, {1, 2}, profile);
    for (int step = 0; step < 4; ++step)
    {
        match.Advance();
    }
    const uint64_t before = match.ConfirmedHash();
    const InputAdmitResult admitted = match.Submit(Command(1, 0, 9));
    EXPECT_EQ(admitted.admit, InputAdmit::DroppedLate);
    EXPECT_EQ(match.DroppedLateInputs(), 1u);
    EXPECT_EQ(match.RollbackCount(), 0u);
    EXPECT_EQ(match.ConfirmedHash(), before);
}

TEST(SessionOnline, InputDelayUsesWorstRtt)
{
    LatencyProfile profile;
    OnlineMatch match(std::make_unique<ScoreRace>(), 1, {1, 2}, profile);
    match.NoteRtt(1, 0);
    match.NoteRtt(2, 100);
    EXPECT_EQ(match.InputDelayTicks(), 3u);
    EXPECT_EQ(match.InterpolationDelayMs(), 50u);
}

TEST(SessionLobby, ReadyGateAndFullLobby)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    SessionClient* cara = host.Connect("Cara");
    ASSERT_NE(alice, nullptr);
    ASSERT_NE(bob, nullptr);
    ASSERT_NE(cara, nullptr);
    EXPECT_EQ(host.Connect(""), nullptr);

    alice->CreateLobby(GameTitle::DynamicGo, PlayMode::OfflineResolution, 2, 99);
    host.Pump();
    ASSERT_NE(alice->Lobby(), nullptr);
    EXPECT_TRUE(alice->Lobby()->players.front().host);
    EXPECT_FALSE(alice->Lobby()->started);

    alice->StartMatch();
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "need-players");

    bob->JoinLobby(alice->Lobby()->joinCode);
    host.Pump();
    ASSERT_NE(bob->Lobby(), nullptr);
    EXPECT_EQ(bob->Lobby()->players.size(), 2u);

    alice->StartMatch();
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "not-ready");
    EXPECT_FALSE(alice->Lobby()->started);

    bob->StartMatch();
    host.Pump();
    EXPECT_EQ(bob->LastReject(), "not-host");

    alice->SetReady(true);
    bob->SetReady(true);
    host.Pump();
    alice->StartMatch();
    host.Pump();
    EXPECT_TRUE(alice->Lobby()->started);
    EXPECT_EQ(alice->Lobby()->seed, 99u);

    cara->JoinLobby(alice->Lobby()->joinCode);
    host.Pump();
    EXPECT_EQ(cara->LastReject(), "match-started");

    InProcessHost open(std::make_unique<ScoreRaceFactory>());
    SessionClient* hostPlayer = open.Connect("Host");
    SessionClient* second = open.Connect("Second");
    SessionClient* third = open.Connect("Third");
    hostPlayer->CreateLobby(GameTitle::AscendantExpanse, PlayMode::OnlineSync, 2, 1);
    open.Pump();
    second->JoinLobby(hostPlayer->Lobby()->joinCode);
    open.Pump();
    third->JoinLobby(hostPlayer->Lobby()->joinCode);
    open.Pump();
    EXPECT_EQ(third->LastReject(), "lobby-full");
    EXPECT_EQ(third->Lobby(), nullptr);
}

TEST(SessionLobby, MissingRulesDoNotStart)
{
    InProcessHost host(std::make_unique<DynamicGoOnlyFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    alice->CreateLobby(GameTitle::ZodiacWars, PlayMode::OnlineSync, 2, 5);
    host.Pump();
    bob->JoinLobby(alice->Lobby()->joinCode);
    host.Pump();
    alice->SetReady(true);
    bob->SetReady(true);
    host.Pump();
    alice->StartMatch();
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "no-rules");
    EXPECT_FALSE(alice->Lobby()->started);
}

TEST(SessionMatchmaking, PairsSameGameAndModeOnly)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    SessionClient* cara = host.Connect("Cara");
    alice->Enqueue(GameTitle::DynamicGo, PlayMode::OfflineResolution, 2);
    host.Pump();
    EXPECT_TRUE(alice->WaitingInQueue());
    EXPECT_EQ(alice->QueueAhead(), 0u);
    EXPECT_EQ(host.Server().QueueSize(), 1u);

    bob->Enqueue(GameTitle::DynamicGo, PlayMode::OnlineSync, 2);
    host.Pump();
    EXPECT_TRUE(bob->WaitingInQueue());
    EXPECT_TRUE(alice->WaitingInQueue());

    cara->Enqueue(GameTitle::AscendantExpanse, PlayMode::OfflineResolution, 2);
    host.Pump();
    EXPECT_TRUE(cara->WaitingInQueue());

    bob->Leave();
    host.Pump();
    EXPECT_FALSE(bob->WaitingInQueue());

    bob->Enqueue(GameTitle::DynamicGo, PlayMode::OfflineResolution, 2);
    host.Pump();
    ASSERT_NE(alice->Lobby(), nullptr);
    ASSERT_NE(bob->Lobby(), nullptr);
    EXPECT_TRUE(alice->Lobby()->started);
    EXPECT_EQ(alice->Lobby()->game, GameTitle::DynamicGo);
    EXPECT_EQ(alice->Lobby()->mode, PlayMode::OfflineResolution);
    EXPECT_EQ(alice->Lobby()->seed, bob->Lobby()->seed);
    EXPECT_FALSE(alice->WaitingInQueue());
    EXPECT_TRUE(cara->WaitingInQueue());
    EXPECT_EQ(host.Server().QueueSize(), 1u);
}

TEST(SessionOfflineMatch, ServerResultMatchesLocalReplay)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    ASSERT_TRUE(StartReadyMatch(host, *alice, *bob, PlayMode::OfflineResolution, 11));

    DecisionPlan alicePlan = PlanFor(99, {Command(99, 0, 1), Command(99, 2, 4)});
    DecisionPlan bobPlan = PlanFor(99, {Command(99, 1, 2, -1)});
    bob->SubmitPlan(bobPlan);
    host.Pump();
    EXPECT_EQ(alice->Result(), nullptr);
    alice->SubmitPlan(alicePlan);
    host.Pump();

    ASSERT_NE(alice->Result(), nullptr);
    ASSERT_NE(bob->Result(), nullptr);
    EXPECT_EQ(alice->Result()->stateHash, bob->Result()->stateHash);
    EXPECT_FALSE(alice->Result()->forfeit);
    EXPECT_TRUE(alice->Result()->complete);
    EXPECT_EQ(alice->Result()->ticksSimulated, 8u);

    ScoreRace replay;
    const std::vector<uint32_t> ids = {alice->PlayerId(), bob->PlayerId()};
    DecisionPlan aliceFixed = alicePlan;
    aliceFixed.playerId = alice->PlayerId();
    DecisionPlan bobFixed = bobPlan;
    bobFixed.playerId = bob->PlayerId();
    const MatchOutcome local = ResolveOffline(replay, 11, ids, {bobFixed, aliceFixed});
    EXPECT_EQ(alice->Result()->stateHash, local.stateHash);
    EXPECT_EQ(alice->Result()->scores, local.scores);
    EXPECT_EQ(alice->Result()->winnerPlayerId, local.winnerPlayerId);

    alice->SubmitPlan(alicePlan);
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "plan-already-submitted");
}

TEST(SessionOfflineMatch, DisconnectForfeitsToThePlayerWhoStays)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    ASSERT_TRUE(StartReadyMatch(host, *alice, *bob, PlayMode::OfflineResolution, 4));
    host.Disconnect(*bob);
    ASSERT_NE(alice->Result(), nullptr);
    EXPECT_TRUE(alice->Result()->forfeit);
    EXPECT_TRUE(alice->Result()->complete);
    EXPECT_EQ(alice->Result()->winnerPlayerId, static_cast<int32_t>(alice->PlayerId()));
    EXPECT_EQ(bob->Lobby(), nullptr);
}

TEST(SessionOfflineMatch, SpoofedPlayerIdIsIgnored)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    ASSERT_TRUE(StartReadyMatch(host, *alice, *bob, PlayMode::OfflineResolution, 8));

    DecisionPlan spoofed = PlanFor(bob->PlayerId(), {Command(bob->PlayerId(), 0, 20)});
    Envelope message;
    message.type = MessageType::SubmitPlan;
    message.playerId = alice->PlayerId();
    message.payload = EncodeDecisionPlan(spoofed);
    host.Server().Receive(alice->PlayerId(), message);

    DecisionPlan honest = PlanFor(bob->PlayerId(), {Command(bob->PlayerId(), 0, 1)});
    Envelope bobMessage;
    bobMessage.type = MessageType::SubmitPlan;
    bobMessage.playerId = bob->PlayerId();
    bobMessage.payload = EncodeDecisionPlan(honest);
    host.Server().Receive(bob->PlayerId(), bobMessage);
    host.Pump();

    ScoreRace replay;
    DecisionPlan aliceOwned = PlanFor(alice->PlayerId(), {Command(alice->PlayerId(), 0, 20)});
    const MatchOutcome local = ResolveOffline(
        replay,
        8,
        {alice->PlayerId(), bob->PlayerId()},
        {aliceOwned, honest});
    ASSERT_NE(alice->Result(), nullptr);
    EXPECT_EQ(alice->Result()->stateHash, local.stateHash);
    EXPECT_EQ(alice->Result()->winnerPlayerId, static_cast<int32_t>(alice->PlayerId()));
}

TEST(SessionOnlineMatch, LockstepHashMatchesOfflineResolution)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    ASSERT_TRUE(StartReadyMatch(host, *alice, *bob, PlayMode::OnlineSync, 15));
    ASSERT_EQ(alice->Lobby()->inputDelayTicks, 1u);

    std::vector<PlayerCommand> aliceCommands;
    std::vector<PlayerCommand> bobCommands;
    for (uint32_t tick = 0; tick < 8; ++tick)
    {
        aliceCommands.push_back(Command(alice->PlayerId(), tick, tick == 0 ? 3u : 0u));
        bobCommands.push_back(Command(bob->PlayerId(), tick, 1));
        alice->SubmitInput(aliceCommands.back());
        bob->SubmitInput(bobCommands.back());
    }
    host.Pump();
    for (int step = 0; step < 8; ++step)
    {
        host.AdvanceOnline();
    }

    ASSERT_NE(alice->Result(), nullptr);
    ASSERT_NE(bob->Result(), nullptr);
    EXPECT_EQ(alice->Result()->stateHash, bob->Result()->stateHash);
    EXPECT_TRUE(alice->HasConfirmedTick());
    EXPECT_EQ(alice->LastConfirmedTick(), 7u);
    EXPECT_EQ(alice->ConfirmedHash(), alice->Result()->stateHash);

    ScoreRace replay;
    const MatchOutcome local = ResolveOffline(
        replay,
        15,
        {alice->PlayerId(), bob->PlayerId()},
        {PlanFor(alice->PlayerId(), aliceCommands), PlanFor(bob->PlayerId(), bobCommands)});
    EXPECT_EQ(alice->Result()->stateHash, local.stateHash);
    EXPECT_EQ(alice->Result()->scores, local.scores);
}

TEST(SessionOnlineMatch, RttRaisesInputDelayAndRollbackRepairsLateCommand)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    LatencyProfile profile;
    profile.rollbackWindowTicks = 6;
    host.Server().SetLatencyProfile(profile);

    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    alice->CreateLobby(GameTitle::DynamicGo, PlayMode::OnlineSync, 2, 21);
    host.Pump();
    bob->JoinLobby(alice->Lobby()->joinCode);
    host.Pump();

    alice->SendPing(1000);
    bob->SendPing(1000);
    host.Pump();
    alice->CompletePing(1000);
    bob->CompletePing(1100);
    host.Pump();
    EXPECT_EQ(alice->RoundTripMs(), 0u);
    EXPECT_EQ(bob->RoundTripMs(), 100u);

    alice->SetReady(true);
    bob->SetReady(true);
    host.Pump();
    alice->StartMatch();
    host.Pump();
    EXPECT_EQ(alice->InputDelayTicks(), 3u);
    EXPECT_EQ(bob->InterpolationDelayMs(), 50u);
    EXPECT_EQ(alice->Lobby()->interpolationDelayMs, 50u);

    for (int step = 0; step < 3; ++step)
    {
        host.AdvanceOnline();
    }
    const uint64_t stale = alice->ConfirmedHash();
    bob->SubmitInput(Command(bob->PlayerId(), 1, 6));
    host.Pump();
    EXPECT_NE(alice->ConfirmedHash(), stale);
    EXPECT_TRUE(alice->HasAuthoritativeSnapshot());
    EXPECT_EQ(alice->ConfirmedHash(), bob->ConfirmedHash());

    ScoreRace replay;
    replay.Reset(21, {alice->PlayerId(), bob->PlayerId()});
    replay.Step();
    replay.Apply(Command(bob->PlayerId(), 1, 6));
    replay.Step();
    replay.Step();
    EXPECT_EQ(alice->ConfirmedHash(), replay.StateHash());
    EXPECT_EQ(*alice->AuthoritativeSnapshot(), replay.Snapshot());
}

TEST(SessionOnlineMatch, FinishedMatchDropsLateInputAndHashMismatchSendsSnapshot)
{
    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    SessionClient* bob = host.Connect("Bob");
    ASSERT_TRUE(StartReadyMatch(host, *alice, *bob, PlayMode::OnlineSync, 2));
    for (int step = 0; step < 8; ++step)
    {
        host.AdvanceOnline();
    }
    ASSERT_NE(alice->Result(), nullptr);

    alice->SubmitInput(Command(alice->PlayerId(), 7, 4));
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "input-late");

    alice->ReportLocalHash(alice->LastConfirmedTick(), alice->ConfirmedHash() ^ 0xffu);
    EXPECT_TRUE(alice->HashMismatch());
    host.Pump();
    EXPECT_FALSE(alice->HashMismatch());
    EXPECT_TRUE(alice->HasAuthoritativeSnapshot());
    EXPECT_EQ(alice->ConfirmedHash(), bob->ConfirmedHash());
}

TEST(SessionTransport, InProcessPipeRoundTripsEnvelopeBytes)
{
    InProcessTransport transport;
    Envelope message;
    message.type = MessageType::Reject;
    message.playerId = 3;
    message.payload = EncodeReject("lobby-full");
    transport.SendToClient(3, message);

    std::vector<Envelope> incoming;
    transport.PollClient(3, incoming);
    ASSERT_EQ(incoming.size(), 1u);
    std::string reason;
    ASSERT_TRUE(DecodeReject(incoming[0].payload, reason));
    EXPECT_EQ(reason, "lobby-full");
    EXPECT_EQ(incoming[0].playerId, 3u);
}

TEST(SessionVenusWars, SetupRoundTripsAndRejectsBadPayloads)
{
    MatchSetup setup;
    setup.schemaVersion = kMatchSetupSchemaVersion;
    setup.contentReference = "scenarios/alpha";
    setup.payload = {9, 4, 1};

    const std::vector<uint8_t> bytes = EncodeMatchSetup(setup);
    MatchSetup decoded;
    ASSERT_TRUE(DecodeMatchSetup(bytes, decoded));
    EXPECT_EQ(decoded.contentReference, setup.contentReference);
    EXPECT_EQ(decoded.payload, setup.payload);
    EXPECT_EQ(std::string(ToString(GameTitle::VenusWars)), "VenusWars");
    GameTitle parsed = GameTitle::DynamicGo;
    EXPECT_TRUE(TryParseGameTitle("VenusWars", parsed));
    EXPECT_EQ(parsed, GameTitle::VenusWars);

    MatchSetup bad = setup;
    bad.schemaVersion = 99;
    EXPECT_EQ(ValidateMatchSetup(bad), SetupReject::UnsupportedVersion);
    bad = setup;
    bad.payload.assign(kMaxSetupPayloadBytes + 1, std::uint8_t{1});
    EXPECT_EQ(ValidateMatchSetup(bad), SetupReject::Oversized);
    bad = setup;
    bad.contentReference = "bad ref";
    EXPECT_EQ(ValidateMatchSetup(bad), SetupReject::InvalidReference);

    InProcessHost host(std::make_unique<ScoreRaceFactory>());
    SessionClient* alice = host.Connect("Alice");
    alice->CreateLobby(GameTitle::VenusWars, PlayMode::OfflineResolution, 2, 7);
    host.Pump();
    alice->SubmitSetup(setup);
    host.Pump();
    const MatchSetup* stored = host.Server().SetupForPlayer(alice->PlayerId());
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->payload, setup.payload);

    alice->SubmitSetup(bad);
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "");

    Envelope forged;
    forged.type = MessageType::SubmitSetup;
    forged.playerId = alice->PlayerId();
    forged.payload = {1, 2, 3};
    host.Server().Receive(alice->PlayerId(), forged);
    host.Pump();
    EXPECT_EQ(alice->LastReject(), "bad-setup");
    EXPECT_EQ(host.Server().SetupForPlayer(alice->PlayerId())->payload, setup.payload);
}

TEST(SessionVenusWars, OfflineResolutionMatchesLocalSteps)
{
    std::vector<uint32_t> ids = {1, 2};
    DecisionPlan alice = PlanFor(1, {PlayerCommand{1, 0, 3, 1, 0}});
    DecisionPlan bob = PlanFor(2, {PlayerCommand{2, 1, 1, 0, 0}});

    ScoreRace local;
    local.Reset(11, ids);
    local.Apply(alice.commands[0]);
    local.Step();
    local.Apply(bob.commands[0]);
    local.Step();
    while (!local.IsComplete())
    {
        local.Step();
    }

    ScoreRace remote;
    const MatchOutcome outcome = ResolveOffline(remote, 11, ids, {bob, alice});
    EXPECT_EQ(outcome.ticksSimulated, local.MaxTicks());
    EXPECT_EQ(outcome.stateHash, local.StateHash());
    EXPECT_EQ(local.Outcome().scores, outcome.scores);
}
