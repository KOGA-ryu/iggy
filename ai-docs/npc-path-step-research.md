# NPC Path Step Research

Purpose: define the clean boundary from actor route target to navigation request, path report, and one-step movement proposal before adding NPC position mutation.

## Grep Commands Used

```sh
rg -n "PathReport|PathFollower|MovementProposal|NavigationRequest|MoveTo|RouteTarget|PathStep" engine/src engine/tests
rg -n "startWalking|getStartDirection|dequeuePath|abortPath|Pathfinding|getTUCost" OpenXcom-master/src
rg -n "asPath|pathIndex|moveNextTarget|moveBestTarget|MOVEWAITROUTE|MOVENAVIGATE|FPR_WAIT" warzone2100-master/src
rg -n "mfndpos|m_move|ALLOW_MDISP|dist2|nix|niy" NetHack-NetHack-5.0/src/monmove.c
rg -n "WalkInDirection|DirOK|FindPath|IsTileAvailable|position.future" DevilutionX-master/Source
rg -n "CPathFind|ThePaths|SetObjective|OBJECTIVE_FLEE|OBJECTIVE_GOTO|SetFollowPath" re3-miami/src
```

## Iggy Current Anchors

- `NpcActorRouteTarget2D` turns movement intent into a route target and carries move mode/speed: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorRouteTarget2D.hpp:10-50`, `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorRouteTarget2D.cpp:28-78`.
- `MoveAwayFrom` correctly returns `NeedsEscapeDestination` unless an escape destination has been selected: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorRouteTarget2D.cpp:53-69`.
- `NpcActorEscapeRouteTarget2D` adapts flee intent into escape target then route target: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorEscapeRouteTarget2D.hpp:10-39`, `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorEscapeRouteTarget2D.cpp:28-45`.
- `NpcActorOccupancyQuery2D` answers occupants and `blockedFor` without mutating actors/map: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorOccupancyQuery2D.hpp:23-59`, `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorOccupancyQuery2D.cpp:61-90`.
- `NavigationGridValidator` still depends on old `modules/npc_ai::NpcMovementPlan`: `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationGridValidator.hpp:1-12`, `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationGridValidator.cpp:21-37`.
- `NavigationRequest` is clean and reusable as a server data result: `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationRequest.hpp:9-24`.
- `NavigationGridPathfinder` runs map-only BFS and emits tile/waypoint path diagnostics: `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationGridPathfinder.cpp:49-107`.
- `NavigationPathFollower` is a pure path-step primitive: path + follow state + current position + max distance -> next position/index/completion: `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationPathFollower.hpp:10-25`, `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationPathFollower.cpp:5-50`.
- Navigation follower tests cover non-found path, already-at-destination, partial step, exact step, multi-waypoint step, and final completion: `/Users/kogaryu/iggy/engine/tests/navigation_path_follower_tests.cpp:43-119`.
- Old `NpcAiNavigationRequest2D` and path report show useful diagnostics but adapt through old `NpcMovementPlan`: `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiNavigationRequest2D.cpp:13-34`, `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiPathReport2D.cpp:12-31`.
- Old `NpcAiMovementProposal2D` selects a waypoint target, not a bounded single actor step: `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiMovementProposal2D.cpp:67-76`.
- Move mode already has centralized speed multipliers: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcMoveMode.hpp:5-15`, `/Users/kogaryu/iggy/engine/src/scene/npc/NpcMoveMode.cpp:5-30`.

## Reference Findings

### NetHack

- Most monster movement is local one-step candidate selection, not cached full-path following: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1717-1733`.
- `mfndpos` generates legal nearby candidates, then movement scores distance/avoidance and selects `nix/niy`: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1927-1982`.
- Occupancy can reject candidate/final movement via `MON_AT` and displacement rules: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1958-1960`.

Lesson: one-step movement can be a complete path strategy for simple creatures; route/path caching is not mandatory for every actor.

### DevilutionX

- Monster walk start converts a chosen direction into `old/future` positions and reserves the future tile: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:786-820`.
- Random/approach walking tries preferred direction, then side directions through `DirOK`: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1732-1749`.
- `DirOK` checks relative movement legality and leader/minion constraints: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4356-4375`.
- Player pathing can compute a path, take only the first direction, then recheck final direction before starting walk: `/Users/kogaryu/iggy/DevilutionX-master/Source/player.cpp:1936-1972`.

Lesson: final step legality is checked close to movement start, even when a path exists.

### OpenXcom

- Pathfinding owns a path vector and exposes `getStartDirection`, `dequeuePath`, and `abortPath`: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.h:80-89`.
- `calculate` clears the previous path, validates destination, sets unit/movement type, and rejects blocked destination tiles before pathing: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:80-101`.
- A* resets nodes, explores neighbors, records previous direction, and builds a path when the target is reached: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:192-238`.
- Path following reads/dequeues one direction at a time: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:600-615`.
- Path can be aborted by clearing the cached path: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:621-625`.
- Walking state peeks the next direction, calculates movement cost/destination, aborts when costs fail, then dequeues and starts walking: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:270-314`, `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:396-408`.
- Unit walk start records direction, status, destination, last position, and cache invalidation: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:608-640`.

Lesson: mature turn movement separates path calculation from path consumption; the walk state owns one consumed step and can abort when conditions change.

### Warzone 2100

- Droid movement state stores path vector, path index, destination, source/target, speed, bump state, and formation pointer: `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:45-65`.
- Route request calculates a path or enters wait-route status; successful route resets `pathIndex`: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:235-330`.
- Path following chooses next/best target from `asPath`, can look ahead, can backtrack if direct path is lost, and advances `pathIndex`: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:620-696`.
- Movement update transitions from navigate to point-to-point, handles empty/lost paths, and reroutes to destination if stuck: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2241-2305`.

Lesson: cached paths need explicit owner state, stale-path detection, and reroute policy. Do not accidentally add that while building first pure path reports.

### re3 Miami

- Scripts assign high-level objectives such as goto/flee/follow route; path graph data is prepared separately by `CPathFind`: `/Users/kogaryu/iggy/re3-miami/src/control/Script3.cpp:952-1056`, `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:441-470`.

Lesson: objective assignment, path graph ownership, and physical movement remain separate layers.

## Boundary Recommendation

Use four explicit actor movement layers:

1. `NpcActorRouteTarget2D`
   - Owner: `scene/npc`
   - Input: movement intent or escape route adapter
   - Output: start position, target position, move mode, speed multiplier
   - No map validation except arrival tolerance.

2. `NpcActorNavigationRequest2D`
   - Owner: `scene/npc`
   - Input: `NpcActorRouteTarget2D`, `LevelTileMap`
   - Output: copied route target plus `navigation::NavigationRequest`
   - Validate destination bounds/walkability.
   - Do not depend on `modules/npc_ai::NpcMovementPlan`.
   - Reuse `navigation::NavigationRequest` status vocabulary if sufficient.

3. `NpcActorPathReport2D`
   - Owner: `scene/npc`
   - Input: actor navigation request, `LevelTileMap`
   - Output: copied request plus `navigation::NavigationPath`
   - Run `NavigationGridPathfinder` only when request is accepted.
   - Preserve `StartBlocked`, `StartOutOfBounds`, `NoPath`, and `DestinationRejected` diagnostics.

4. `NpcActorPathStep2D`
   - Owner: `scene/npc`
   - Input: path report, current actor position, move mode/speed config
   - Output: proposed next position/tile only; no actor mutation.
   - Use `NavigationPathFollower` under the hood.
   - Compute `maxDistance = baseStepDistance * npcMoveModeSpeedMultiplier(moveMode) * route.speedMultiplier` or use route speed directly if it already contains the combined multiplier.
   - Start from the first non-current waypoint when the actor is already within arrival tolerance of the path's first waypoint.

## Occupancy Placement

Safe first placement:
- route validation: static map only
- pathfinding: static map only
- path step: optional final proposed tile occupancy check using `NpcActorOccupancyQuery2D::blockedFor`
- executor: final hard check again before mutation

Wait/review:
- occupancy-aware BFS neighbor filtering
- reservations before path step/executor
- dynamic actor bodies in `CollisionWorld2D`
- path cache invalidation when occupancy changes

Why:
- Reference games often check final movement close to movement start.
- Occupancy-aware pathfinding is useful but multiplies cost by neighbor checks and creates stale-path complexity.

## Builder-Ready Slice Sequence

1. `NpcActorNavigationRequest2D`
   - Pure/read-only.
   - Inputs: route target, map.
   - Statuses: `Built`, `NoRouteTarget`, `RouteTargetNotReady`, `DestinationOutOfBounds`, `DestinationBlocked`.
   - Tests: no route target does not build; non-ready route preserves route diagnostic; walkable route builds accepted request; blocked/out-of-bounds preserve `NavigationRequestStatus`.

2. `NpcActorPathReport2D`
   - Pure/read-only.
   - Inputs: navigation request, map.
   - Statuses: `PathFound`, `NoNavigationRequest`, `PathNotFound`.
   - Tests: invalid request skips pathfinder; found path preserves tiles/waypoints; blocked start/out-of-bounds start/no path preserve navigation path diagnostics; input request is not mutated.

3. `NpcActorPathStep2D`
   - Pure proposal only.
   - Inputs: path report, optional `NavigationPathFollowState`, step config.
   - Statuses: `Proposed`, `NoPath`, `AlreadyAtTarget`, `InvalidMoveMode`, `ZeroStep`, `BlockedByOccupancy` optional.
   - Output: npc id, old position, proposed position, proposed tile, waypoint index, completed flag, move mode, requested/max distance.
   - Tests: no path gives no proposal; same target completes; walk moves 1x base step; run/sprint multiply step distance; large step can cross waypoints; current waypoint is skipped when within tolerance; occupancy block prevents proposal if occupancy is supplied.

4. Later `NpcActorPathFollowState2D`
   - Keep out of first implementation unless needed.
   - Would own waypoint index/path identity for cached paths.
   - Needs stale-path policy.

5. Later `NpcActorRouteCache2D` / path cache owner review
   - Do not put cached path inside `NpcActorState2D` yet.
   - Candidate owners: derived scene/npc movement cache, runtime frame state, or control state.
   - Decide only after executor and post-move report exist.

## Safe Now

- Pure actor navigation request wrapper.
- Pure actor path report wrapper.
- Pure path step proposal using `NavigationPathFollower`.
- Static map validation/pathfinding.
- Optional final proposed tile occupancy check in path-step diagnostics.
- Tests proving no actor/map/runtime mutation.

## Wait / Review

- Path caching ownership.
- Stale path invalidation.
- Occupancy-aware BFS.
- Reservations.
- Executor mutation.
- Runtime/session ownership.
- Animation/facing writeback.
- Post-move cache consumers.

## Compute Notes

- Current BFS path report is `O(width * height)` worst case per path request.
- Recomputing one path per NPC per frame will become expensive as actor count or map size grows.
- First implementation can recompute for simplicity if tests are small and isolated.
- API shape should not assume recompute forever: carry route target, path, and follow state separately so a later cache can store path/index without changing executor semantics.
- Path following is cheap: proportional to crossed waypoints in one step.
- Occupancy at final proposed tile is cheap with current query for small actor counts; indexed occupancy can make it `O(1)` later.

## Short Answer

After `NpcActorPostMoveReport2D`, build:

1. `NpcActorNavigationRequest2D`
2. `NpcActorPathReport2D`
3. `NpcActorPathStep2D`

Keep them in `scene/npc`, keep them pure/read-only, and use `NavigationPathFollower` for the step. Do not use `modules/npc_ai::NpcMovementPlan` in the new actor road except as historical reference.
