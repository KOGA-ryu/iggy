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

## Caller-Owned Boundaries

The caller still owns:

- raw input/device mapping
- command frame creation
- why a tile mutates
- when tile edits are applied
- when derived caches are refreshed or when the explicit mutation/cache step is called
- save/load snapshot creation
- presentation camera state
- render-frame request timing

## Do Not Merge Yet

Do not merge these into existing ticks without a dedicated ownership review:

- level tile mutation
- derived cache refresh
- render frame building
- presentation camera update
- save snapshot creation
- raw device input
- command queue buffering

## Likely Next Integration Choices

Possible future slices:

- runtime step that composes `LevelTileMutation` plus `LevelDerivedCacheUpdater`
- runtime adapter that delegates explicit tile edits to `LevelMutationCacheUpdateStep`
- save snapshot that excludes `LevelDerivedCacheState`
- render-frame step that reads session carried render cache
- camera/presentation state packet

Pause before any slice that makes runtime infer map changes or own cache rebuild policy.
