#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/RuntimeConfig.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcSoundPerception.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/diagnostics/RuntimeEvent.hpp"
#include "runtime/diagnostics/RuntimeMetrics.hpp"
#include "runtime/inventory/InventoryState.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

enum class SessionLifecycle : std::uint8_t {
  Loading,
  Playing,
  Paused,
  Complete,
  Failed,
};

enum class SessionOutcome : std::uint8_t {
  None,
  DemoComplete,
  Victory,
  Defeat,
  Failed,
};

enum class SessionResetPolicy : std::uint8_t {
  ClearCommandLog,
  NewCommandEpoch,
};

struct SessionIdentity {
  std::string packageId;
  std::string scenarioId;
  std::uint64_t sessionSeed = 0;
  std::uint32_t schemaVersion = 1;
};

struct BaselineSnapshot {
  SessionIdentity identity;
  RuntimeConfig config;
  WorldState world;
  PlayerRoster players;
  ClockState clock;
  CameraState camera;
  AbilityState abilities;
  InventoryState inventory;
  CombatState combat;
  AiState ai;
  ObjectiveState objectives;
  std::uint64_t baselineHash = 0;
};

struct SessionTransientState {
  AbilityRuntimeState abilityRuntime;
  std::vector<RuntimeEvent> events;
  RuntimeMetrics metrics;
  std::vector<CommandSequence> pendingExecutionSequences;
  bool lastMovementResultAvailable = false;
  MovementResult lastMovementResult;
  // Per-tick sound bus (a1s2, L1). Movement execution PUSHES a SoundEvent per moving
  // player; the AI loop (enqueueNpcBehaviorCommands) READS it to resolve per-guard
  // hearing. Cleared at tick start so hearing sees only THIS tick's noise. Transient
  // (not persisted/hashed) -- it is fully regenerated each tick from movement.
  std::vector<SoundEvent> soundEvents;
  bool cameraInputClearRequested = false;
  bool summaryDirty = true;
  bool stateHashDirty = true;
};

struct SessionState {
  SessionIdentity identity;
  RuntimeConfig config;
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;

  WorldState world;
  PlayerRoster players;
  ClockState clock;
  CameraState camera;
  AbilityState abilities;
  CommandLog commandLog;
  CommandId nextCommandId = 1;

  InventoryState inventory;
  CombatState combat;
  AiState ai;
  ObjectiveState objectives;

  BaselineSnapshot baseline;
  SessionTransientState transient;

  // L4 reasoning graph (A3). TRANSIENT-IN-THE-PERSISTENCE-SENSE: it lives OUTSIDE `transient` so it
  // survives ticks (transient is cleared every tick), but it is deliberately absent from StateHash,
  // SaveCodec and SaveEnvelope (the A4 route-state precedent) -- never hashed, never serialized. It
  // is set ONCE from the UNFILTERED activation-time RoomAsset (never the runtime-filtered collision
  // view) and is NOT rebuilt to track runtime state in A3; dynamic openings/locks become future
  // `locked` edge annotations, not rebuilds. After replaceStateFromLoad the slot is default-empty
  // (the envelope never carried it) -- a valid empty value, not stale/dangling.
  ReasoningGraph reasoningGraph;

  std::uint64_t currentStateHash = 0;
};

}  // namespace iggy3d
