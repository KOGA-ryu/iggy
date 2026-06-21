# `tests/unit/movement_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove movement command execution behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 70
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

- valid move changes transform
- too-far move rejects/blocks
- invalid actor fails
- tactical move uses same deterministic mutation path

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

- Movement request/result structs are transient and excluded from save/state hash
  unless runtime events explicitly record them.
- Replay reconstructs movement from command log, world state, and config.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/movement_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required test source path:

```text
tests/unit/movement_system_tests.cpp
```

Use only the project test framework, public `iggy3d` headers, and small local
helpers. Do not include app, renderer, projection, sockets, wall-clock helpers,
or old `iggy` files.

Required setup helpers:

- create a `WorldState` containing a player entity at `(0,0,0)`;
- create a runtime config with `movementDistanceMeters = 3.000`;
- create accepted `Move` command records for the two acceptance movements;
- read actor position through `WorldState` public accessors only.

Required tests:

- `accepted_move_to_key_updates_player_position`: execute movement to
  `(2,0,0)`, assert result `blocked=None`, distance `2.000`, final position
  `(2,0,0)`, mode `Walk`, and world actor position matches.
- `accepted_tactical_move_updates_player_position`: start player at `(2,0,0)`,
  execute tactical movement to `(2,0,1)`, assert distance `1.000`, mode
  `Tactical`, result `None`, and final summary position source
  `(2.000,0.000,1.000)`.
- `accepted_command_conversion_preserves_payload`: conversion copies actor,
  point target, `CommandRecord::commandId` into `MovementRequest::sourceCommandId`,
  mode, and movement limit without mutating world or command log.
- `missing_point_conversion_returns_nonfinite_destination`: conversion of a
  malformed direct/internal `Move` without point target copies actor/source
  fields, sets the invalid non-finite destination sentinel, and
  `executeMovement` returns `DestinationNotFinite`.
- `missing_world_blocks_without_crash`: null context returns `MissingWorld`.
- `invalid_actor_blocks_without_mutation`: invalid or missing actor returns
  `InvalidActor`.
- `inactive_actor_blocks_without_mutation`: inactive actor returns
  `ActorInactive`.
- `non_finite_destination_blocks`: NaN and infinity return
  `DestinationNotFinite`.
- `too_far_destination_blocks`: destination beyond `3.000` returns
  `MovementTooFar`.
- `world_update_rejection_blocks_by_world`: if the public
  `WorldState::updateTransform` path rejects an otherwise valid move, result is
  `BlockedByWorld` and world is unchanged.
- `blocked_movement_preserves_start_position`: actor position remains unchanged
  for every blocked case.
- `movement_mutates_only_world_transform`: no command log, inventory,
  objective, combat, AI, clock, camera, projection, or renderer object is
  required or mutated.

Acceptance linkage:

- These unit tests must make the two acceptance movement facts independently
  true before `tests/acceptance/complete_runtime_demo_tests.cpp` is written:
  move to `(2,0,0)` puts the player in reach of `gold_key`, and tactical move
  to `(2,0,1)` creates the final summary position.
