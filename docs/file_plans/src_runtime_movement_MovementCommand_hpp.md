# `src/runtime/movement/MovementCommand.hpp`

Updated: 2026-06-20

Exact purpose: declare the deterministic movement request and result value
types consumed by `MovementSystem`, session tick, command replay, tactical
movement tests, and the first-room acceptance demo.

## Build Position

- priority rank: 67
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/movement`
- file kind: `header`

This header defines movement data only. It does not validate command admission,
mutate world state, run pathfinding, integrate physics, animate actors, or read
raw input.

## Ownership

This file owns:

- movement mode enum;
- movement blocked reason enum;
- movement request value;
- movement result value;
- movement formatting/count helper declarations required by tests.

It must not own:

- command admission;
- actor/player authority;
- world mutation;
- collision or navmesh algorithms;
- animation;
- renderer transforms;
- raw keyboard/mouse/controller input;
- app command-line behavior;
- network transport;
- old `/Users/kogaryu/iggy` movement semantics.

## Required Header Shape

The implementation file must be:

```text
src/runtime/movement/MovementCommand.hpp
```

Required include style:

```cpp
#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
```

This header depends on `Command.hpp` for `CommandId`, `CommandRecord`, and
`kInvalidCommandId`. It depends on core values and command values, but not
world/session/system implementation headers.

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Enums

### `MovementMode`

Declare:

```cpp
enum class MovementMode : std::uint8_t {
  Walk,
  Tactical,
  Reposition,
};
```

Semantics:

- `Walk`: normal realtime explicit move.
- `Tactical`: slow-time/planning explicit move.
- `Reposition`: tool/test/internal reposition for deterministic setup.

The first acceptance demo uses:

- `MovementMode::Walk` for `cmd_move_to_key`;
- `MovementMode::Tactical` for `cmd_tactical_move`.

`Walk` and `Tactical` use the same movement math in the complete build; keep
the mode for diagnostics, replay, and command-source reporting.

### `MovementBlockedReason`

Declare:

```cpp
enum class MovementBlockedReason : std::uint8_t {
  None,
  InvalidActor,
  ActorInactive,
  InvalidDestination,
  DestinationNotFinite,
  MovementTooFar,
  BlockedByWorld,
  MissingWorld,
  InternalError,
};
```

Semantics:

- `None`: movement succeeded.
- `InvalidActor`: actor id invalid or missing.
- `ActorInactive`: actor exists but cannot move.
- `InvalidDestination`: destination point missing or semantically invalid.
- `DestinationNotFinite`: destination contains NaN/Inf.
- `MovementTooFar`: move exceeds configured one-command distance.
- `BlockedByWorld`: the public `WorldState` mutation API refused the otherwise
  valid transform update.
- `MissingWorld`: system context missing world state.
- `InternalError`: invariant violation.

Admission uses `CommandRejectionReason`; movement uses
`MovementBlockedReason`. Movement should not replace admission rejection reasons.

## Required Value Types

### `MovementRequest`

Declare:

```cpp
struct MovementRequest {
  EntityId actor;
  Vec3 destination;
  MovementMode mode = MovementMode::Walk;
  float maxDistanceMeters = 0.0f;
  CommandId sourceCommandId = kInvalidCommandId;
};
```

Field semantics:

- `actor`: entity to move.
- `destination`: target world-space position in meters.
- `mode`: movement mode for diagnostics/replay.
- `maxDistanceMeters`: maximum allowed move distance; `0.0f` means system should
  use runtime config default.
- `sourceCommandId`: `CommandRecord::commandId` that produced this movement
  request.

The request is not save truth by itself. The command log and resulting world
state are save/replay truth.

### `MovementResult`

Declare:

```cpp
struct MovementResult {
  EntityId actor;
  Vec3 start;
  Vec3 destination;
  Vec3 finalPosition;
  MovementMode mode = MovementMode::Walk;
  MovementBlockedReason blocked = MovementBlockedReason::None;
  CommandId sourceCommandId = kInvalidCommandId;
  float distanceMeters = 0.0f;
};
```

Field semantics:

- `start`: actor position before movement attempt.
- `destination`: requested target point.
- `finalPosition`: actor position after movement attempt.
- `blocked`: exact movement failure reason; `None` means success.
- `distanceMeters`: measured start-to-destination distance.

For blocked movement:

- `finalPosition` equals `start`;
- `blocked` must be non-`None`;
- world must remain unchanged unless partial movement becomes a documented
  feature.

For successful movement:

- `finalPosition == destination` in the first complete runtime;
- `blocked == None`;
- world transform mutation is performed by `MovementSystem`, not this value
  type.

## Helper Declarations

This header declares pure helpers:

```cpp
bool movementSucceeded(const MovementResult& result);
bool movementBlocked(const MovementResult& result);
MovementMode movementModeForClock(bool tacticalModeActive);
```

Helper rules:

- O(1);
- no world lookup;
- no mutation;
- no admission decisions;
- no file IO;
- no renderer/input dependency.

`MovementCommand.hpp` does not declare command-to-request conversion. The single
public conversion helper is
`MovementSystem::movementRequestFromAcceptedCommand(...)`.

## Acceptance Demo Movement Contract

The first-room acceptance demo requires two accepted movement requests.

### `cmd_move_to_key`

Source command:

```text
Move(actor=player, targetPoint=(2.000,0.000,0.000))
```

Expected movement request:

- actor: `player`;
- destination: `(2.000,0.000,0.000)`;
- mode: `MovementMode::Walk`;
- max distance: `3.000` meters from config/default;
- source `commandId`: `cmd_move_to_key.commandId`.

Expected movement result:

- start: `(0.000,0.000,0.000)`;
- final position: `(2.000,0.000,0.000)`;
- distance: `2.000`;
- blocked: `None`.

This movement puts player within `1.500` meters reach of `gold_key`.

### `cmd_tactical_move`

Source command:

```text
Move(actor=player, targetPoint=(2.000,0.000,1.000))
```

Expected movement request:

- actor: `player`;
- destination: `(2.000,0.000,1.000)`;
- mode: `Tactical`;
- max distance: `3.000` meters from config/default;
- source `commandId`: `cmd_tactical_move.commandId`.

Expected movement result:

- start: `(2.000,0.000,0.000)`;
- final position: `(2.000,0.000,1.000)`;
- distance: `1.000`;
- blocked: `None`.

Final player position in acceptance summary:

```text
player.position=(2.000,0.000,1.000)
```

## Invalid Movement Contract

The movement request/result values must support these failure cases:

- invalid actor;
- missing actor;
- inactive actor;
- NaN destination;
- infinite destination;
- movement distance greater than configured maximum;
- blocked-by-world failure from `WorldState::updateTransform`.

Failure values must be explicit enum states, not prose-only diagnostics.

## Data Ownership

`MovementRequest` owns intent data for a single movement execution attempt.

`MovementResult` owns the outcome facts for a single movement execution attempt.

Neither type owns:

- `WorldState`;
- entity transform mutation;
- command admission status;
- command log storage;
- save envelope storage;
- projection scene items.

`MovementSystem` owns converting an accepted command/request into a world
transform mutation.

## Save Replay Multiplayer Notes

Save:

- movement request/result are not primary save truth;
- command log stores movement command intent;
- world state stores final actor transform;
- runtime events can store movement result for diagnostics only.

Replay:

- replay re-submits the original move command;
- movement request is rebuilt deterministically;
- movement result must match original run if state/config match.

Multiplayer:

- movement request carries source `commandId` but not network packet data;
- player slot remains in `CommandRecord`, not `MovementRequest`;
- remote/local commands produce the same movement request after admission.

## State Hash Notes

State hash should not hash transient `MovementRequest` or `MovementResult`
values directly.

State hash should observe movement through:

- command log, if command log is included in hash;
- final `WorldState` transform;
- relevant objective/inventory/combat consequences if any.

## Diagnostics And Errors

Movement diagnostics should report:

- source `commandId`;
- actor id;
- requested destination;
- start position;
- final position;
- movement mode;
- blocked reason;
- distance.

Diagnostics text can change. `MovementBlockedReason` enum values should remain
stable once tests use them.

## Compute Cost

Movement request/result value operations:

- construction: O(1);
- copy/move: O(1);
- helper predicates: O(1);
- distance field is computed by `MovementSystem`, not this header.

No allocation should be required by these value types.

## Tests And Verification

Covered by:

- `tests/unit/movement_system_tests.cpp`;
- `tests/unit/command_admission_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- default request has invalid actor, zero/explicit destination, `Walk` mode,
  invalid source `commandId`;
- success result uses `blocked=None`;
- blocked result uses non-`None` reason;
- command-to-request conversion through
  `movementRequestFromAcceptedCommand(...)` preserves actor, point, source
  `commandId`, and mode;
- `cmd_move_to_key` is representable exactly;
- `cmd_tactical_move` is representable exactly;
- non-finite destination is representable and then rejected by movement
  system/admission tests;
- no helper mutates world or command log.

## Completion Criteria

- `src/runtime/movement/MovementCommand.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares `MovementMode`.
- It declares `MovementBlockedReason`.
- It declares `MovementRequest`.
- It declares `MovementResult`.
- It can represent both acceptance movement commands exactly.
- It separates movement values from admission, command log, world mutation,
  animation, renderer, app input, network transport, and save codec.
- It has no app, projection, renderer, socket, test, or old `iggy` dependency.
