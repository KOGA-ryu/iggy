# Runtime Pipeline Plan

Purpose: current runtime sequencing reference for planner handoffs. This is not a promise to integrate every step immediately.

## Current Layers

Command data:

```text
GameplayCommand2D
GameplayCommandFrame2D
GameplayCommandFrame2DValidator
```

Player interpretation:

```text
PlayerAgentState
PlayerCommandPlanner2D
PlayerCommandFramePlanner2D
PlayerInputIntentGate2D
PlayerInputCommandFrameMapper2D
PlayerInputGatedCommandFrameMapper2D
```

Player execution:

```text
PlayerMovementExecutor2D
  -> physics2d::CharacterMove2D
  -> physics2d::CollisionWorld2D
```

Runtime player command orchestration:

```text
RuntimePlayerCommandPlanningStep
RuntimePlayerCommandExecutionStep
RuntimePlayerCommandStep
```

Scene interaction interpretation:

```text
InteractionTarget2DRegistry
InteractionTargetQuery2D
InteractionReach2D
InteractionPlan2D
InteractionEffectCatalog2D
InteractionEffectPlan2D
InteractionTargetToggle2D
InteractionEffectApplier2D
InteractionEffectPlanApplier2D
```

Runtime input-command orchestration:

```text
PlayerInputIntent2D list
  -> RuntimePlayerInputQueueStep
       -> PlayerInputCommandFrameMapper2D
       -> or PlayerInputGatedCommandFrameMapper2D with PlayerInputContext2D
       -> RuntimeCommandQueue
  -> RuntimeQueuedCommandRunner
       -> RuntimeSessionCommandTickRunner
  -> RuntimePlayerInputCommandReporter
  -> RuntimePlayerInputFrameStep
```

Runtime level mutation orchestration:

```text
RuntimeLevelMutationStep
RuntimeSessionMutationCommandStep
RuntimeSessionMutationCommandRunner
```

Runtime interaction orchestration:

```text
RuntimeInteractionCommandStep
RuntimeInteractionCommandFrameStep
RuntimeInteractionEffectCommandStep
RuntimeInteractionEffectCommandFrameStep
RuntimeInteractionEffectApplyStep
```

Runtime player-input interaction frames:

```text
RuntimePlayerInputInteractionFrameStep
RuntimePlayerInputInteractionFrameReporter
RuntimePlayerInputInteractionEffectFrameStep
RuntimePlayerInputInteractionEffectFrameReporter
```

Session ticking:

```text
RuntimeSessionTick
RuntimeSessionTickRunner
RuntimeSessionCommandTick
RuntimeSessionCommandTickRunner
```

Collision world selection:

```text
RuntimeCollisionWorldProvider
  explicit override
  -> session.derivedCaches.collision.world
  -> empty world
```

Derived cache ownership:

```text
LevelRenderCacheState
LevelCollisionCacheState
LevelDerivedCacheState
RuntimeSessionState carries LevelDerivedCacheState
```

Tile mutation and cache refresh:

```text
LevelTileMutation
  -> changed TileCoord list
  -> LevelMutationCacheUpdateStep
       -> LevelDerivedCacheUpdater
  -> caller replaces carried level + derived cache state
```

Snapshot flow:

```text
RuntimeSessionSnapshotBuilder
  -> authoritative level + tickIndex + optional player
  -> excludes derived caches

RuntimeSessionSnapshotValidator
  -> validates authoritative snapshot consistency

RuntimeSessionSnapshotChunkEncoder
  -> RuntimeSaveChunkArchive
  -> RuntimeSaveChunkArchiveEncoder
  -> RuntimeSaveFileEnvelope
  -> RuntimeSaveFileIO / RuntimeSaveSlotStore

RuntimeSaveFileIO / RuntimeSaveSlotStore
  -> RuntimeSaveFileEnvelope
  -> RuntimeSaveChunkArchiveDecoder
  -> RuntimeSessionSnapshotChunkDecoder
  -> RuntimeSessionSnapshotValidator

RuntimeSessionSnapshotRestorer
  -> RuntimeSessionBuilder
  -> rebuilds requested caches from restore config
```

Save slot management:

```text
RuntimeSaveSlotPathPolicy
  -> RuntimeSaveSlotStore
  -> RuntimeSaveSlotListing
  -> RuntimeSaveSlotDeletion
```

## Current Runtime Command Tick Order

```text
input RuntimeSessionState + GameplayCommandFrame2D
  -> RuntimePlayerCommandStep
       -> validate command frame
       -> plan player commands
       -> execute first MoveToPoint only
  -> choose NPC target position
       -> post-command player position if present
       -> fallbackPlayerPosition otherwise
  -> RuntimeSessionTick
       -> existing level/NPC tick
       -> tickIndex increments once
```

## Current Runtime Input Frame Order

```text
input RuntimeSessionState + RuntimeCommandQueueState + PlayerInputIntent2D list
  -> RuntimePlayerInputQueueStep
       -> maps intents into one GameplayCommandFrame2D
       -> appends mapped frame to RuntimeCommandQueueState
  -> RuntimeQueuedCommandRunner
       -> drains queued command frames
       -> runs RuntimeSessionCommandTickRunner
  -> RuntimePlayerInputCommandReporter
       -> summarizes intake, queue, runner, and tick diagnostics
```

Gated variant:

```text
input RuntimeSessionState + RuntimeCommandQueueState + PlayerInputContext2D + PlayerInputIntent2D list
  -> RuntimePlayerInputQueueStep::pushGated
       -> PlayerInputGatedCommandFrameMapper2D
       -> gate issues for blocked/invalid intents
       -> nested mapping issues for unblocked but unsupported intents
       -> appends accepted command frame to RuntimeCommandQueueState
  -> RuntimeQueuedCommandRunner
  -> RuntimePlayerInputCommandReporter::reportGated
```

Interaction variant:

```text
input RuntimeSessionState + RuntimeCommandQueueState + PlayerInputContext2D + PlayerInputIntent2D list
  -> RuntimePlayerInputFrameStep::runGated
       -> maps/gates input and runs queued command ticks
  -> RuntimeInteractionCommandFrameStep
       -> evaluates accepted Interact commands against InteractionTarget2DRegistry
  -> RuntimePlayerInputInteractionFrameReporter
       -> summarizes player input + interaction ready/blocked diagnostics
```

Effect-plan variant:

```text
RuntimePlayerInputFrameStep::runGated
  -> RuntimeInteractionEffectCommandFrameStep
       -> RuntimeInteractionCommandStep
       -> InteractionEffectPlan2D
  -> RuntimePlayerInputInteractionEffectFrameReporter
       -> reports requested effects without applying them
```

Effect-application variant:

```text
RuntimeInteractionEffectApplyStep
  -> RuntimeInteractionEffectCommandStep
       -> InteractionPlan2D
       -> InteractionEffectPlan2D
  -> InteractionEffectPlanApplier2D
       -> applies ToggleTarget to a returned InteractionTarget2DRegistry
       -> defers InspectText and EmitEvent as reported entries
```

This does not replace session state or own target registry lifetime. A caller must explicitly decide whether the returned registry becomes the next interaction target source.

## Current Runtime Mutation Command Tick Order

```text
input RuntimeSessionState + level edits + GameplayCommandFrame2D
  -> RuntimeLevelMutationStep
       -> LevelMutationCacheUpdateStep
       -> updates session level + derived caches
       -> mirrors legacy render cache fields
  -> RuntimeSessionCommandTick
       -> selects collision world using provider or explicit override
       -> runs player command step
       -> runs RuntimeSessionTick
```

## Caller-Owned Boundaries

The caller still owns:

- raw input/device mapping
- command frame creation when bypassing the input-intent queue path
- player input intent creation
- why a tile mutates
- when tile edits are applied
- when derived caches are refreshed or when the explicit mutation/cache step is called
- when save snapshots are captured/restored
- which save slot is read, written, listed, or deleted
- presentation camera state
- render-frame request timing

## Do Not Merge Yet

Do not merge these into existing ticks without a dedicated ownership review:

- render frame building
- presentation camera update
- platform storage, compression, encryption, cloud sync, or save UI policy
- raw device input
- interaction target registry lifetime/persistence
- interaction effects that mutate level/player/inventory/event state

## Likely Next Integration Choices

Possible future slices:

- render-frame step that reads session carried render cache
- camera/presentation state packet
- save slot retention/overwrite policy if caller needs more than explicit slot store/list/delete calls
- interaction target registry carrier/update step if callers need interaction target state to persist across frames

Pause before any slice that makes runtime infer map changes or own cache rebuild policy.
