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
- `InventoryState2D` when carried by a future authoritative state packet
- `LevelItemDrop2DRegistry` when carried by a future authoritative level/session state packet
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
- NPC tick reports
- collision query results
- mutation/cache update results and command tick diagnostics
- player input interaction reports

## Save Boundary

Save snapshots should include:

- level/session id or map reference when introduced
- authoritative tile/map state or blueprint reference
- tick index
- player state
- inventory state and item-drop state once a carrying state packet is defined
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

Inventory stacks and level item drops are scene/inventory data. `PickupPlan2D` is a transient plan:

```text
LevelItemDrop2DRegistry + actor position + drop id
  -> PickupPlan2DResult
```

The current pickup boundary does not remove drops, add stacks to `InventoryState2D`, mutate `RuntimeSessionState`, or define save snapshot shape for inventory/drop state. Those ownership choices require a later slice.
