# `tests/unit/interaction_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove interaction effects behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 83
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

- pickup adds gold_key
- pickup deactivates entity
- inspect is no-op success
- invalid target does not mutate
- objective handoff occurs

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

- `tests/unit/interaction_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/interaction_system_tests.cpp
```

Required tests:

- `pickup_adds_gold_key_and_deactivates_target`: accepted interact against
  active `gold_key` returns `Succeeded`, adds `gold_key:1` to player 0, and
  sets `gold_key.active=false`.
- `pickup_triggers_objective_evaluation`: after pickup, objective state reports
  `collect_gold_key=Complete` or returns an outcome suggestion consumed by
  session.
- `rejected_or_pending_command_does_not_execute`: non-accepted command returns
  `InvalidCommand` and mutates nothing.
- `raw_retry_command_does_not_execute`: accepted raw `Retry` passed directly to
  `InteractionSystem` returns `InvalidCommand`; only `SessionTick` normalizes
  retry into an effective interact command.
- `normalized_retry_interact_executes_once`: request with command kind
  `Interact`, `sourceCommandId=cmd_retry_key.commandId`, and
  `retrySourceCommandId=cmd_interact_oob.commandId` succeeds once and records
  those `CommandId` values in the result/event.
- `pickup_reports_canonical_interaction_fields`: result asserts
  `InteractionDefinition` field names `kind`, `primaryEffect`, `itemId`,
  `itemCount`, `objectiveId`, `repeatable`, and
  `deactivateTargetOnSuccess`; tests do not use legacy interaction aliases.
- `invalid_target_does_not_mutate`: missing target returns `InvalidTarget`.
- `inactive_target_does_not_duplicate_pickup`: inactive `gold_key` returns
  `TargetInactive` and inventory stays unchanged.
- `null_world_context_returns_invalid_world`: null world returns
  `InteractionStatus::InvalidWorld`, preserves request command linkage where
  available, and mutates no inventory/objective state.
- `null_inventory_context_returns_invalid_inventory`: null inventory returns
  `InteractionStatus::InvalidInventory`, does not call `WorldState::setActive`,
  and mutates no objective state.
- `null_objective_context_returns_invalid_objective_state`: null objective state
  returns `InteractionStatus::InvalidObjectiveState`, does not call
  `InventorySystem::addItem`, and leaves world active flags unchanged.
- `inventory_failure_is_atomic`: invalid item/count or invalid inventory leaves
  target active and objective unchanged.
- `invalid_objective_context_is_atomic`: objective precheck failure returns
  `ObjectiveFailed` before inventory add, target deactivation, or objective
  mutation.
- `valid_pickup_objective_evaluation_is_infallible`: structurally valid
  objective/inventory state produces a successful pickup with inventory, target,
  and objective mutation flags set.
- `inspect_supported_as_no_mutation`: supported inspect returns success with no
  inventory/world/objective mutation.
- `interaction_does_not_run_reach_query`: tests call interaction directly with
  accepted command and verify reach legality is not recomputed here.

Acceptance linkage:

- These tests prove the post-retry mutation facts required by the first-room
  demo: `inventory.player0=gold_key:1`, `gold_key.active=false`, and
  `objective.collect_gold_key=Complete`.
- They also prove `cmd_retry_key` executes the original interact exactly once
  after admission/reach has accepted the retry.
