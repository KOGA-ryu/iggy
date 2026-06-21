#pragma once

#include <cstdint>
#include <string>

#include "runtime/session/SessionState.hpp"

namespace iggy3d {

enum class RuntimeProofStatus : std::uint8_t {
  Unknown,
  Pass,
  Fail,
};

struct RuntimeSummaryInput {
  const SessionState* state = nullptr;
  CommandId retryExecutedCommandId = kInvalidCommandId;
  CommandSequence retryExecutedSequence = kInvalidCommandSequence;
  RuntimeProofStatus saveRoundtrip = RuntimeProofStatus::Unknown;
  RuntimeProofStatus resetBaseline = RuntimeProofStatus::Unknown;
  RuntimeProofStatus replayHash = RuntimeProofStatus::Unknown;
};

struct RuntimeSummary {
  std::string scenario;
  std::string lifecycle;
  std::string outcome;
  std::string finalTick;
  std::string playerPosition;
  std::string inventoryPlayer0;
  std::string goldKeyActive;
  std::string tacticalMarkerAlphaActive;
  std::string objectiveCollectGoldKey;
  std::string clockMode;
  std::string cameraMode;
  std::string cameraPreviousRealtime;
  std::string commandsSubmitted;
  std::string commandsAccepted;
  std::string commandsRejected;
  std::string commandsRetry;
  std::string commandsCombat;
  std::string combatTrainingDummyHp;
  std::string combatTrainingDummyDefeated;
  std::string combatLastAttackCommandId;
  std::string combatLastAttackSequence;
  std::string combatLastAttackDamage;
  std::string combatLastAttackTarget;
  std::string firstRejection;
  std::string retryOriginalRejectedCommandId;
  std::string retryCommandId;
  std::string retrySourceCommandId;
  std::string retryRetrySourceCommandId;
  std::string retryExecutedCommandId;
  std::string retryExecutedSequence;
  RuntimeProofStatus saveRoundtrip = RuntimeProofStatus::Unknown;
  RuntimeProofStatus resetBaseline = RuntimeProofStatus::Unknown;
  RuntimeProofStatus replayHash = RuntimeProofStatus::Unknown;
  std::string stateHash;
};

RuntimeSummary buildRuntimeSummary(const RuntimeSummaryInput& input);
std::string formatRuntimeSummary(const RuntimeSummary& summary);

}  // namespace iggy3d
