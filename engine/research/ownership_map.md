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
- `LevelCollisionSource2D`
- `LevelCollisionSourceWorldBuilder2D`
- `LevelCollisionWorldMerge2D`
- `LevelCombinedDerivedCacheBuilder2D`
- `LevelMutationCacheUpdateStep`
- `LevelRenderFrame2D`
- tile render chunk builders/updaters

`LevelCollisionSource2D` is derived compile output. It is the source input
for derived collision cache/world construction, not the authored source of
truth for level layout and not runtime/session-owned state. Project/editor
truth remains draft symbols for now. Future export/build artifacts may
serialize compiled collision with provenance, but custom collision should be
authored as draft/editor symbols and compiled into `LevelCollisionSource2D`,
not hand-edited as compiled boxes.

Does not own:
- runtime tick order
- player command orchestration
- physics query internals
- backend rendering
- save/load format

### `scene/draft`

Owns authored semantic draft data and auditable draft compile reports.

Current anchors:
- `DraftDocument2D`
- `DraftCompilePlanner2D`
- `DraftWallCompiler2D`
- `DraftDoorCompiler2D`
- `DraftWallDoorAttach2D`
- `DraftWallDoorMorphPlanner2D`
- `DraftWallCutCompiler2D`
- `DraftBuildingCompiler2D`
- `DraftLevelGeometryPlanner2D`

`DraftDocument2D` is authored intent and the current project/editor source of
truth. Draft compile results are derived reports that can feed scene/level
builders explicitly; they do not become playable runtime state by themselves.

Does not own:
- runtime/session state
- save/load or export persistence
- physics collision world semantics
- backend UI, canvas, or rendering
- item, NPC, or interaction gameplay execution

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

### `scene/ai`

Owns map tactical AI context, abstract NPC trait/action vocabulary, NPC AI temperament/state inputs, auditable NPC AI decision reports, map-aware read/play reports, Play-control proposal projection, route/path/movement proposal transforms, and NPC AI command-frame mapping.

Current anchors:
- `AiMap2D`
- `AiMapQuery2D`
- `NpcAiBehaviorPool`
- `NpcTraitSet`
- `NpcStrengthPool`
- `NpcStrengthDraw`
- `NpcDexterityPool`
- `NpcDexterityDraw`
- `NpcConstitutionPool`
- `NpcConstitutionDraw`
- `NpcIntelligencePool`
- `NpcIntelligenceDraw`
- `NpcWisdomPool`
- `NpcWisdomDraw`
- `NpcCharismaPool`
- `NpcCharismaDraw`
- `NpcHand`
- `NpcRead`
- `NpcMapRead`
- `NpcMapReadProjection`
- `NpcMapPlayReport`
- `NpcPlay`
- `NpcPlayControlProposal`
- `NpcTell`
- `NpcFold`
- `NpcObjective`
- `NpcAiProfile2D`
- `NpcAiCurrentState2D`
- `NpcAiContextScore2D`
- `NpcAiBehaviorIntent2D`
- `NpcAiIntentTarget2D`
- `NpcAiDecision2D`
- `NpcAiRouteRequest2D`
- `NpcAiNavigationRequest2D`
- `NpcAiPathReport2D`
- `NpcAiMovementProposal2D`
- `NpcAiMovementCommandMapper2D`
- `NpcAiCommandFrameMapper2D`

Does not own:
- runtime command queue lifetime
- session tick order
- player input intake
- NPC movement execution
- physics collision solving
- save/load format
- user-facing semantic name approval for new major AI layers

### `scene/npc`

Owns scene-level NPC actor identity, profile/current-goal state, active behavior/control state, movement mode, Play-control application/reporting over actor control registries, and per-frame actor/control projections that can feed AI decisions.

Current anchors:
- `NpcActorState2D`
- `NpcActorState2DRegistry`
- `NpcActorState2DRegistryBuilder`
- `NpcBehaviorState`
- `NpcMoveMode`
- `NpcActorControlState2D`
- `NpcActorControlState2DRegistry`
- `NpcActorControlState2DRegistryBuilder`
- `NpcPlayControlApply2D`
- `NpcPlayControlFrameApply2D`
- `NpcPlayControlFrameReport2D`
- `NpcActorFrameState2D`
- `NpcActorFrameStateProjector2D`

Does not own:
- tactical AI scoring or path planning
- runtime command queue order
- player command execution
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

### `scene/inventory`

Owns item definitions, inventory stack data, level item-drop data, pickup planning, pickup transfer semantics, and inventory/pickup events.

Current anchors:
- `InventoryState2D`
- `InventoryState2DBuilder`
- `InventoryAddItem2D`
- `ItemDefinition2D`
- `ItemDefinition2DCatalog`
- `InventoryEvent2D`
- `InventoryEventRecorder2D`
- `InventoryStackPolicy2D`
- `InventoryPolicyAddItem2D`
- `LevelItemDrop2D`
- `LevelItemDrop2DRegistry`
- `LevelItemDrop2DRegistryBuilder`
- `LevelItemDropConsume2D`
- `PickupPlan2D`
- `PickupTransfer2D`
- `PickupPolicyTransfer2D`

Does not own:
- runtime command-frame order
- raw input mapping
- save slot/file format
- item assets or backend loading
- inventory UI
- interaction effect semantics outside pickup-specific planning

### `scene/ui`

Owns UI model data, tool/layout state, and UI intent-to-action planning.

Current anchors:
- `UiToolBeltState`
- `UiShellModel`
- `UiFeatureContext`
- `UiToolInventory`
- `UiToolIntent`
- `UiSettingsState`
- `UiRuntimeFrameInspectorModel`
- `UiInteractionEventPanelModel`

Does not own:
- runtime session mutation
- command execution
- save-file format or persistence
- platform/window/input-device bindings
- rendering backend

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
- `RuntimeInventoryState`
- `RuntimePickupStep`
- `RuntimePickupEffectStep`
- `RuntimePickupEffectFrameStep`
- `RuntimePolicyPickupStep`
- `RuntimePolicyPickupEffectStep`
- `RuntimePolicyPickupEffectFrameStep`
- `RuntimePlayerInputInteractionPickupFrameStep`
- `RuntimePlayerInputInteractionPickupFrameReporter`
- `RuntimePlayerInputInteractionPolicyPickupFrameStep`
- `RuntimeGameplayState`
- `RuntimeGameplayFrameStep`
- `RuntimeGameplayFrameRunner`
- `RuntimeGameplayFrameReporter`
- `RuntimePolicyGameplayFrameStep`
- `RuntimePolicyGameplayFrameRunner`
- `RuntimePolicyGameplayFrameReporter`
- `RuntimeNpcAiCommandQueueStep`
- `RuntimeNpcAiMovementQueueStep`
- `RuntimeNpcAiDecisionQueueStep`
- `RuntimePlayerNpcAiQueueStep`
- `RuntimePlayerNpcAiCommandFrameStep`
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
- explicit `RuntimeInventoryState` values passed through pickup steps
- explicit `RuntimeGameplayState` values that group session, command queue, interaction state, and inventory state for gameplay-frame orchestration

Runtime does not own:
- level tile mutation semantics
- NPC AI decision/scoring semantics
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

Pickup orchestration:

```text
RuntimePickupEffectStep
  -> RuntimePickupStep
       -> PickupPlan2D
       -> PickupTransfer2D
            -> InventoryAddItem2D
            -> LevelItemDropConsume2D
  -> returns updated RuntimeInventoryState
```

Runtime can orchestrate pickup from a session player and explicit `RuntimeInventoryState`, but `scene/inventory` owns the inventory/drop mutation semantics. `RuntimeInventoryState` is not stored in `RuntimeSessionState` yet.

Policy pickup orchestration:

```text
RuntimePolicyPickupEffectFrameStep
  -> RuntimePolicyPickupEffectStep
       -> RuntimePolicyPickupStep
            -> PickupPlan2D
            -> PickupPolicyTransfer2D
                 -> InventoryPolicyAddItem2D
                      -> InventoryStackPolicy2D
                 -> LevelItemDropConsume2D
            -> InventoryEventRecorder2D
  -> returns updated RuntimeInventoryState
```

The policy path is the catalog-aware pickup lane. Runtime supplies session/player context, explicit inventory/drop state, and an item definition catalog, then delegates item stack limits, add policy, drop consumption, and inventory events to `scene/inventory`.

Policy gameplay orchestration:

```text
RuntimePolicyGameplayFrameStep
  -> RuntimePlayerInputInteractionPolicyPickupFrameStep
       -> RuntimePlayerInputInteractionEffectApplyFrameStep
       -> RuntimePolicyPickupEffectFrameStep
  -> RuntimePolicyGameplayFrameReporter
  -> returns updated RuntimeGameplayState
```

This path is the catalog-aware gameplay frame lane. It groups session, command queue, interaction state, and inventory state in `RuntimeGameplayState`, but it does not make interaction or inventory fields part of `RuntimeSessionState` or the save snapshot contract by itself.

NPC AI queue orchestration:

```text
RuntimeNpcAiDecisionQueueStep
  -> NpcAiDecision2D
  -> NpcAiRouteRequest2D
  -> NpcAiNavigationRequest2D
  -> NpcAiPathReport2D
  -> NpcAiMovementProposal2D
  -> NpcAiCommandFrameMapper2D
  -> RuntimeCommandQueue

RuntimePlayerNpcAiQueueStep
  -> RuntimePlayerInputQueueStep
  -> RuntimeNpcAiDecisionQueueStep
```

Runtime chooses intake order and queue behavior. `scene/ai` owns scoring, target selection, route/path/movement-proposal semantics, and command-frame mapping. NPC AI queue steps should not mutate `RuntimeSessionState` or make NPC AI decisions authoritative save data by themselves.

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

### `apps/qt_shell`

Owns desktop Qt host projection: window chrome, activity rail, panels, tool belt, palette, settings pages, status bar, and Qt widget/style construction.

Current anchors:
- `IggyQtShellWindow`
- `IggyQtShellUi`

Does not own:
- scene/ui model semantics
- runtime command/session mutation
- backend renderer/GPU ownership
- save/load semantics
- raw input-device contracts beyond Qt widget events
