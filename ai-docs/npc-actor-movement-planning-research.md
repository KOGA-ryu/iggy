# NPC Actor Movement Planning Research

Purpose: deepen the builder plan for moving from map-aware NPC control state to
actor movement without collapsing route, path, step, physics, and position
mutation into one opaque system.

This is a research/planning document. No production ownership is changed here.

## Source Anchors Inspected

Current Iggy scene/NPC ownership:

- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorState2D.hpp:11`:
  actor truth contains `npcId`, profile/faction ids, `position`,
  `currentGoalId`, and `present`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorState2D.hpp:20`:
  actor registry is vector-owned lookup data.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorControlState2D.hpp:13`:
  control state owns objective, behavior, and move mode, not position.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorFrameState2D.hpp:11`:
  frame state is a projection joining actor and control facts.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcObjective.hpp:8`:
  objective vocabulary includes `Flee`, `Follow`, `Attack`, `MoveTo`, and
  `Interact`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcBehaviorState.hpp:8`:
  behavior vocabulary includes `Seeking` and `Fleeing`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcMoveMode.hpp:5`:
  move mode is movement intensity, with helper speed multipliers.

Current local movement-intent slice already present in the worktree:

- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorMovementIntent2D.hpp:10`:
  intent type is `None`, `MoveTo`, `MoveAwayFrom`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorMovementIntent2D.hpp:16`:
  intent status distinguishes ready, no movement, missing control, actor not
  present, unsupported behavior, and invalid move mode.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorMovementIntent2D.cpp:37`:
  projection preserves frame facts and maps `Seeking` to `MoveTo`, `Fleeing` to
  `MoveAwayFrom`.
- `/Users/kogaryu/iggy/engine/tests/npc_actor_movement_intent_2d_tests.cpp:153`:
  seeking with a moving mode produces ready `MoveTo`.
- `/Users/kogaryu/iggy/engine/tests/npc_actor_movement_intent_2d_tests.cpp:183`:
  fleeing with a moving mode produces ready `MoveAwayFrom`.
- `/Users/kogaryu/iggy/engine/tests/npc_actor_movement_intent_2d_tests.cpp:288`:
  projection does not mutate the input frame.

Reusable navigation and physics APIs:

- `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationRequest.hpp:17`:
  accepted/rejected navigation request stores destination tile.
- `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationPath.hpp:18`:
  path stores tiles and waypoints.
- `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationGridPathfinder.cpp:49`:
  pathfinder validates start tile, walkability, then runs BFS.
- `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationPathFollower.hpp:10`:
  follow state owns waypoint index and completion.
- `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationPathFollower.cpp:5`:
  follower computes the next position by consuming movement distance across
  waypoints.
- `/Users/kogaryu/iggy/engine/src/servers/physics2d/CharacterMove2D.hpp:16`:
  movement result preserves requested delta, allowed delta, bounds, and motion
  query.
- `/Users/kogaryu/iggy/engine/src/servers/physics2d/CharacterMove2D.cpp:24`:
  character move is a read-only collision-world query that returns allowed
  movement.
- `/Users/kogaryu/iggy/engine/src/servers/physics2d/CollisionWorld2D.hpp:17`:
  collision world is a vector-backed query container.

Existing older/reference movement chain:

- `/Users/kogaryu/iggy/engine/src/modules/npc_ai/NpcMovementPlan.hpp:9`:
  older NPC movement plan only supports `MoveTo`.
- `/Users/kogaryu/iggy/engine/src/modules/npc_ai/NpcNavigationController.cpp:17`:
  older controller combines validation, pathfinding, path following, and next
  position into one step.
- `/Users/kogaryu/iggy/engine/tests/npc_navigation_controller_tests.cpp:48`:
  old tests prove no movement, request rejection, no path, moving, and arrived.

Existing scene/AI route vocabulary:

- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiRouteRequest2D.hpp:8`:
  route request statuses include no decision, hold, no movement, requested.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiNavigationRequest2D.hpp:15`:
  navigation request result wraps route plus navigation request.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiPathReport2D.hpp:15`:
  path report wraps navigation plus path.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiMovementProposal2D.hpp:11`:
  movement proposal chooses a waypoint/proposed position from a path.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcAiMovementCommandMapper2D.cpp:10`:
  command mapper converts movement proposal into runtime command data; do not
  use this as immediate actor movement.

Related research docs:

- `/Users/kogaryu/iggy/ai-docs/ai-actor-movement-research.md`
- `/Users/kogaryu/iggy/ai-docs/ai-mutation-runtime-movement-queueing.md`
- `/Users/kogaryu/iggy/ai-docs/programming-methods-tally.md`
- `/Users/kogaryu/iggy/engine/research/api_index.md`

## Ownership Boundary

Target movement road:

```text
NpcActorControlState2DRegistry
-> NpcActorFrameState2D projection
-> NpcActorMovementIntent2D
-> route target / escape target resolution
-> navigation request
-> path report
-> follow step proposal
-> physics movement query
-> actor state mutation
-> deferred occupancy/collision/cache updates
```

Do not skip straight from control state to `actor.position = target`. The useful
seams are:

- control state says why/how urgently to move
- movement intent says MoveTo or MoveAwayFrom
- route target turns intent into a concrete destination
- path report says whether grid movement is possible
- follow step says the next desired position/delta
- physics query clamps movement against collision
- executor mutates actor position by value
- post-move world/cache work remains deferred until ownership is reviewed

## Seeking Versus Fleeing

Seeking:

- `Seeking` behavior with moving `NpcMoveMode` can lower directly to `MoveTo`.
- It needs `npcId`, `startPosition`, `targetPosition`, `moveMode`, and speed
  multiplier.
- It can build a `NavigationRequest` from the target position immediately.

Fleeing:

- `Fleeing` should not be represented as `MoveTo(threatPosition)`.
- It should be represented as `MoveAwayFrom`, where `targetPosition` means
  threat/source position.
- Before pathfinding, `MoveAwayFrom` needs an escape target resolver.
- Minimal data needed: actor start, threat/source position, desired clearance or
  max sample distance, map bounds/walkability, optional AI-map tags such as cover
  or danger, and fallback status if no candidate exists.
- First safe implementation can report `NeedsEscapeDestination` or
  `UnsupportedMoveAwayFrom` until the escape-candidate slice exists.

## Reuse Rules

Safe to reuse:

- `NavigationGridPathfinder` for grid BFS once a concrete destination exists.
- `NavigationPathFollower` for waypoint stepping.
- `CharacterMove2D` for collision clamping.
- `CollisionWorld2D` as read-only collision query input.
- Player executor test patterns: copy input, preserve diagnostics, do not mutate
  source inputs.

Do not reuse blindly:

- `PlayerCommandPlan2D`: player command semantics are not NPC control semantics.
- Player facing/status fields: NPC actor state currently has no facing or
  movement status.
- Older `modules/npc_ai::NpcNavigationController` as final shape: it combines
  too many phases for the new scene/npc road.
- Runtime command mapper as immediate movement: it produces command data for a
  queue/frame lane, not direct actor-state mutation.

## Detailed Slice Sequence

### 1. `NpcActorMovementIntent2D`

Status: already appears present locally.

Type: pure/read-only projection.

Input:

- `NpcActorFrameState2D`

Output:

- `NpcActorMovementIntent2D`
- `MoveTo` for seeking
- `MoveAwayFrom` for fleeing
- no actor state mutation

Tests:

- missing control
- not-present actor
- seeking `Walk/Run` ready
- fleeing `Run/Sprint` ready
- still/none move mode does not move
- attacking/interacting unsupported
- input frame not mutated

### 2. `NpcActorRouteTarget2D`

Type: pure/read-only resolution.

Likely folder:

- `/Users/kogaryu/iggy/engine/src/scene/npc`

Input:

- `NpcActorMovementIntent2D`
- optional `LevelTileMap`
- config: arrival tolerance, flee distance, candidate radius

Output:

- `NpcActorRouteTarget2DResult`
- status: `Ready`, `NoMovementIntent`, `AlreadyAtTarget`,
  `NeedsEscapeDestination`, `NoEscapeDestination`, `InvalidTarget`
- concrete destination only when the intent has one

Boundary:

- `MoveTo` can pass through its target.
- `MoveAwayFrom` should either resolve a concrete escape point or clearly report
  that another slice is needed.
- No pathfinding yet.

Tests:

- `MoveTo` emits target destination.
- already-at-target reports no route needed.
- `MoveAwayFrom` preserves threat/source data and reports unresolved until
  escape candidate support exists.
- input intent is not mutated.

### 3. `NpcActorNavigationRequest2D`

Type: pure/read-only validation.

Input:

- `NpcActorRouteTarget2DResult`
- `LevelTileMap`

Output:

- wrapper around `navigation::NavigationRequest`
- destination rejection diagnostics

Boundary:

- Reuse the destination validation logic, but avoid depending on
  `modules/npc_ai::NpcMovementPlan`.
- If the existing `NavigationGridValidator` cannot be reused without the old
  module type, add a small scene-level adapter.

Tests:

- no route target -> no request
- out of bounds destination
- blocked destination
- accepted destination records tile coordinates

### 4. `NpcActorPathReport2D`

Type: pure/read-only path query.

Input:

- `NpcActorNavigationRequest2DResult`
- `LevelTileMap`

Output:

- wrapper around `navigation::NavigationPath`
- status: no request, path found, path not found, start rejected

Boundary:

- Reuse `NavigationGridPathfinder`.
- No path-follow state yet.
- No actor mutation.

Tests:

- reachable path preserves waypoints
- unreachable path reports no path and preserves actor position in diagnostics
- start blocked/out of bounds reports path failure

### 5. `NpcActorPathStep2D`

Type: pure/read-only movement proposal.

Input:

- `NpcActorPathReport2DResult`
- optional `NavigationPathFollowState`
- `maxDistance = baseSpeed * npcMoveModeSpeedMultiplier(moveMode) * dt`

Output:

- desired next position
- desired delta
- next follow state
- status: no path, moving, arrived

Boundary:

- Reuse `NavigationPathFollower`.
- Follow state is still not placed into authoritative actor state until ownership
  is reviewed.

Tests:

- partial step advances position
- exact waypoint advances waypoint index
- large step arrives
- no path produces no desired movement

### 6. `NpcActorMovementExecutor2D`

Type: first mutation slice.

Input:

- one `NpcActorState2D`
- `NpcActorPathStep2DResult` or equivalent desired position/delta
- `CollisionWorld2D`
- config: body size, anchor, max step/epsilon

Output:

- `NpcActorMovementExecution2DResult`
- copied `NpcActorState2D state`
- requested delta
- `CharacterMove2DResult`
- status: no movement, moved, blocked, rejected step, arrived

Boundary:

- Mutate only the returned actor copy.
- Do not mutate registry yet.
- Do not write occupancy/cache/runtime.
- Reuse `CharacterMove2D`, not player executor.

Tests:

- empty world moves actor copy
- blocked world clamps position and preserves hit metadata
- zero delta no movement
- rejected/no path preserves position
- input actor, path step, and collision world are not mutated

### 7. `NpcActorMovementFrameApply2D`

Type: controlled registry mutation by value.

Input:

- `NpcActorState2DRegistry`
- ordered list of per-actor execution results

Output:

- new registry copy
- apply entries/issues
- duplicate NPC handling policy

Boundary:

- This is where registry mutation belongs, not in pathfinding or route selection.
- Follow the existing control-frame apply/report style.

Tests:

- applies one moved actor
- missing actor issue
- duplicate execution issue or last-write policy, but explicit
- actor ordering preserved
- unrelated actors preserved

### 8. `NpcActorMovementFrameReport2D`

Type: pure report over frame apply.

Input:

- frame apply result

Output:

- changed count, moved count, blocked count, no movement count, issues

Boundary:

- No mutation.
- This gives UI/debug/ASCII projection a stable audit surface.

Tests:

- counts statuses correctly
- preserves issue metadata

### 9. Acceptance Road Test

Type: end-to-end scene-level test, still outside runtime.

Expected proof:

```text
actor registry + control registry
-> frame projection
-> movement intent
-> route target
-> navigation request
-> path report
-> path step
-> physics executor
-> movement frame apply/report
-> new actor registry
```

Acceptance scenario:

- map with a corridor and one blocker
- NPC A seeking a reachable target moves one step toward it
- NPC B fleeing reports unresolved escape target until escape candidate support
  exists, or moves away once that slice exists
- NPC C with still mode does not move
- blocked movement reports collision metadata
- original registries and collision world remain unchanged
- final registry contains only the expected position mutations

## Ownership Review Gates

Pause before:

- storing path-follow state in `NpcActorState2D`
- adding actor occupancy to `LevelTileMap`
- including NPC bodies in `CollisionWorld2D`
- adding dynamic actor-vs-actor collision
- runtime queue/session integration
- save/load persistence of movement state
- animation/facing state additions
- post-move interaction triggers

Currently missing and deferred:

- map occupancy update after actor movement
- dynamic collision world rebuild/merge with actor bodies
- path invalidation when map burns/rebuilds/rubble changes
- LOS/visibility recompute after movement
- spatial index for actors
- follow-state ownership
- runtime orchestration

## Compute Costs

- Per-NPC BFS pathfinding is `O(width * height)` worst case and allocates
  visited/previous/frontier vectors per call.
- Path following is `O(waypoints crossed this step)`.
- `CharacterMove2D` is `O(collision objects)` because `CollisionWorld2D` is a
  vector-backed query container.
- Full report copies can be expensive if each frame stores path, actor, control,
  movement, physics hit, and registry copies for many NPCs.
- Registry `find` over vector is linear; fine early, but batch movement can
  become `O(actor * result)` without an index or ordered merge.
- Flee candidate search can become `O(candidate tiles * scoring inputs)`; keep
  it explicit and bounded.
- Do not add dirty caches or spatial indexes before tests show the simple vector
  path is too costly.

## Builder-Ready Next Order Recommendation

Because `NpcActorMovementIntent2D` is already present locally, the next safe
builder order is:

```text
Build `NpcActorRouteTarget2D`.

Keep it pure/read-only.
Input: `NpcActorMovementIntent2D`, optional `LevelTileMap`, config.
Output: route target result with concrete destination for `MoveTo` and explicit
        unresolved/unsupported status for `MoveAwayFrom`.
Tests: MoveTo pass-through, already-at-target, MoveAwayFrom preserves threat
       position and does not fake a destination, input not mutated.
Do not include: pathfinding, CharacterMove2D, registry mutation, runtime,
                occupancy, dynamic collision, save/load.
```

After that, build `NpcActorNavigationRequest2D`, then `NpcActorPathReport2D`,
then `NpcActorPathStep2D`, then a one-actor `NpcActorMovementExecutor2D`.

## Blockers / Caveats

- The worktree already contains uncommitted production changes for
  `NpcActorMovementIntent2D` and CMake source/test registration. This doc did
  not modify them.
- `NavigationGridValidator` currently depends on old
  `modules/npc_ai::NpcMovementPlan`; a new scene-level adapter may be cleaner
  than extending the old module dependency.
- `MoveAwayFrom` requires a separate escape-target candidate policy before it can
  become a path request.
- Follow-state ownership is undecided; keep it out of `NpcActorState2D` for now.
