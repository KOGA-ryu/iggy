# `src/runtime/movement/MovementSystem.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic actor movement execution for accepted
realtime and tactical movement commands, mutating actor transforms only through
`WorldState` ownership APIs.

## Build Position

- priority rank: 68
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/movement`
- file kind: `header`

This header declares the movement system boundary. It consumes already-admitted
movement intent and returns movement results. It does not admit commands, own
the world, run physics, animate actors, or read raw input.

## Ownership

This file owns:

- movement execution context shape;
- movement system API declarations;
- request validation at movement-execution level;
- distance-limit policy hook;
- finite-destination checks;
- transform mutation boundary through `WorldState`;
- movement result production;
- deterministic conversion from accepted command to movement request through the
  required `movementRequestFromAcceptedCommand(...)` helper;
- blocked movement reason mapping.

It must not own:

- command admission;
- authority/player slot validation;
- target discovery;
- command log append;
- session tick order;
- world storage internals;
- raw input conversion;
- animation state;
- physics simulation;
- navmesh/pathfinding internals;
- renderer transform upload;
- app CLI paths;
- network transport;
- old `/Users/kogaryu/iggy` movement code.

## Required Header Shape

The implementation file must be:

```text
src/runtime/movement/MovementSystem.hpp
```

Required include style:

```cpp
#pragma once

#include "config/RuntimeConfig.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/world/WorldState.hpp"
```

The API must make world mutation ownership explicit. A header forward
declaration of `WorldState` is valid only when `MovementSystem.cpp` includes
`WorldState.hpp` for the public mutation API.

Required namespace:

```cpp
namespace iggy3d {
}
```

Do not include app, session, projection, renderer, socket, test, or old `iggy`
headers.

## Required Public Types

### `MovementSystemContext`

Declare this exact context:

```cpp
struct MovementSystemContext {
  WorldState* world = nullptr;
  const RuntimeConfig* config = nullptr;
};
```

Semantics:

- `world` is mutable because movement success updates actor transform through
  world APIs;
- `config` is read-only;
- missing `world` blocks movement with `MissingWorld`;
- missing config means system uses request `maxDistanceMeters` if nonzero;
  otherwise it uses the locked fallback `3.000`.

### Movement Result Return

Complete-build public API returns `MovementResult` directly. Mutation is inferred
from `blocked == None`.

## Required API

Declare these public functions:

```cpp
MovementResult executeMovement(
    MovementSystemContext& context,
    const MovementRequest& request);

MovementRequest movementRequestFromAcceptedCommand(
    const CommandRecord& command,
    MovementMode mode,
    const RuntimeConfig& config);
```

Private implementation helpers:

```cpp
MovementBlockedReason validateMovementRequest(
    const MovementSystemContext& context,
    const MovementRequest& request);

float movementDistanceMeters(const Vec3& start, const Vec3& destination);
float movementLimitMeters(const MovementRequest& request, const RuntimeConfig& config);
```

API rules:

- `executeMovement` is the only function in this file allowed to mutate world;
- `movementRequestFromAcceptedCommand` is the only public command-to-movement
  conversion helper in the complete build;
- helper functions must be pure/read-only;
- conversion from command must require an accepted `Move` command or document
  behavior for invalid command kind;
- session/tick code decides when to call movement execution.

## Movement Execution Semantics

`executeMovement` must:

1. verify context has world;
2. verify actor id is valid;
3. find actor in `WorldState`;
4. verify actor is active/movable if world exposes that field;
5. verify destination point is finite;
6. read actor start position from world;
7. compute distance from start to destination;
8. resolve max distance from request or config;
9. block if distance exceeds max;
10. update actor transform/position through `WorldState` API;
11. return `MovementResult` with start, destination, final position, distance,
    mode, actor, source `commandId`, and blocked reason.

No physics integration:

- no velocity simulation;
- no acceleration;
- no collision response beyond a structured `BlockedByWorld` failure from
  `WorldState::updateTransform`;
- no interpolation;
- no animation.

First complete runtime movement is explicit command reposition within allowed
distance.

## World Mutation Boundary

Movement system mutates only:

- actor position/transform through `WorldState::updateTransform`.

Movement system must not mutate:

- command log;
- inventory;
- objectives;
- combat;
- AI;
- clock;
- camera;
- save envelope;
- projection;
- renderer state.

If movement affects objective or interaction range indirectly, those systems
observe the new world state in subsequent system steps. Movement does not complete objectives by
itself.

## Distance And Limit Policy

Default movement limit:

```text
RuntimeConfig::movementDistanceMeters = 3.000
```

Resolution order:

1. if `request.maxDistanceMeters > 0`, use it;
2. else use config movement limit;
3. else use documented fallback `3.000` only if config is unavailable and tests
   lock that fallback.

Distance:

- Euclidean distance in X/Y/Z world meters;
- first-room movement is horizontal/forward but implementation is 3D;
- distance calculation must be deterministic and finite.

Failure:

- if distance > limit, return `MovementTooFar`;
- do not partially move actor;
- world remains unchanged.

## Destination Validity

Destination is valid only when:

- all coordinates are finite;
- position is representable in runtime world coordinates;
- the current world mutation API accepts it.

Failure mapping:

- NaN/Inf coordinate: `DestinationNotFinite`;
- semantically invalid point: `InvalidDestination`.

Command admission can reject invalid target points earlier. Movement system must
still guard against invalid requests because replay/load/internal bugs should
not corrupt world state.

## Command Conversion Semantics

`movementRequestFromAcceptedCommand` must:

- require `command.kind == CommandKind::Move`;
- read actor from `command.actor`;
- read destination from `command.payload.target.point` when present;
- copy source `commandId`;
- use the movement mode supplied by caller;
- set max distance from config.

It must not:

- run command admission;
- append to command log;
- mutate world;
- infer actor from player slot;
- read renderer picking state.

If command lacks a point target, conversion never asserts in normal compiled
behavior. It returns a request with `actor`, `sourceCommandId`, movement mode,
and max distance copied, and sets destination to the non-finite invalid `Vec3`
sentinel used by movement tests. `executeMovement` then returns
`MovementBlockedReason::DestinationNotFinite`. Admitted move commands always
have valid point targets; this path exists only for direct/internal misuse.

## Acceptance Demo Movement Requirements

### `cmd_move_to_key`

Initial world:

- player position `(0.000,0.000,0.000)`.

Request:

- actor `player`;
- destination `(2.000,0.000,0.000)`;
- max distance `3.000`;
- mode `Walk` or documented normal movement mode.

Expected result:

- blocked `None`;
- distance `2.000`;
- final position `(2.000,0.000,0.000)`;
- world mutated exactly once through `WorldState`;
- no inventory/objective mutation.

### `cmd_tactical_move`

Initial world for this command:

- player position `(2.000,0.000,0.000)`;
- clock is slow/tactical.

Request:

- actor `player`;
- destination `(2.000,0.000,1.000)`;
- max distance `3.000`;
- mode `Tactical`.

Expected result:

- blocked `None`;
- distance `1.000`;
- final position `(2.000,0.000,1.000)`;
- final acceptance summary uses this position.

## Blocked Movement Requirements

The system must produce deterministic blocked results for:

- missing world context: `MissingWorld`;
- invalid/missing actor: `InvalidActor`;
- inactive actor: `ActorInactive`;
- non-finite destination: `DestinationNotFinite`;
- movement too far: `MovementTooFar`;
- `WorldState::updateTransform` rejects the update: `BlockedByWorld`.

Blocked result rules:

- `finalPosition == start` when actor exists;
- `blocked != None`;
- world not mutated;
- source `commandId` preserved;
- result distance is computed when start/destination are valid.

## Session Tick Integration

`SessionTick` or session execution path owns when movement runs.

Movement system expects:

- command has already been accepted;
- command log already contains command or will contain it deterministically
  before execution;
- actor/slot legality has already been checked by admission;
- movement can still return `InvalidActor`, `ActorInactive`,
  `DestinationNotFinite`, `MovementTooFar`, or `BlockedByWorld` for direct
  MovementSystem calls, replay/load/internal invariant failures, tests that
  bypass admission, or world/config state changes between admission and
  execution. Normal command submission rejects too-far moves during admission.

If movement blocks after admission:

- return the exact blocked result status listed above;
- emit runtime event if event system is available;
- do not rewrite the already-logged admission status or rejection reason.

## Save Replay Multiplayer Notes

Save:

- save final world transform, not transient movement request/result;
- save command log movement intent.

Replay:

- rebuild movement request from accepted command;
- execute through this same system;
- final transform must match original run.

Multiplayer:

- local/remote command source does not affect movement math;
- authority/admission checks player slot before movement;
- movement request contains actor and destination only, not socket/packet data.

## State Hash Notes

State hash observes movement through:

- final `WorldState` actor transform;
- command log if command history is included.

Do not hash transient `MovementResult` directly unless runtime events/metrics
are explicitly included in a debug hash.

## Diagnostics And Errors

Movement result must provide enough facts for runtime events:

- actor id;
- source `commandId`;
- start position;
- destination;
- final position;
- mode;
- blocked reason;
- distance.

Diagnostic text is allowed and is not save truth.

## Compute Cost

Baseline first build:

- context validation: O(1);
- actor lookup: O(entity count);
- distance calculation: O(1);
- transform update: O(entity lookup or O(1) if lookup returns mutable reference);
- total: O(entity count).

Indexes can reduce lookup cost only if:

- entity iteration order remains deterministic;
- replay produces same result;
- state hash is unchanged.

## Tests And Verification

Covered by:

- `tests/unit/movement_system_tests.cpp`;
- `tests/unit/command_admission_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- accepted move command converts to expected request;
- `cmd_move_to_key` moves player to `(2.000,0.000,0.000)`;
- `cmd_tactical_move` moves player to `(2.000,0.000,1.000)`;
- invalid actor blocks `InvalidActor`;
- inactive actor blocks `ActorInactive`;
- NaN/Inf destination blocks `DestinationNotFinite`;
- too-far movement blocks `MovementTooFar`;
- blocked movement does not mutate world;
- successful movement mutates only actor transform;
- no command log, inventory, objective, camera, clock, projection, or renderer
  state is mutated.

## Completion Criteria

- `src/runtime/movement/MovementSystem.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares `MovementSystemContext`.
- It declares movement execution API.
- It declares accepted-command-to-request conversion or assigns that conversion
  to a documented owner.
- It documents all blocked movement reasons.
- It preserves `WorldState` as the only transform mutation owner.
- It supports both acceptance movement commands.
- It has no app, projection, renderer, socket, test, session-cycle, or old
  `iggy` dependency.
