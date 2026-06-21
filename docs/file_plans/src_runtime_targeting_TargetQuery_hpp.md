# `src/runtime/targeting/TargetQuery.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic target discovery for interact, inspect, combat, and tactical commands.

## Build Position

- priority rank: 71
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/targeting`
- file kind: `header`

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
- discovery-distance cap where positive
- target result id, point, distance, status

## Semantics

- linear scan in stable world order
- ignore inactive/self entities unless command allows self
- nearest wins by squared distance; exact equal squared distance uses lower entity id
- no mutation

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/runtime/targeting/TargetQuery.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/targeting/TargetQuery.hpp
```

This header declares read-only target discovery. It does not decide final
command admission, execute effects, or mutate selection state.

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/world/EntityState.hpp"
```

Forward declare `WorldState` if possible. Do not include app, session,
projection, renderer, tests, or old `iggy`.

Required enum:

```cpp
enum class TargetQueryStatus : std::uint8_t {
  Found,
  NotFound,
  InvalidWorld,
  InvalidActor,
  InvalidOrigin,
};
```

Required request:

```cpp
struct TargetQueryRequest {
  const WorldState* world = nullptr;
  EntityId actor;
  bool hasOrigin = false;
  Vec3 origin;
  CommandKind commandKind = CommandKind::None;
  float maxDistanceMeters = 0.0f;
  bool allowSelf = false;
  bool requireActive = true;
};
```

Origin rules:

- If `hasOrigin` is true, use `origin`.
- Otherwise, use actor position from `world`.
- Invalid actor without explicit origin returns `InvalidActor`.
- Non-finite explicit origin returns `InvalidOrigin`.

Required result:

```cpp
struct TargetQueryResult {
  TargetQueryStatus status = TargetQueryStatus::NotFound;
  EntityId target;
  Vec3 origin;
  Vec3 targetPoint;
  float distanceMeters = 0.0f;
  bool targetActive = false;
  bool targetSupportsCommand = false;
};
```

Required API:

```cpp
TargetQueryResult queryTarget(const TargetQueryRequest& request);
bool targetSupportsCommandKind(const EntityState& entity, CommandKind kind);
```

`targetSupportsCommandKind` is a public header helper. This header depends on
`runtime/world/EntityState.hpp` for the complete `EntityState` type and on
`runtime/command/Command.hpp` for `CommandKind`.

Semantics:

- Scan active world entities in deterministic world order.
- Skip self unless `allowSelf`.
- Skip inactive entities when `requireActive`.
- Filter by command kind targetability.
- `TargetQueryResult::targetPoint` for entity targets is the entity transform
  position for the complete build.
- Targetability is decided only from `EntityTargeting::targetable` and
  `EntityTargeting::actions`/`TargetAction`; interaction effect fields do not
  make an entity targetable by themselves.
- Nearest valid target wins by computed finite squared distance from query origin
  to `targetPoint`.
- Tie-break order is lexicographic `(distanceSquared, EntityId)`: exact equality
  of the computed squared-distance value breaks by lower `EntityId`.
- No epsilon or tolerance is used in the first complete build.
- Candidate non-finite `targetPoint` values are skipped as invalid candidates; if
  no valid candidate remains the query returns `NotFound`.
- `maxDistanceMeters <= 0` means no query-distance cap. Reach range remains
  owned by `ReachQuery`/admission.

Acceptance fact:

- In first-room initial state, `queryTarget(actor=player, kind=Interact)` must
  find `gold_key` at distance `3.000`, not `tactical_marker_alpha`, because
  the marker is not an `Interact` pickup target.
