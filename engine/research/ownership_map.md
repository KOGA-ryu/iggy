# Ownership Map

Purpose: compact reference for Planner Dex and Reviewer Dex when a build slice crosses subsystem boundaries. This file records current ownership, not desired long-term architecture.

## Core Rules

- Authoritative state is the smallest state needed to resume gameplay truth.
- Derived cache state is rebuildable from authoritative state and should be excluded from save snapshots.
- Runtime may carry derived state for continuity, but the source subsystem owns how it is built or refreshed.
- Explicit caller overrides beat session-carried derived caches.
- Builders and updaters should be pure data transforms unless a slice explicitly introduces mutation.

## Subsystems

### `core/resource`

Owns stable logical identity and asset metadata.

Current anchors:
- `ResourceId`
- `AssetCatalog`

Does not own:
- backend handles
- file loading/import
- scene state
- runtime sequencing

### `servers/render`

Owns backend-free render contracts, command composition, and render resource metadata.

Current anchors:
- `RenderCommand2D`
- `RenderCommandList2DComposer`
- `TextureResource`
- `ShaderResource`
- `MaterialResource`
- `RenderResourceRegistry`

Does not own:
- scene visibility policy
- tile or sprite semantics
- runtime frame order
- GPU/window/backend behavior

### `servers/physics2d`

Owns generic collision shapes, worlds, queries, and movement helpers.

Current anchors:
- `CollisionShape2D`
- `CollisionWorld2D`
- `CollisionOverlap2D`
- `CollisionMotionQuery2D`
- `CharacterMove2D`

Does not own:
- player state
- level tile interpretation
- runtime orchestration
- collision cache lifetime
- entity registries or physics stepping

### `scene/level`

Owns level data interpretation, tile mutation semantics, visibility, render-frame assembly, and level-scoped derived caches.

Current authoritative anchors:
- `LevelTileMap`
- `LevelRuntimeState`
- `LevelTileMutation`

Current derived cache anchors:
- `LevelRenderCacheState`
- `LevelCollisionCacheState`
- `LevelDerivedCacheState`

Current conversion anchors:
- `LevelCollisionWorldBuilder`
- `LevelMutationCacheUpdateStep`
- `LevelRenderFrame2D`
- tile render chunk builders/updaters

Does not own:
- runtime tick order
- player command orchestration
- physics query internals
- backend rendering
- save/load format

### `scene/player`

Owns player scene state, player command interpretation, and player-specific movement rules.

Current anchors:
- `PlayerAgentState`
- `PlayerCommandPlanner2D`
- `PlayerCommandFramePlanner2D`
- `PlayerMovementExecutor2D`
- `PlayerInputIntent2D`
- `PlayerInputIntentGate2D`
- `PlayerInputCommandMapper2D`
- `PlayerInputCommandFrameMapper2D`
- `PlayerInputGatedCommandFrameMapper2D`

Does not own:
- raw input binding
- runtime session tick order
- collision world construction
- NPC AI
- save/load format

### `scene/interaction`

Owns interaction target/effect data and scene-level interaction interpretation.

Current anchors:
- `InteractionTarget2D`
- `InteractionTarget2DRegistry`
- `InteractionTargetQuery2D`
- `InteractionReach2D`
- `InteractionPlan2D`
- `InteractionEffect2D`
- `InteractionEvent2D`
- `InteractionEventRecorder2D`
- `InteractionEffectCatalog2D`
- `InteractionEffectPlan2D`
- `InteractionTargetToggle2D`
- `InteractionEffectApplier2D`
- `InteractionEffectPlanApplier2D`

Does not own:
- runtime command-frame order
- player input queueing
- authoritative effect application to level/player state outside the interaction target registry
- inventory, combat, or quest semantics
- backend UI or rendering

### `modules/animation`

Owns animation clip data, playback state, and sampling.

Current anchors:
- `SpriteFrameSet2D`
- `SpriteAnimationState2D`
- `SpriteAnimationSampler2D`

Does not own:
- render command construction
- scene sprite placement
- runtime tick orchestration

### `scene/sprite`

Owns sprite visible-state to render-command transformation.

Current anchors:
- `SpriteRenderCommands2D`

Does not own:
- animation playback
- render backend resources
- runtime sequencing

### `runtime`

Owns session packets and deterministic orchestration.

Current anchors:
- `GameplayCommand2D`
- `GameplayCommandFrame2DValidator`
- `RuntimeSessionState`
- `RuntimeSessionBuilder`
- `RuntimeSessionTick`
- `RuntimeSessionTickRunner`
- `RuntimePlayerCommandPlanningStep`
- `RuntimePlayerCommandExecutionStep`
- `RuntimePlayerCommandStep`
- `RuntimeCommandQueue`
- `RuntimePlayerInputQueueStep`
- `RuntimeQueuedCommandRunner`
- `RuntimePlayerInputCommandRunner`
- `RuntimePlayerInputCommandReporter`
- `RuntimePlayerInputFrameStep`
- `RuntimeInteractionCommandStep`
- `RuntimeInteractionCommandFrameStep`
- `RuntimeInteractionEffectCommandStep`
- `RuntimeInteractionEffectCommandFrameStep`
- `RuntimeInteractionEffectApplyStep`
- `RuntimeInteractionEffectApplyFrameStep`
- `RuntimeInteractionState`
- `RuntimePlayerInputInteractionFrameStep`
- `RuntimePlayerInputInteractionFrameReporter`
- `RuntimePlayerInputInteractionEffectFrameStep`
- `RuntimePlayerInputInteractionEffectFrameReporter`
- `RuntimePlayerInputInteractionEffectApplyFrameStep`
- `RuntimeSessionCommandTick`
- `RuntimeSessionCommandTickRunner`
- `RuntimeLevelMutationStep`
- `RuntimeSessionMutationCommandStep`
- `RuntimeSessionMutationCommandRunner`
- `RuntimeSessionSnapshotBuilder`
- `RuntimeSessionSnapshotRestorer`
- `RuntimeSessionSnapshotValidator`
- `RuntimeSaveChunkArchive`
- `RuntimeSaveChunkArchiveBuilder`
- `RuntimeSaveChunkArchiveValidator`
- `RuntimeSessionSnapshotChunkEncoder`
- `RuntimeSessionSnapshotChunkDecoder`
- `RuntimeSaveChunkArchiveEncoder`
- `RuntimeSaveChunkArchiveDecoder`
- `RuntimeSaveFileEnvelope`
- `RuntimeSaveFileIO`
- `RuntimeSaveSlotPathPolicy`
- `RuntimeSaveSlotStore`
- `RuntimeSaveSlotListing`
- `RuntimeSaveSlotDeletion`
- `RuntimeSessionSaver`
- `RuntimeSessionLoader`
- `RuntimeCollisionWorldProvider`

Runtime may carry:
- `LevelRuntimeState`
- optional `PlayerAgentState`
- `LevelDerivedCacheState`
- explicit `RuntimeInteractionState` values passed through interaction/input steps

Runtime does not own:
- level tile mutation semantics
- cache rebuild policy
- raw device input
- backend renderer/window/GPU
- platform storage, cloud sync, compression, or encryption
- physics internals

## Current Precedence Rules

Collision world selection:

```text
explicit CollisionWorld2D override
  -> session.derivedCaches.collision.world
  -> empty CollisionWorld2D
```

Player command tick order:

```text
GameplayCommandFrame2D
  -> RuntimePlayerCommandPlanningStep
  -> RuntimePlayerCommandExecutionStep
  -> RuntimeSessionTick
```

Derived cache update ownership:

```text
LevelTileMutation result changedTiles
  -> LevelMutationCacheUpdateStep
       -> LevelDerivedCacheUpdater
  -> caller replaces carried level + derived cache state
```

Runtime mutation orchestration:

```text
RuntimeLevelMutationStep
  -> LevelMutationCacheUpdateStep
  -> updates RuntimeSessionState.level + RuntimeSessionState.derivedCaches
  -> mirrors legacy renderCache / hasRenderCache
```

Mutation-command tick order:

```text
RuntimeSessionMutationCommandStep
  -> RuntimeLevelMutationStep
  -> RuntimeSessionCommandTick
```

Runtime should not infer tile changes or rebuild caches inside existing tick steps. Tile edits are explicit inputs to mutation steps.

Interaction orchestration:

```text
GameplayCommandFrame2D
  -> RuntimeInteractionCommandFrameStep
       -> InteractionPlan2D
  -> RuntimeInteractionEffectCommandFrameStep
       -> InteractionEffectPlan2D
```

Runtime may compose command frames with scene/interaction targets and effect catalogs, but scene/interaction owns target lookup, reach, ready/blocked status, and requested effect-plan semantics. Runtime does not apply effects to authoritative level/player state yet.

Interaction effect application:

```text
RuntimeInteractionEffectApplyStep
  -> RuntimeInteractionEffectCommandStep
  -> InteractionEffectPlanApplier2D
       -> InteractionEffectApplier2D
       -> InteractionTargetToggle2D
  -> returns updated InteractionTarget2DRegistry

RuntimeInteractionEffectApplyFrameStep
  -> applies Interact commands sequentially
  -> carries the updated registry from one command to the next
```

The current application boundary is limited to returning an updated interaction target registry plus transient interaction events. `RuntimeInteractionState` can carry targets/effects through explicit runtime input steps, but it is not stored in `RuntimeSessionState`. This path does not mutate `LevelRuntimeState` or execute inventory/combat/quest behavior.

Save snapshot ownership:

```text
RuntimeSessionSnapshot
  includes authoritative level, tickIndex, optional player
  excludes LevelDerivedCacheState and render/collision caches
  -> RuntimeSessionSnapshotValidator
  -> RuntimeSessionSnapshotChunkEncoder / Decoder
  -> RuntimeSaveChunkArchiveEncoder / Decoder
  -> RuntimeSaveFileEnvelope
  -> RuntimeSaveFileIO / RuntimeSaveSlotStore
```

The save lane currently owns local save persistence: authoritative snapshot validation, chunk archive structure, byte-vector codecs, file envelope/checksum wrapping, local file read/write helpers, slot path policy, slot store/load, slot listing, slot deletion, and session save/load composition.

It still does not own platform-specific storage, cloud saves, compression, encryption, cross-process locking, user-facing save UI, or broad retention policy unless a future slice scopes those explicitly.
