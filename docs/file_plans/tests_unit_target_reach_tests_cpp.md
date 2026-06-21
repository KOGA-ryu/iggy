# `tests/unit/target_reach_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove target discovery and reach gating behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 75
- tier: Tier 5: Playable Gameplay Systems
- module: `unit tests`
- file kind: `test`

## Ownership

This file owns:

- test scenarios
- assertions
- fixture setup helpers local to this test file
- regression coverage for documented semantics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d headers
- test framework chosen by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- nearest valid target found
- inactive/self ignored
- tie breaks by entity id
- start key out of range
- post-move key in range

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime should stay small; fixture-level tests may scan all demo entities and commands.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Target/reach request and result structs are transient and excluded from
  save/state hash.
- Replay reconstructs target/reach outcomes from saved state, command input, and
  explicit runtime config.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/target_reach_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/target_reach_tests.cpp
```

Use local world builders or the first-room fixture seed through public content
and runtime APIs. Tests must be headless and deterministic.

Required target discovery tests:

- `finds_gold_key_for_initial_interact`: player at `(0,0,0)`, `gold_key` at
  `(3,0,0)`, `tactical_marker_alpha` at `(2,0,1)`; query `Interact` returns
  `Found`, target `gold_key`, `targetPoint=(3.000,0.000,0.000)`, distance
  `3.000`.
- `max_distance_zero_means_uncapped`: `maxDistanceMeters <= 0` does not filter
  otherwise valid targets.
- `ignores_self_when_self_not_allowed`: player is not returned as its own
  interact target.
- `ignores_inactive_targets`: inactive pickup is skipped for `Interact`.
- `filters_by_command_kind`: marker is not returned for pickup/interact target
  discovery because marker targeting actions omit `Interact`.
- `nearest_wins_and_equal_squared_distance_uses_lower_entity_id`: ordering uses
  lexicographic `(distanceSquared, EntityId)`; exact equal computed squared
  distance breaks by lower `EntityId` with no epsilon/tolerance.
- `non_finite_candidate_target_point_is_skipped`: invalid candidate transform
  target points are skipped; if no valid candidate remains, query returns
  `NotFound`.
- `query_does_not_mutate_world`: world state before/after is identical.

Required reach tests:

- `initial_key_interact_is_out_of_range`: player `(0,0,0)` to key `(3,0,0)`
  with range `1.500` returns `ReachQueryStatus::OutOfRange`, distance `3.000`,
  and rejection maps to `CommandRejectionReason::OutOfRange`.
- `post_move_key_interact_is_reachable`: player `(2,0,0)` to key `(3,0,0)`
  with range `1.500` returns `Reachable`, distance `1.000`.
- `reach_uses_entity_transform_position`: reach from actor transform to target
  entity transform produces the acceptance distances `3.000` and `1.000`;
  local bounds do not change those distances.
- `range_boundary_is_inclusive`: distance exactly `1.500` is reachable.
- `non_positive_range_returns_invalid_range`: range `0.0` and negative range
  return `ReachQueryStatus::InvalidRange` and do not produce `Reachable`.
- `admission_passes_configured_interaction_range`: command admission uses
  `RuntimeConfig::interactionRangeMeters == 1.500` in the reach request; changing
  test config changes admission reach behavior without changing `ReachQuery`.
- `invalid_actor_and_target_are_structured`: invalid actor and target return
  explicit statuses, not crashes or prose-only messages.
- `inactive_target_returns_target_inactive`: when active target is required.
- `reach_query_does_not_execute_interaction`: inventory/objective/world active
  flags are unchanged.

Acceptance linkage:

- These tests protect the first command rejection and retry success path:
  initial `Interact(player,gold_key)` rejects `OutOfRange`; after movement to
  `(2,0,0)`, retry can pass reach.
