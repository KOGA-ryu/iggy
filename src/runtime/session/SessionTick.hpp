#pragma once

#include <cstdint>
#include <vector>

#include "runtime/command/Command.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct EffectiveCommandIntent {
  CommandRecord command;
  CommandKind effectiveKind = CommandKind::None;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};

enum class SessionTickStatus : std::uint8_t {
  Stepped,
  NoWork,
  BlockedByPausedClock,
  SessionNotPlayable,
  InvalidState,
};

struct SessionTickInput {
  SessionState* state = nullptr;
  std::vector<CommandRecord> acceptedCommands;
  bool forceStepWhilePaused = false;
};

struct SessionTickResult {
  SessionTickStatus status = SessionTickStatus::InvalidState;
  CommandTick tickBefore = kInvalidCommandTick;
  CommandTick tickAfter = kInvalidCommandTick;
  std::uint32_t movementsExecuted = 0;
  std::uint32_t interactionsExecuted = 0;
  std::uint32_t combatExecuted = 0;
  std::uint32_t aiProposalsGenerated = 0;
  std::uint32_t eventsEmitted = 0;
  bool lifecycleChanged = false;
  SessionOutcome outcome = SessionOutcome::None;
  std::vector<CommandSequence> executedSequences;
};

SessionTickResult runSessionTick(const SessionTickInput& input);

}  // namespace iggy3d
