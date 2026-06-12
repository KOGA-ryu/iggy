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
- backend handles, windows, GPU resources
- raw device input state

## Runtime Carrying Rule

Runtime may carry derived caches in `RuntimeSessionState` when it improves orchestration, but runtime should not become the subsystem that defines how those caches are built.

Current pattern:

```text
scene/level builds or updates LevelDerivedCacheState
runtime carries LevelDerivedCacheState
runtime steps may select from it through explicit providers
```

## Mutation Rule

Authoritative mutation should report changed facts. Derived cache refresh should consume those facts.

Current pattern:

```text
LevelTileMutation
  -> changedTiles
  -> LevelDerivedCacheUpdater
```

Runtime should not infer changed tiles inside session ticks.

