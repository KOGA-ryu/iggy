# `src/runtime/targeting/ReachQuery.hpp`

Updated: 2026-06-20

Exact purpose: declare reach/range validation between actors, targets, and target points.

## Build Position

- priority rank: 73
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/targeting`
- file kind: `header`

## Ownership

This file owns:

- reach request/result values
- transform-position distance calculation
- range rejection reason

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

- actor id
- target entity id or target point
- maximum range meters
- actual distance meters

## Semantics

- uses actor transform position to the explicit target point or target entity
  transform position
- out-of-range interaction returns exact `OutOfRange` reason
- does not execute interaction effects

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(entity lookup) plus O(1) math.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- `ReachQueryRequest` and `ReachQueryResult` are transient and excluded from
  save/state hash.
- Reach results are reconstructable from `WorldState`, command input, and
  explicit range configuration.
- Replay must reproduce the same reach result from the same saved state,
  command input, and runtime config.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/targeting/ReachQuery.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/targeting/ReachQuery.hpp
```

This header declares read-only range validation. It is the source of the
acceptance-sensitive `OutOfRange` fact carried by `CommandAdmission`.

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
```

Forward declare `WorldState` if possible.

Required enum:

```cpp
enum class ReachQueryStatus : std::uint8_t {
  Reachable,
  OutOfRange,
  InvalidWorld,
  InvalidActor,
  InvalidTarget,
  TargetInactive,
  InvalidPoint,
  InvalidRange,
};
```

Required request:

```cpp
struct ReachQueryRequest {
  const WorldState* world = nullptr;
  EntityId actor;
  EntityId target;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  float maxRangeMeters = 0.0f;
  bool requireActiveTarget = true;
};
```

Required result:

```cpp
struct ReachQueryResult {
  ReachQueryStatus status = ReachQueryStatus::InvalidWorld;
  EntityId actor;
  EntityId target;
  Vec3 actorPoint;
  Vec3 targetPoint;
  float maxRangeMeters = 0.0f;
  float distanceMeters = 0.0f;
};
```

Required API:

```cpp
ReachQueryResult queryReach(const ReachQueryRequest& request);
CommandRejectionReason rejectionReasonForReach(const ReachQueryResult& result);
```

Mapping:

- `Reachable` -> `None`;
- `OutOfRange` -> `OutOfRange`;
- invalid actor -> `InvalidActor`;
- invalid target/point -> `InvalidTarget` or `InvalidTargetPoint` depending
  command context;
- inactive target -> `TargetInactive`;
- `InvalidRange` -> `InternalError` when mapped through admission.

Range ownership:

- `ReachQuery` does not read `RuntimeConfig` and does not define gameplay
  defaults.
- Callers must pass a finite positive `maxRangeMeters`.
- `CommandAdmission` is the caller that reads
  `RuntimeConfig::interactionRangeMeters` and passes the first-room value
  `1.500`.
- `maxRangeMeters <= 0.0f` returns `InvalidRange` deterministically.
- When `hasTargetPoint` is false, reach uses the target entity transform
  position as `targetPoint`; bounds are not used for complete-build reach.
- When `hasTargetPoint` is true, reach uses the explicit finite `targetPoint`.

Acceptance facts:

- Player `(0,0,0)` to `gold_key` `(3,0,0)` with range `1.500` returns
  `OutOfRange`, distance `3.000`.
- Player `(2,0,0)` to `gold_key` `(3,0,0)` with range `1.500` returns
  `Reachable`, distance `1.000`.
