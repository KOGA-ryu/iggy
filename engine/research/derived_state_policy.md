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
- NPC agent state in level/runtime state
- `RuntimeSessionState::tickIndex`

## Current Derived State

- `LevelRenderCacheState`
- `LevelCollisionCacheState`
- `LevelDerivedCacheState`
- `RenderCommandList2D` frame packets
- `LevelRenderFrame2DResult`
- player command plans
- NPC tick reports
- collision query results
- mutation/cache update results and command tick diagnostics

## Save Boundary

Save snapshots should include:

- level/session id or map reference when introduced
- authoritative tile/map state or blueprint reference
- tick index
- player state
- NPC/actor state
- gameplay-relevant ids and resource ids

Save snapshots should exclude:

- `LevelRenderCacheState`
- `LevelCollisionCacheState`
- `LevelDerivedCacheState`
- render command lists
- collision query worlds if rebuildable from map data
- command plans and per-tick reports
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

The runtime save lane may own in-memory serialization contracts:

- `RuntimeBinaryWriter` / `RuntimeBinaryReader`
- `RuntimeSaveChunkArchive`
- snapshot-to-chunk codecs
- chunk-archive-to-byte-vector codecs

These are still not disk persistence. File paths, save slots, atomic writes, compression, encryption, cloud storage, and platform-specific storage remain separate future boundaries.
