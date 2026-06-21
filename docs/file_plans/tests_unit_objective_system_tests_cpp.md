# `tests/unit/objective_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove objective state and outcome behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 95
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

- collect_gold_key completes when inventory contains key
- outcome changes once
- nonmatching inventory does not complete
- reset clears completion

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

- if this file owns save truth, it must define exact fields included in `SaveEnvelope`;
- if this file owns derived data, it must be regenerable and excluded from save truth;
- if this file affects commands, replay must reproduce the same result and state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/objective_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/objective_system_tests.cpp
```

Required tests:

- `collect_gold_key_starts_active`: first-room objective seed creates active
  objective with condition player0 has `gold_key:1`.
- `validate_objective_context_accepts_first_room_state`: structural validation
  succeeds for first-room objectives and player slot 0 inventory with empty
  stacks.
- `validate_objective_context_rejects_invalid_structure_without_mutation`:
  invalid objective ids, invalid statuses, or invalid inventory stacks return
  structured invalid status and change no objective records.
- `missing_inventory_item_does_not_complete`: empty inventory keeps objective
  `Active`.
- `gold_key_inventory_completes_objective`: adding `gold_key:1` then evaluating
  sets status `Complete`.
- `completion_suggests_demo_complete_outcome`: result suggests
  `DemoComplete` when `collect_gold_key` completes.
- `completion_is_idempotent`: second evaluation leaves complete status and does
  not count another new completion.
- `nonmatching_item_or_player_does_not_complete`: wrong item or wrong slot
  leaves active.
- `failed_objective_does_not_auto_complete`: failed state remains failed unless
  the test explicitly invokes a current reset API or replaces baseline state.
- `evaluation_scans_vector_order`: multiple active objectives are evaluated in
  objective vector order and matching `PlayerHasItem` objectives complete
  exactly once.
- `objective_evaluation_does_not_mutate_inventory_or_world`: only objective
  state changes.
- `evaluation_after_valid_precheck_is_infallible`: after validation succeeds,
  `evaluateObjectives` does not return invalid objective/inventory status for
  the same structure in the same tick.
- `reset_baseline_objective_state_can_be_restored`: reset tests can assert the
  objective returns to `Active` through session baseline replacement.

Acceptance linkage:

- These tests protect the key final summary fact:
  `objective.collect_gold_key=Complete`.
