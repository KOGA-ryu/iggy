#pragma once

#include <cstdint>

namespace iggy3d {

struct RuntimeMetrics {
  std::uint64_t ticksRun = 0;
  std::uint64_t commandsSubmitted = 0;
  std::uint64_t commandsAccepted = 0;
  std::uint64_t commandsRejected = 0;
  std::uint64_t retryCommands = 0;
  std::uint64_t movementExecutions = 0;
  std::uint64_t interactionExecutions = 0;
  std::uint64_t combatExecutions = 0;
  std::uint64_t combatDefeats = 0;
  std::uint64_t abilityCasts = 0;
  std::uint64_t abilityImpacts = 0;
  std::uint64_t acquiredItems = 0;
  std::uint64_t completedObjectives = 0;
  std::uint64_t clockTransitions = 0;
  std::uint64_t cameraTransitions = 0;
  std::uint64_t saveCreations = 0;
  std::uint64_t loadCompletions = 0;
  std::uint64_t resetCompletions = 0;
  std::uint64_t replayCompletions = 0;
};

}  // namespace iggy3d
