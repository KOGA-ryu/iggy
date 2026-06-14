# Derived State Policy

Purpose: distinguish authoritative state from rebuildable derived data, and record save/rebuild expectations.

## Definitions

Authoritative state:
- required to continue gameplay truth
- eligible for save snapshots
- mutation must report changed facts explicitly

Derived state:
- rebuildable from authoritative state and config
- carried for performance or continuity
- excluded from save snapshots unless a later slice explicitly says otherwise

Transient frame data:
- produced for a frame/tick
- usually diagnostic or presentation-facing
- never authoritative by default

## Current Authoritative State

- `LevelRuntimeState`
- `LevelTileMap`
- `PlayerAgentState` when carried by `RuntimeSessionState`
- `RuntimeInventoryState` when carried by caller or a future authoritative session packet
- `InventoryState2D` and `LevelItemDrop2DRegistry` when carried through `RuntimeInventoryState`
- NPC agent state in level/runtime state
- `RuntimeSessionState::tickIndex`

## Current Derived State

- `LevelRenderCacheState`
- `LevelCollisionCacheState`
- `LevelDerivedCacheState`
- `RenderCommandList2D` frame packets
- `LevelRenderFrame2DResult`
- player command plans
- interaction plans
- interaction effect plans
- interaction effect application results
- interaction events emitted while applying effects
- pickup plans
- inventory events emitted while applying inventory/pickup effects
- NPC AI context scores, behavior intents, target selections, decisions, route requests, navigation requests, path reports, movement proposals, and command-frame mappings
- NPC tick reports
- collision query results
- mutation/cache update results and command tick diagnostics
- player input interaction reports
- gameplay frame reports

## Save Boundary

Save snapshots should include:

- level/session id or map reference when introduced
- authoritative tile/map state or blueprint reference
- tick index
- player state
- inventory state and item-drop state when included in a future save snapshot boundary
- NPC/actor state
- gameplay-relevant ids and resource ids

Save snapshots should exclude:

- `LevelRenderCacheState`
- `LevelCollisionCacheState`
- `LevelDerivedCacheState`
- render command lists
- collision query worlds if rebuildable from map data
- command plans and per-tick reports
- interaction plans, requested effect lists, application diagnostics, interaction events, and interaction reports
- pickup plans and pickup diagnostics
- inventory events and pickup frame diagnostics
- NPC AI decision, path, movement proposal, queue, and command-frame diagnostics unless a later slice promotes actor state into snapshots
- mutation/cache update diagnostics
- backend handles, windows, GPU resources
- raw device input state

## Runtime Carrying Rule

Runtime may carry derived caches in `RuntimeSessionState` when it improves orchestration, but runtime should not become the subsystem that defines how those caches are built.

Current pattern:

```text
scene/level builds or updates LevelDerivedCacheState
runtime carries LevelDerivedCacheState
runtime steps may select from it through explicit providers
runtime mutation steps may replace carried derived cache state by delegating to scene/level updaters
```

## Mutation Rule

Authoritative mutation should report changed facts. Derived cache refresh should consume those facts.

Current pattern:

```text
LevelTileMutation
  -> changedTiles
  -> LevelMutationCacheUpdateStep
       -> LevelDerivedCacheUpdater
```

Runtime should not infer changed tiles inside session ticks.

## Snapshot Rule

`RuntimeSessionSnapshot` captures authoritative session data only:

- `LevelRuntimeState`
- `tickIndex`
- optional `PlayerAgentState`

It intentionally excludes:

- `LevelDerivedCacheState`
- legacy runtime render-cache mirrors
- render frames
- command plans and reports
- backend/device state

Restoration should rebuild requested derived caches through `RuntimeSessionBuilder` configuration, not deserialize them as authoritative state.

## Save Codec Rule

The runtime save lane may own local persistence contracts for authoritative snapshots:

- `RuntimeBinaryWriter` / `RuntimeBinaryReader`
- `RuntimeSaveChunkArchive`
- snapshot-to-chunk codecs
- chunk-archive-to-byte-vector codecs
- `RuntimeSaveFileEnvelope`
- `RuntimeSaveFileIO`
- `RuntimeSaveSlotPathPolicy`
- `RuntimeSaveSlotStore`
- `RuntimeSaveSlotListing`
- `RuntimeSaveSlotDeletion`
- `RuntimeSessionSaver`
- `RuntimeSessionLoader`

These contracts may read and write local save files and manage explicit save slots. They still do not make derived caches authoritative or saveable.

Compression, encryption, cloud storage, platform-specific storage, cross-process locking, save UI, and broad retention/overwrite policy remain separate future boundaries.

## Interaction Rule

Interaction targets and effect catalogs are scene/interaction data. Interaction plans, effect plans, and runtime interaction reports are transient results:

```text
InteractionTarget2DRegistry + actor position + target id
  -> InteractionPlan2DResult
  -> InteractionEffectPlan2DResult
  -> runtime reports
```

Current effect application can return an updated `InteractionTarget2DRegistry` for target-toggle effects. It also records transient `InteractionEvent2D` values for target toggles, inspect-text requests, and emitted events. That returned registry is scene/interaction state, but runtime does not persist it automatically in `RuntimeSessionState`.

Effects that mutate `LevelRuntimeState`, `PlayerAgentState`, inventory, combat, quests, or global event streams still require a separate ownership slice.

`RuntimeInteractionState` may carry interaction targets/effects as an explicit caller-owned packet through runtime interaction steps. It is not currently part of authoritative session snapshots.

## Inventory Rule

Inventory stacks and level item drops are scene/inventory data. `PickupPlan2D` is a transient plan, while `PickupTransfer2D` returns updated inventory/drop state:

```text
LevelItemDrop2DRegistry + actor position + drop id
  -> PickupPlan2DResult
  -> PickupTransfer2DResult
       -> InventoryState2D
       -> LevelItemDrop2DRegistry
```

Runtime can carry those updates through explicit `RuntimeInventoryState` and `RuntimePickupStep` / `RuntimePickupEffectStep`. `RuntimeSessionState` does not currently own inventory/drop state, and save snapshot shape for inventory/drop state is not defined yet.

The catalog-aware path adds item definitions, stack policy, policy add, policy transfer, and inventory events:

```text
ItemDefinition2DCatalog + RuntimeInventoryState + PickupPlan2DResult
  -> PickupPolicyTransfer2DResult
       -> InventoryPolicyAddItem2DResult
       -> LevelItemDropConsume2DResult
       -> InventoryEventRecorder2D
```

`RuntimePolicyPickupStep`, `RuntimePolicyPickupEffectStep`, and `RuntimePolicyPickupEffectFrameStep` orchestrate that policy lane from runtime context. They still do not make item definitions, inventory/drop registries, or inventory events part of `RuntimeSessionState` by themselves.

`RuntimeGameplayState` can group session, command queue, interaction state, and inventory state for frame orchestration. Treat it as a runtime packet, not a save snapshot contract. A future save/session slice must explicitly decide which interaction and inventory fields become authoritative persisted state.

`RuntimePolicyGameplayFrameStep` and `RuntimePolicyGameplayFrameRunner` are catalog-aware runtime orchestration over `RuntimeGameplayState`. They can carry updated interaction and inventory state forward and accumulate transient inventory events, but they still do not make `ItemDefinition2DCatalog`, interaction state, inventory state, or item drops part of the current `RuntimeSessionSnapshot`.

## NPC AI Rule

NPC AI tactical maps, behavior presets, profiles, current AI state inputs, and actor registries are scene data. Decision outputs are transient reports:

```text
AiMap2D + NpcAiProfile2D + NpcAiCurrentState2D
  -> NpcAiDecision2DResult
  -> NpcAiRouteRequest2DResult
  -> NpcAiNavigationRequest2DResult
  -> NpcAiPathReport2DResult
  -> NpcAiMovementProposal2DResult
  -> NpcAiCommandFrameMapper2DResult
```

Runtime NPC AI queue steps can enqueue the resulting gameplay command frames, but they do not make AI reports authoritative. `NpcActorState2DRegistry` is a scene-owned actor-state candidate; its relationship to existing NPC agent state and save snapshots needs an explicit future boundary decision.
