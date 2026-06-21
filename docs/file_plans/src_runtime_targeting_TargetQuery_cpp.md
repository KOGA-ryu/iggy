# `src/runtime/targeting/TargetQuery.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic target discovery for interact, inspect, combat, and tactical commands.

## Build Position

- priority rank: 72
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/targeting`
- file kind: `source`

## Ownership

This file owns:

- query shape
- target result shape
- entity-kind filter rules
- tie-break policy

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

- query actor id or origin point
- command kind filter
- max range
- target result id, point, distance, status

## Semantics

- linear scan in stable world order
- ignore inactive/self entities unless command allows self
- nearest wins by squared distance; exact equal squared distance uses lower entity id
- no mutation

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(entity count) per query.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- `TargetQueryRequest` and `TargetQueryResult` are transient and excluded from
  save/state hash.
- Query results are reconstructable from `WorldState` plus command input.
- Replay must reproduce the same target discovery result from the same saved
  state and command input.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/targeting/TargetQuery.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/targeting/TargetQuery.cpp
```

Include paired header first:

```cpp
#include "runtime/targeting/TargetQuery.hpp"
```

Implementation algorithm for `queryTarget`:

1. Return `InvalidWorld` if `request.world == nullptr`.
2. Resolve origin from `request.origin` if present, otherwise from actor entity.
3. Return `InvalidActor` if actor lookup is needed and fails.
4. Return `InvalidOrigin` for any non-finite origin coordinate.
5. Iterate `WorldState` entities in stable storage/order.
6. Skip invalid, self, inactive, or unsupported targets according to request.
7. Compute target point as the candidate entity transform position.
8. Compute finite squared distance from origin to target point for ordering and
   compute Euclidean distance for result diagnostics.
9. Skip entities beyond `maxDistanceMeters` only when that value is positive.
10. Pick the lexicographically smaller `(distanceSquared, EntityId)` pair. Equal
    distance means exact equality of the computed squared-distance value; no
    epsilon or tolerance is used.
11. Return `Found` with target facts, or `NotFound`.

Targetability rules for the complete build:

- `targetSupportsCommandKind` reads only `EntityTargeting::targetable` and
  `EntityTargeting::actions`/`TargetAction`.
- `Pickup` is selected for `Interact` only because its targeting actions contain
  `Interact`.
- `Marker` supports tactical movement only when its targeting actions contain
  `Move`; it must not be selected for first-room `Interact`.
- `Player` self-target is skipped for first-room target discovery.
- Inactive entities are skipped for `Interact`.

Mutation and ownership:

- This file never mutates `WorldState`.
- It never writes command records, runtime events, save data, projection output,
  UI prompts, or renderer picking state.
- It is called by `CommandAdmission`, debug projection, or tests as a read-only
  query.

Diagnostics:

- Machine-readable status is the result enum.
- Human text is allowed for diagnostics but must not be the only failure
  signal.

Compute cost:

- O(entity count) scan, O(1) per entity.
- Spatial indexes, when introduced, must be derived/rebuildable and preserve
  tie-breaking by lower `EntityId`.
