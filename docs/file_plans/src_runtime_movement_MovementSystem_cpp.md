# `src/runtime/movement/MovementSystem.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic actor movement for realtime and tactical commands.

## Build Position

- priority rank: 69
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/movement`
- file kind: `source`

## Ownership

This file owns:

- movement distance checks
- destination finite checks
- transform mutation through world API
- blocked movement diagnostics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- uses `RuntimeConfig::movementDistanceMeters`
- writes actor transform on success
- records blocked reason on failure

## Semantics

- no physics simulation in first complete runtime
- movement is command-based and deterministic
- navigation/pathfinding must enter behind this API

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return `MovementResult` with exact blocked reason fields documented by
   `MovementSystem.hpp`; diagnostics are emitted only by the owning
   session/runtime event path when that path wraps the result;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(entity lookup) per movement command; first build lookup is O(entity count).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- `MovementResult` is transient and excluded from save/state hash unless
  runtime events explicitly record it.
- Movement is replayed from `CommandLog` movement command intent plus saved
  `WorldState` and config.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/movement/MovementSystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required paired source path:

```text
src/runtime/movement/MovementSystem.cpp
```

Required include order:

```cpp
#include "runtime/movement/MovementSystem.hpp"
```

Then include only implementation dependencies actually needed for finite checks,
distance math, config defaults, and `WorldState` mutation. Do not include
session, app, projection, renderer, tests, filesystem, sockets, or old `iggy`
headers.

Functions implemented here must match `MovementSystem.hpp`:

- `executeMovement(MovementSystemContext&, const MovementRequest&)`;
- `movementRequestFromAcceptedCommand(...)`;
- any declared helper such as `validateMovementRequest`,
  `movementDistanceMeters`, or `movementLimitMeters`.

`executeMovement` algorithm:

1. If `context.world == nullptr`, return a result with
   `blocked=MissingWorld`, `actor=request.actor`, destination copied, source
   source `commandId` copied, and `world` unmodified.
2. Validate `request.actor`; missing/invalid actor returns `InvalidActor`.
3. Look up actor in `WorldState` using the public world API.
4. Reject inactive/unmovable actor as `ActorInactive`.
5. Reject non-finite destination as `DestinationNotFinite`.
6. Capture `start` from the actor transform before mutation.
7. Compute Euclidean 3D distance from `start` to `request.destination`.
8. Resolve max movement distance from request first, then config, then the
   documented fallback `3.000` only if no config is available.
9. If distance exceeds max, return `MovementTooFar` and keep actor at `start`.
10. Mutate only the actor transform/position through `WorldState::updateTransform`
    or the exact public world transform mutation API.
    If that public API rejects the update after the prior checks succeed, return
    `BlockedByWorld` and leave the world unchanged.
11. Return `blocked=None`, `finalPosition=request.destination`,
    `distanceMeters=distance`, and `sourceCommandId=request.sourceCommandId`.

Acceptance-sensitive movement:

- `cmd_move_to_key`: player starts at `(0,0,0)`, destination
  `(2,0,0)`, distance `2.000`, limit `3.000`, result `None`, final player
  position `(2,0,0)`.
- `cmd_tactical_move`: player starts at `(2,0,0)`, destination
  `(2,0,1)`, distance `1.000`, limit `3.000`, result `None`, final player
  position `(2,0,1)`.

Mutation invariants:

- Successful movement mutates exactly one actor transform.
- Blocked movement never mutates world, command log, inventory, objective,
  combat, AI, clock, camera, save, replay, projection, or renderer state.
- Movement never completes objectives and never acquires items directly.
- If world state changed between admission and execution, movement returns the
  exact failed condition: `InvalidActor`, `ActorInactive`,
  `DestinationNotFinite`, `MovementTooFar`, or `BlockedByWorld`. It must not
  rewrite the already-logged command admission result.

Save/replay/multiplayer implications:

- Save truth is the final actor transform in `WorldState` plus the original
  command in `CommandLog`; transient `MovementResult` is not primary save truth.
- Replay rebuilds the request from the accepted command and must produce the
  same final transform when state/config match.
- Multiplayer command source does not change movement math; authority and actor
  ownership are already checked before this system runs.

Completion requires implementation tests proving every blocked reason plus both
acceptance movement positions exactly.
