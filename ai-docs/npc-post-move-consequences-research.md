# NPC Post-Move Consequences Research

Purpose: identify what becomes stale after an NPC position changes, before Iggy adds `NpcActorMovementExecutor2D`.

## Grep Commands Used

```sh
rg -n "newsym\\(|vision|visible|place_monster|remove_monster|m_move" NetHack-NetHack-5.0/src
rg -n "ChangeLight|ChangeVision|occupyTile|M_StartStand|UpdateEnemy|DoWalk" DevilutionX-master/Source
rg -n "setUnitPosition|setUnit\\(|calculateFOV|calculateUnitLighting|checkReactionFire|cacheUnit" OpenXcom-master/src
rg -n "RemoveAndAdd\\(|ProcessShift\\(|ProcessCollision|m_entryInfoList|bCollisionProcessed" re3-miami/src
rg -n "moveUpdateDroid|MOVEWAITROUTE|bumpTime|formationLeave|MOVEPAUSE" warzone2100-master/src
rg -n "Interaction|Reach|Target|Effect|Derived|Cache|RuntimeSession|NpcActor" engine/src
```

## Iggy Current Anchors

- NPC actor state currently owns id/profile/faction/position/present only: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorState2D.hpp:11-24`.
- NPC occupancy is already shaped as a derived projection from actor registry, not map/runtime truth: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorOccupancy2D.hpp:45-59`.
- Player movement executor mutates a copied player state and reports status/movement; it does not rebuild caches itself: `/Users/kogaryu/iggy/engine/src/scene/player/PlayerMovementExecutor2D.cpp:38-99`.
- Level derived caches already update from changed tiles: `/Users/kogaryu/iggy/engine/src/scene/level/LevelDerivedCacheState.hpp:39-53`.
- Tile render chunk cache updater consumes dirty chunks: `/Users/kogaryu/iggy/engine/src/scene/level/LevelTileRenderChunkCacheUpdater.hpp:20-35`.
- Runtime session owns level/player/tick/cache mirrors, but NPC actor state is not part of it yet: `/Users/kogaryu/iggy/engine/src/runtime/RuntimeSessionState.hpp:12-20`.
- Interaction reach is query-style: actor position plus target query gives a result; it is not a persistent cache: `/Users/kogaryu/iggy/engine/src/scene/interaction/InteractionReach2D.hpp:20-36`.
- Runtime reports already use event/counter summaries rather than forcing every subsystem into one mutation step: `/Users/kogaryu/iggy/engine/src/runtime/RuntimeGameplayFrameReport.hpp:11-47`.

## Reference Findings

### NetHack

- Movement can temporarily remove/place monsters and refresh only touched cells with `newsym`: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1489-1505`.
- Old position is explicitly refreshed after movement: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1508-1513`.
- Door/unblock movement-like consequences update display, recalculate the blocking point, and trigger vision recalculation: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1528-1535`.
- Normal monster move removes old occupancy, places new occupancy, emits movement messaging, then handles special body shape/unhide facts: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2049-2063`.
- `newsym` is the display dirty-cell projection entry point: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:912-938`.

Lesson: update authoritative position/occupancy first, then mark only affected display/vision cells unless a special effect demands more.

### DevilutionX

- Player walk start reserves the future tile before final arrival: `/Users/kogaryu/iggy/DevilutionX-master/Source/player.cpp:84-90`.
- Player walk finish clears old `dPlayer`, sets final position, occupies final tile, updates light/vision coordinates: `/Users/kogaryu/iggy/DevilutionX-master/Source/player.cpp:404-429`.
- Monster walk finish clears old `dMonster`, mutates position, occupies final tile, updates light, then switches to stand mode: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1087-1099`.
- Stand mode clears movement variables, resets future/old positions, and updates enemy targeting: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3927-3940`.
- Knockback clears old occupancy and updates light around position changes: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3950-3961`.

Lesson: movement completion synchronously fixes occupancy, light/vision coordinates, animation/mode state, and enemy target freshness.

### OpenXcom

- `setUnitPosition` validates body tiles, then mutates unit position and tile occupancy together: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.cpp:1584-1637`.
- Falling/movement state clears old occupied tiles and sets new occupied tiles: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitFallBState.cpp:106-126`.
- Movement consequences include unit cache invalidation/cache rebuild, unit lighting recalculation, FOV, proximity grenade checks, and possible path abort: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitFallBState.cpp:176-201`.
- Normal walk does FOV/cache work when kneeling changes posture: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:80-92`.
- Normal walk recalculates unit lighting/FOV, checks proximity grenades, stops on spotting, and checks reaction fire: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:188-223`.
- End-of-walk refreshes cache, unit lighting, FOV, and map unit cache: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:526-539`.
- Full FOV recompute exists, but loops units and is reserved for broader changes: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/TileEngine.cpp:3072-3080`.

Lesson: post-move can fan out to expensive gameplay checks, but the movement state keeps them targeted to the moved unit/tiles when possible.

### re3 Miami

- Physical movement applies speed/turn, scans current sector lists for shift/collision, may roll back, then updates sector membership with `RemoveAndAdd`: `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.cpp:2055-2105`.

Lesson: continuous worlds use spatial sector membership as the post-move cache; update only the moved entity's sector links.

### Warzone 2100

- Droid movement update stores old status, handles inactive/formation cleanup, waits for routes, and changes movement state as it progresses: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2165-2238`.
- Bump timers can flip between move/pause states after collision pressure: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2333-2356`.

Lesson: post-move state includes movement-status bookkeeping and reroute/block pressure, not just position.

## Transferable Shape for Iggy

Post-move work splits into three layers:

1. Immediate authoritative mutation:
   - actor copied state position
   - optional facing/move status
   - blocked/moved/no-op status

2. Derived/query invalidation facts:
   - old/new tile became dirty
   - occupancy must be rebuilt from actor registry
   - actor's AI-map read is stale
   - interaction reach may need re-evaluation
   - render projection for old/new tile may be stale

3. Deferred systems:
   - visibility/LOS/FOV
   - sound/noise/alert events
   - triggers/overlaps
   - animation graph
   - render/cache rebuild
   - runtime event bus/session integration

The first Iggy executor should stop at layers 1 and 2.

## Recommended First Slice

Build `NpcActorPostMoveReport2D` before or alongside the first executor.

Suggested folder:
- `engine/src/scene/npc/`

Suggested data:
- `npcId`
- `oldPosition`
- `newPosition`
- `oldTile`
- `newTile`
- `status`: `NotMoved`, `Moved`, `Blocked`, `Rejected`
- `blockingKind`: `None`, `Map`, `Npc`, `CollisionWorld`, `InvalidStep`
- `blockingNpcId`
- `dirtyTiles`: old tile and new tile, deduped
- `needsOccupancyRebuild`
- `needsAiMapQueryRefresh`
- `needsInteractionRefresh`
- `needsRenderRefresh`
- `needsVisibilityRefresh` as report-only, false or optional until LOS ownership is reviewed

Corrected from the suggested shape:
- Keep `dirtyRegions` out of the first slice. No region/chunk ownership exists for actor movement yet.
- Keep `needsRenderRefresh` report-only. Existing render caches are level-tile derived, not actor-render cache invalidators.
- Do not set `needsAiMapQueryRefresh` for blocked/no-op moves unless the actor's position or control state changed.

## Builder-Ready Slice Sequence

1. Pure/report-only: `NpcActorPostMoveReport2D`
   - Input: npc id, old/new position, blocked reason.
   - Output: old/new tile, deduped dirty tiles, refresh flags.
   - Tests: no move emits one dirty tile or none by policy; move emits old+new; same-tile substep emits one tile; blocked-by-npc carries blocking id.

2. Pure/report-only: `NpcActorPostMoveReportBuilder2D`
   - Input: before actor, after actor, execution status/block reason.
   - Output: report.
   - Tests: moved sets occupancy/AI/interaction/render refresh flags; blocked does not request occupancy rebuild unless position changed.

3. Mutation later: `NpcActorMovementExecutor2D`
   - Mutates copied `NpcActorState2D` or copied registry only.
   - Consumes route/path/step and occupancy query.
   - Returns execution result plus `NpcActorPostMoveReport2D`.

4. Wait/review: `NpcActorPostMoveFrameReport2D`
   - Aggregates per-actor dirty tiles and refresh counters.
   - Useful before runtime integration, but not needed for first single-actor executor test.

5. Wait/review: cache consumers
   - Occupancy rebuild from resulting registry.
   - AI-map query refresh.
   - interaction reach refresh.
   - actor render projection refresh.
   - LOS/visibility/FOV.

## Safe Now

- Report-only dirty facts.
- Old/new tile derivation.
- Blocking kind/id fields.
- Refresh booleans that do not call other systems.
- Tests that prove the executor does not mutate map, runtime, cache, event bus, or visibility state.

## Wait / Review

- Runtime/session ownership of NPC actor state.
- Actual cache rebuilds from post-move facts.
- Visibility/LOS/FOV recomputation.
- interaction trigger execution.
- sound/noise/alert events.
- animation graph state beyond minimal facing/move status.
- render cache/chunk update calls.
- region ownership, dirty chunks, spatial buckets.

## Compute Notes

- Post-move report construction is `O(1)` per actor.
- Dirty old/new tile list is at most two tiles before multi-tile actors.
- Occupancy rebuild remains `O(actor_count)` if rebuilt from registry after a frame.
- Full FOV/LOS is expensive and should not run inside the first executor.
- Render/cache updates should consume dirty facts later, ideally by chunk/tile batching.
- Event/trigger checks can grow with nearby target count; keep them outside the first position mutation.
