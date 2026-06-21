# `src/runtime/targeting/ReachQuery.cpp`

Updated: 2026-06-20

Exact purpose: implement reach/range validation between actors, targets, and target points.

## Build Position

- priority rank: 74
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/targeting`
- file kind: `source`

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

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

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

- `src/runtime/targeting/ReachQuery.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/targeting/ReachQuery.cpp
```

Include paired header first:

```cpp
#include "runtime/targeting/ReachQuery.hpp"
```

`queryReach` algorithm:

1. Return `InvalidWorld` on null world.
2. Look up actor; missing/inactive actor returns `InvalidActor`.
3. Resolve target point:
   - if `hasTargetPoint`, validate finite and use it;
   - otherwise look up target entity;
   - if target missing, return `InvalidTarget`;
   - if target inactive and required, return `TargetInactive`;
   - use the target entity transform position.
4. Reject non-finite target point as `InvalidPoint`.
5. Reject non-finite or non-positive `maxRangeMeters` as `InvalidRange`.
6. Compute Euclidean 3D distance.
7. Use exactly `maxRangeMeters` from request. Do not read `RuntimeConfig`, do
   not apply a default, and do not clamp or repair the range.
8. Return `Reachable` when `distance <= maxRangeMeters`; otherwise
   `OutOfRange`.

Invariants:

- No mutation of world, command log, interaction, inventory, objective,
  diagnostics, save, projection, or renderer state.
- Exact `OutOfRange` result must be preserved for acceptance. Do not collapse
  it into `TargetNotReachable`.
- Distances are in meters using X/Y/Z world positions.
- Bounds are not used by complete-build reach; the distance is actor transform
  position to explicit target point or target entity transform position.
- Non-positive range returns `InvalidRange` before distance acceptance is
  considered.

`rejectionReasonForReach` must be deterministic and stable. For the first-room
initial interact it must return `CommandRejectionReason::OutOfRange`.

Compute cost:

- O(entity count) for actor plus target lookup in the first build.
- O(1) math after lookup.
