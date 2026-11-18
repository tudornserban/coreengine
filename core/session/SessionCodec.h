#pragma once

#include "SessionTypes.h"

#include <vector>

namespace Session
{

std::vector<uint8_t> EncodeEnvelope(const Envelope& message);
bool DecodeEnvelope(const std::vector<uint8_t>& bytes, Envelope& message);

std::vector<uint8_t> EncodeCreateLobby(const CreateLobbyRequest& request);
bool DecodeCreateLobby(const std::vector<uint8_t>& bytes, CreateLobbyRequest& request);

std::vector<uint8_t> EncodeJoinLobby(const JoinLobbyRequest& request);
bool DecodeJoinLobby(const std::vector<uint8_t>& bytes, JoinLobbyRequest& request);

std::vector<uint8_t> EncodeSetReady(bool ready);
bool DecodeSetReady(const std::vector<uint8_t>& bytes, bool& ready);

std::vector<uint8_t> EncodeEnqueue(const EnqueueRequest& request);
bool DecodeEnqueue(const std::vector<uint8_t>& bytes, EnqueueRequest& request);

std::vector<uint8_t> EncodeLobbyView(const LobbyView& lobby);
bool DecodeLobbyView(const std::vector<uint8_t>& bytes, LobbyView& lobby);

std::vector<uint8_t> EncodeQueueStatus(const QueueStatusMessage& status);
bool DecodeQueueStatus(const std::vector<uint8_t>& bytes, QueueStatusMessage& status);

std::vector<uint8_t> EncodeDecisionPlan(const DecisionPlan& plan);
bool DecodeDecisionPlan(const std::vector<uint8_t>& bytes, DecisionPlan& plan);

std::vector<uint8_t> EncodeMatchSetup(const MatchSetup& setup);
bool DecodeMatchSetup(const std::vector<uint8_t>& bytes, MatchSetup& setup);

std::vector<uint8_t> EncodeMatchOutcome(const MatchOutcome& outcome);
bool DecodeMatchOutcome(const std::vector<uint8_t>& bytes, MatchOutcome& outcome);

std::vector<uint8_t> EncodePlayerCommand(const PlayerCommand& command);
bool DecodePlayerCommand(const std::vector<uint8_t>& bytes, PlayerCommand& command);

std::vector<uint8_t> EncodeTickConfirm(const TickConfirmMessage& confirm);
bool DecodeTickConfirm(const std::vector<uint8_t>& bytes, TickConfirmMessage& confirm);

std::vector<uint8_t> EncodeStateSnapshot(const StateSnapshotMessage& snapshot);
bool DecodeStateSnapshot(const std::vector<uint8_t>& bytes, StateSnapshotMessage& snapshot);

std::vector<uint8_t> EncodeTimestamp(const TimestampMessage& stamp);
bool DecodeTimestamp(const std::vector<uint8_t>& bytes, TimestampMessage& stamp);

std::vector<uint8_t> EncodeHashReport(const HashReportMessage& report);
bool DecodeHashReport(const std::vector<uint8_t>& bytes, HashReportMessage& report);

std::vector<uint8_t> EncodeReject(const std::string& reason);
bool DecodeReject(const std::vector<uint8_t>& bytes, std::string& reason);

}
