#pragma once

#include <cstdint>
#include <vector>

#include "runtime/command/Command.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementCommand.hpp"
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
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  bool forceStepWhilePaused = false;
  bool usePhysicsMovePlanner = false;
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

// MA4 s2 arming (§M3-literal, exposed for direct testing): arm a Move request for traversal ONLY when
// the acting NPC is a climber whose CURRENT route leg crosses a climb edge -- resolving the ONE
// bridging slot from the set-once registry. Player + unrouted/chasing NPCs leave the request
// default-off (the executeMovement path is then byte-identical).
void armAiMoveTraversal(const SessionState& state, EntityId actor, MovementRequest& request);

}  // namespace iggy3d
