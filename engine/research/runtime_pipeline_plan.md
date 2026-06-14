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

Scene NPC AI interpretation:

```text
AiMap2D
AiMapQuery2D
NpcAiBehaviorPool
NpcTraitSet
NpcStrengthPool / NpcStrengthDraw
NpcDexterityPool / NpcDexterityDraw
NpcConstitutionPool / NpcConstitutionDraw
NpcIntelligencePool / NpcIntelligenceDraw
NpcWisdomPool / NpcWisdomDraw
NpcCharismaPool / NpcCharismaDraw
NpcHand
NpcRead
NpcPlay
NpcPlayControlProposal
NpcTell
NpcFold
NpcObjective
NpcAiProfile2D
NpcAiCurrentState2D
NpcAiDecision2D
NpcAiRouteRequest2D
NpcAiNavigationRequest2D
NpcAiPathReport2D
NpcAiMovementProposal2D
NpcAiCommandFrameMapper2D
```

NPC actor state:

```text
NpcActorState2DRegistry
NpcActorControlState2DRegistry
NpcPlayControlFrameApply2D
NpcPlayControlFrameReport2D
NpcActorFrameState2D
  -> NpcAiCurrentState2D / RuntimeNpcAiDecisionQueueNpcInput
```

Scene interaction interpretation:

```text
InteractionTarget2DRegistry
InteractionTargetQuery2D
InteractionReach2D
InteractionPlan2D
InteractionEffectCatalog2D
InteractionEffectPlan2D
InteractionEvent2D
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

Runtime NPC AI queue orchestration:

```text
RuntimeNpcAiDecisionQueueStep
  -> scene/ai decision pipeline
  -> RuntimeNpcAiMovementQueueStep
       -> RuntimeNpcAiCommandQueueStep
       -> RuntimeCommandQueue

RuntimePlayerNpcAiQueueStep
  -> RuntimePlayerInputQueueStep
  -> RuntimeNpcAiDecisionQueueStep

RuntimePlayerNpcAiCommandFrameStep
  -> RuntimePlayerNpcAiQueueStep
  -> RuntimeQueuedCommandRunner
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
RuntimeInteractionEffectApplyFrameStep
RuntimeInteractionState
```

Runtime player-input interaction frames:

```text
RuntimePlayerInputInteractionFrameStep
RuntimePlayerInputInteractionFrameReporter
RuntimePlayerInputInteractionEffectFrameStep
RuntimePlayerInputInteractionEffectFrameReporter
RuntimePlayerInputInteractionEffectApplyFrameStep
```

Inventory and pickup:

```text
ItemDefinition2D
ItemDefinition2DCatalog
InventoryState2D
LevelItemDrop2DRegistry
InventoryEvent2D
PickupPlan2D
InventoryAddItem2D
LevelItemDropConsume2D
PickupTransfer2D
InventoryStackPolicy2D
InventoryPolicyAddItem2D
PickupPolicyTransfer2D
RuntimeInventoryState
RuntimePickupStep
RuntimePickupEffectStep
RuntimePickupEffectFrameStep
RuntimePolicyPickupStep
RuntimePolicyPickupEffectStep
RuntimePolicyPickupEffectFrameStep
RuntimePlayerInputInteractionPickupFrameStep
RuntimePlayerInputInteractionPickupFrameReporter
RuntimePlayerInputInteractionPolicyPickupFrameStep
```

Gameplay frame orchestration:

```text
RuntimeGameplayState
  -> RuntimeGameplayFrameStep
       -> RuntimePlayerInputInteractionPickupFrameStep
       -> RuntimeGameplayFrameReporter
  -> RuntimeGameplayFrameRunner

RuntimeGameplayState + ItemDefinition2DCatalog
  -> RuntimePolicyGameplayFrameStep
       -> RuntimePlayerInputInteractionPolicyPickupFrameStep
       -> RuntimePolicyGameplayFrameReporter
  -> RuntimePolicyGameplayFrameRunner
```

UI models and actions:

```text
UiFeatureContext
UiToolInventory
UiShellModel
UiToolIntent
UiRuntimeFrameInspectorModel
UiInteractionEventPanelModel
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

Draft building collision compile:

```text
DraftDocument2D
  -> DraftBuildingCompiler2D
  -> DraftLevelGeometryPlanner2D
  -> LevelCollisionSource2DBuilder
  -> LevelCollisionSourceWorldBuilder2D
  -> LevelCombinedDerivedCacheBuilder2D
```

`DraftDocument2D` is authored intent. `LevelCollisionSource2D` is derived
compile output and source input for collision cache/world construction. It is
not the authored source of truth for level layout and is not owned by
`RuntimeSessionState`. Custom collision should be authored as draft/editor
symbols and compiled into `LevelCollisionSource2D`; serializing compiled
collision with provenance is future export/build-artifact work.

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

NPC AI queue variant:

```text
input RuntimeCommandQueueState + AiMap2D + LevelTileMap + NPC AI inputs
  -> RuntimeNpcAiDecisionQueueStep
       -> NpcAiDecision2D
       -> NpcAiRouteRequest2D
       -> NpcAiNavigationRequest2D
       -> NpcAiPathReport2D
       -> NpcAiMovementProposal2D
       -> NpcAiCommandFrameMapper2D
       -> RuntimeCommandQueue
```

Player + NPC AI queue variant:

```text
RuntimePlayerNpcAiQueueStep
  -> RuntimePlayerInputQueueStep
  -> RuntimeNpcAiDecisionQueueStep
  -> explicit order: PlayerThenNpcAi or NpcAiThenPlayer

RuntimePlayerNpcAiCommandFrameStep
  -> RuntimePlayerNpcAiQueueStep
  -> RuntimeQueuedCommandRunner
```

Runtime owns queue order and command-runner composition. `scene/ai` remains the source of truth for tactical map lookup, scoring, intent, route/path reports, movement proposals, and command-frame mapping.

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
       -> records ToggleTarget, InspectText, and EmitEvent interaction events

RuntimeInteractionEffectApplyFrameStep
  -> applies Interact commands sequentially
  -> feeds each updated registry into the next command application
```

This does not replace session state or own target registry lifetime. A caller must explicitly decide whether the returned registry becomes the next interaction target source.

Player-input apply variant:

```text
RuntimePlayerInputInteractionEffectApplyFrameStep
  -> RuntimePlayerInputFrameStep::runGated
  -> RuntimeInteractionEffectApplyFrameStep
  -> returns updated RuntimeInteractionState or explicit interaction target registry
```

This keeps interaction state explicit. It does not store interaction targets/effects inside `RuntimeSessionState`.

Pickup effect variant:

```text
InteractionEffect2D::PickupItem
  -> RuntimePickupEffectStep
       -> RuntimePickupStep
            -> PickupPlan2D
            -> PickupTransfer2D
  -> updated RuntimeInventoryState
```

This keeps inventory/drop state explicit. It does not store inventory or item drops inside `RuntimeSessionState`.

Policy pickup effect variant:

```text
InteractionEffectPlanApplyResult
  -> RuntimePolicyPickupEffectFrameStep
       -> RuntimePolicyPickupEffectStep
            -> RuntimePolicyPickupStep
                 -> PickupPolicyTransfer2D
                      -> InventoryPolicyAddItem2D
                      -> LevelItemDropConsume2D
            -> InventoryEventRecorder2D
  -> updated RuntimeInventoryState
```

This is the catalog-aware pickup path. Item definition lookup, stack-cap enforcement, inventory add events, pickup events, and drop-consume events remain `scene/inventory` behavior. Runtime only sequences policy pickup effects across applied interaction effects.

Policy player-input pickup variant:

```text
RuntimePlayerInputInteractionPolicyPickupFrameStep
  -> RuntimePlayerInputInteractionEffectApplyFrameStep
       -> gated player input
       -> command queue / command ticks
       -> interaction effect application
  -> RuntimePolicyPickupEffectFrameStep
       -> catalog-aware pickup effects
  -> updated RuntimeSessionState
  -> updated RuntimeCommandQueueState
  -> updated RuntimeInteractionState
  -> updated RuntimeInventoryState
```

This is the policy-aware replacement candidate for the simple `RuntimePlayerInputInteractionPickupFrameStep`. It still keeps item definitions as an explicit input and keeps interaction/inventory state outside `RuntimeSessionState`.

Gameplay frame variant:

```text
RuntimeGameplayState
  -> RuntimeGameplayFrameStep
       -> RuntimePlayerInputInteractionPickupFrameStep
            -> RuntimePlayerInputInteractionEffectApplyFrameStep
            -> RuntimePickupEffectFrameStep
       -> RuntimeGameplayFrameReport
  -> updated RuntimeGameplayState
```

`RuntimeGameplayState` groups session, command queue, interaction state, and inventory state for frame orchestration. It does not make interaction or inventory fields part of `RuntimeSessionState`, and it does not change the save snapshot boundary by itself.

Policy gameplay frame variant:

```text
RuntimeGameplayState + ItemDefinition2DCatalog
  -> RuntimePolicyGameplayFrameStep
       -> RuntimePlayerInputInteractionPolicyPickupFrameStep
            -> RuntimePlayerInputInteractionEffectApplyFrameStep
            -> RuntimePolicyPickupEffectFrameStep
       -> RuntimePolicyGameplayFrameReport
  -> updated RuntimeGameplayState

RuntimePolicyGameplayFrameRunner
  -> bounded loop over RuntimePolicyGameplayFrameStep
  -> accumulates InventoryEventRecorder2D
```

This is the catalog-aware gameplay frame lane. It should be used when pickup stack limits and item definition diagnostics matter. It does not remove the simple gameplay frame lane yet.

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
- interaction target registry lifetime/persistence beyond explicit `RuntimeInteractionState` values
- interaction effects that mutate level/player/inventory/event state

## Likely Next Integration Choices

Possible future slices:

- render-frame step that reads session carried render cache
- camera/presentation state packet
- save slot retention/overwrite policy if caller needs more than explicit slot store/list/delete calls
- decide whether `RuntimeInteractionState` should remain a caller-owned sibling packet or become part of a broader session/presentation packet
- decide whether `RuntimeGameplayState` is the long-term top-level runtime packet and how its interaction/inventory fields relate to authoritative save snapshots
- decide whether policy gameplay frames should replace or coexist with the simple gameplay frame lane
- decide how `NpcActorState2DRegistry` relates to existing `modules/npc_ai::NpcAgentState` and future save snapshots before runtime owns NPC actor lifetime

Pause before any slice that makes runtime infer map changes or own cache rebuild policy.
