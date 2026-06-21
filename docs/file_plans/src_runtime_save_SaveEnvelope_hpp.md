# `src/runtime/save/SaveEnvelope.hpp`

Updated: 2026-06-20

Exact purpose: declare the versioned save container for durable runtime state.

## Build Position

- priority rank: 111
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `header`

## Ownership

This file owns:

- save schema version
- runtime version
- package/scenario ids
- session payload sections
- state hash field

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- standard library: `<cstdint>`, `<string>`, `<vector>`;
- core values: `core/ids/EntityId.hpp`, `core/math/Aabb3.hpp`,
  `core/math/Transform3.hpp`, `core/math/Vec3.hpp`;
- runtime value headers: `SessionState.hpp`, `Command.hpp`, `CommandLog.hpp`,
  `WorldState.hpp`/`EntityState.hpp`, `PlayerSlot.hpp`, `ClockState.hpp`,
  `CameraState.hpp`, `InteractionDefinition.hpp`, `InventoryState.hpp`,
  `CombatState.hpp`, `AiState.hpp`, and `ObjectiveState.hpp`;
- no app, projection, renderer, tests, socket, filesystem, or old iggy includes.

## Data Contract

- world/player/clock/camera/inventory/combat/ai/objective/command log sections
- excludes projection, renderer, raw input, runtime summary, events, and metrics

## Semantics

- save envelope is the durable boundary
- load validates version before mutation
- state hash verifies payload consistency

## Detailed Design Contract

Declare these first-build constants exactly:

```cpp
inline constexpr std::uint32_t kSaveSchemaVersion = 1;
inline constexpr std::uint32_t kMinimumReadableSaveSchemaVersion = 1;
inline constexpr std::uint32_t kRuntimeSaveVersion = 1;
```

Declare these value types exactly, adjusting only namespace qualification for
types already declared elsewhere:

```cpp
struct SaveEnvelopeMetadata {
  std::uint32_t schemaVersion = kSaveSchemaVersion;
  std::uint32_t minimumReadableSchemaVersion =
      kMinimumReadableSaveSchemaVersion;
  std::uint32_t runtimeSaveVersion = kRuntimeSaveVersion;
  std::string packageId;
  std::string scenarioId;
  std::string createdByToolId;
  std::uint64_t savedStateHash = 0;
  std::string savedStateHashHex;
};

struct SaveSessionSection {
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;
  std::uint64_t currentTick = 0;
  CommandId nextCommandId = 1;
  std::uint64_t sessionSeed = 0;
  std::uint32_t sessionSchemaVersion = 1;
  std::string packageId;
  std::string scenarioId;
};

struct SaveEntityRecord {
  EntityId id;
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  bool targetable = false;
  std::vector<TargetAction> targetActions;
  InteractionKind interactionKind = InteractionKind::None;
  InteractionEffectKind interactionPrimaryEffect = InteractionEffectKind::None;
  std::string interactionItemId;
  std::uint32_t interactionItemCount = 0;
  std::string interactionObjectiveId;
  bool interactionRepeatable = false;
  bool interactionDeactivateTargetOnSuccess = false;
};

struct SaveWorldSection {
  EntityId nextEntityId;
  std::vector<SaveEntityRecord> entities;
};

struct SavePlayerSlotRecord {
  PlayerSlotId slotId = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  EntityId controlledActor;
  std::string stableName;
};

struct SavePlayerSection {
  std::vector<SavePlayerSlotRecord> slots;
};

struct SaveClockSection {
  ClockMode mode = ClockMode::Normal;
  ClockMode previousUnpausedMode = ClockMode::Normal;
  float previousUnpausedTimeScale = 1.0F;
  std::uint64_t tickIndex = 0;
  std::uint32_t fixedTickRateHz = 20;
  float timeScale = 1.0F;
};

struct SaveCameraSection {
  CameraMode activeMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeMode = CameraMode::ThirdPerson;
  EntityId targetEntity;
  Vec3 targetPoint;
  bool targetHasPoint = false;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  float orbitDistance = 8.0F;
};

struct SaveCommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  CommandKind kind = CommandKind::None;
  CommandSource source = CommandSource::Unknown;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  bool hasTargetEntity = false;
  EntityId targetEntity;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  CommandId retrySourceCommandId = kInvalidCommandId;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};

struct SaveCommandLogSection {
  CommandLogResetPolicy resetPolicy = CommandLogResetPolicy::Clear;
  CommandSequence nextSequence = 1;
  std::uint64_t epoch = 0;
  std::vector<SaveCommandRecord> records;
};

struct SaveInventoryStackRecord {
  std::string itemId;
  std::uint32_t count = 0;
};

struct SavePlayerInventoryRecord {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::vector<SaveInventoryStackRecord> stacks;
};

struct SaveInventorySection {
  std::vector<SavePlayerInventoryRecord> players;
};

struct SaveCombatantRecord {
  EntityId entity;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
  bool defeated = false;
};

struct SaveCombatSection {
  std::vector<SaveCombatantRecord> combatants;
};

struct SaveAiActorRecord {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
};

struct SaveAiSection {
  std::vector<SaveAiActorRecord> actors;
};

struct SaveObjectiveRecord {
  std::string objectiveId;
  ObjectiveStatus status = ObjectiveStatus::Inactive;
  ObjectiveConditionKind conditionKind = ObjectiveConditionKind::None;
  PlayerSlotId conditionPlayerSlot = kInvalidPlayerSlotId;
  std::string conditionItemId;
  std::uint32_t conditionItemCount = 0;
};

struct SaveObjectiveSection {
  std::vector<SaveObjectiveRecord> objectives;
};

struct SaveEnvelope {
  SaveEnvelopeMetadata metadata;
  SaveSessionSection session;
  SaveWorldSection world;
  SavePlayerSection players;
  SaveClockSection clock;
  SaveCameraSection camera;
  SaveCommandLogSection commandLog;
  SaveInventorySection inventory;
  SaveCombatSection combat;
  SaveAiSection ai;
  SaveObjectiveSection objectives;
};
```

Field ownership is locked for the first complete build:

- `SaveSessionSection.packageId`, `scenarioId`, `sessionSeed`,
  `sessionSchemaVersion`, and `nextCommandId` map directly from
  `SessionState`.
- `SaveEnvelopeMetadata.packageId` must equal `SaveSessionSection.packageId`;
  mismatch returns `SaveLoadStatus::InvalidEnvelope` before load mutation.
- `SaveEnvelopeMetadata.scenarioId` must equal `SaveSessionSection.scenarioId`;
  mismatch returns `SaveLoadStatus::InvalidEnvelope` before load mutation.
- `SaveSessionSection.currentTick` must equal `SaveClockSection.tickIndex`;
  mismatch returns `SaveLoadStatus::InvalidEnvelope` before load mutation.
- `SaveSessionSection.nextCommandId` must be present, greater than zero, and
  greater than every restored `SaveCommandLogSection.records[*].commandId`.
  Save/load validates this cursor during the envelope decode/load path before a
  restored runtime session becomes usable. Any violation returns
  `SaveLoadStatus::InvalidEnvelope`.
- `SaveWorldSection.entities` is in `WorldState::entities()` storage order.
- `SaveEntityRecord` saves `localBounds` only; no separate world bounds are
  saved because the current `EntityState` owns `localBounds` and `transform`.
- `SaveEntityRecord` is the flattened save representation of durable
  `EntityState` fields, including canonical
  `runtime/interaction/InteractionDefinition.hpp` metadata.
- `SaveEntityRecord::interactionKind` maps from
  `EntityState::interaction.kind`.
- `SaveEntityRecord::interactionPrimaryEffect` maps from
  `EntityState::interaction.primaryEffect`.
- `SaveEntityRecord::interactionItemId` maps from
  `EntityState::interaction.itemId`.
- `SaveEntityRecord::interactionItemCount` maps from
  `EntityState::interaction.itemCount`.
- `SaveEntityRecord::interactionObjectiveId` maps from
  `EntityState::interaction.objectiveId`.
- `SaveEntityRecord::interactionRepeatable` maps from
  `EntityState::interaction.repeatable`.
- `SaveEntityRecord::interactionDeactivateTargetOnSuccess` maps from
  `EntityState::interaction.deactivateTargetOnSuccess`.
- `SavePlayerSection.slots` maps from `PlayerRoster::slots()` in deterministic
  slot order.
- save records use `kInvalidPlayerSlotId` from `runtime/player/PlayerSlot.hpp`
  for invalid player-slot defaults.
- `SaveClockSection` saves every `ClockState` field except `stepRequested`;
  load sets `ClockState::stepRequested=false`. `stepRequested` is transient
  command-consumption state and is not hash input.
- `SaveCameraSection` saves every durable `CameraState` field except
  `inputClearRequested`; load sets `inputClearRequested=false`.
- `SaveCommandLogSection` saves `records`, `nextSequence`, and `epoch` because
  `CommandLog` owns those fields. Command id allocation belongs to `Session`;
  the durable allocation cursor is `SaveSessionSection.nextCommandId`.
- `SaveCommandLogSection.resetPolicy` is a schema constant and must always be
  `CommandLogResetPolicy::Clear` in the first complete build; it is not mutable
  `CommandLog` state.
- Rejected commands remain in `SaveCommandLogSection.records` so retry and
  replay proof can restore `cmd_interact_oob` and `cmd_retry_key`.
- Inventory, combat, AI, and objective sections mirror their authoritative
  runtime vectors exactly in deterministic order.
- `savedStateHashHex` is required, must equal `formatStateHash(savedStateHash)`,
  and is compared by `SaveLoad` before session replacement.

Required exclusions:

- renderer/window/GPU resources;
- app paths, CLI args, process environment, wall-clock timestamps;
- raw input events and controller device ids;
- projection, debug projection, runtime summary, transient metrics/events;
- network sockets or platform handles.

Ordering must be stable inside every repeated section. Envelope values become
active runtime truth only after `SaveCompatibility` accepts the envelope and
`SaveLoad` applies all sections all-or-nothing through the session boundary.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(saved state size).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- this file owns the save payload shape and exact durable section fields;
- replay and save/load use the same command log and state hash metadata;
- multiplayer metadata is value-only and limited to saved player/authority fields
  already owned by runtime state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/save/SaveEnvelope.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
