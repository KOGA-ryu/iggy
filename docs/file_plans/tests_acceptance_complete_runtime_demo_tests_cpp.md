# `tests/acceptance/complete_runtime_demo_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove complete first-room acceptance demo behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 137
- tier: Tier 8: Product Proof And Tools
- module: `acceptance tests`
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

- load package and scenario
- reject out-of-range interact
- move then retry interact succeeds
- enter tactical slow time
- pause/step/resume
- save/load round-trip
- reset branch restores baseline
- replay reaches final summary and hash

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Detailed Design Contract

Acceptance proof required in one complete runtime test:

- target discovery identifies the first-room `gold_key` interactable;
- `cmd_interact_oob` is rejected with exact `OutOfRange` reason and no state
  mutation;
- `cmd_move_to_key` succeeds and places the player at `(2.000,0.000,0.000)`,
  putting `gold_key` within the configured `1.500` meter interaction range;
- `cmd_retry_key` is admitted as a retry record, then `SessionTick` executes a
  normalized effective `Interact` copied from `cmd_interact_oob`;
- command log diagnostics expose the original rejected `commandId` for
  `cmd_interact_oob` and the accepted retry `commandId` for `cmd_retry_key`;
- retry execution uses
  `EffectiveCommandIntent::sourceCommandId = cmd_retry_key.commandId` and
  `EffectiveCommandIntent::retrySourceCommandId = cmd_interact_oob.commandId`;
- retry interaction succeeds and updates inventory, `gold_key.active=false`, and
  objective `collect_gold_key=Complete`;
- retry proof asserts `CommandPayload::retrySourceCommandId` on `cmd_retry_key`
  equals `cmd_interact_oob.commandId`, and the effective interaction wrapper
  fields are `EffectiveCommandIntent::sourceCommandId` and
  `EffectiveCommandIntent::retrySourceCommandId`;
- replay reproduces the rejected out-of-range command, move, retry effective
  interact, inventory `gold_key:1`, inactive `gold_key`, complete
  `collect_gold_key`, and final position exactly;
- `cmd_tactical_move` succeeds and places the player at
  `(2.000,0.000,1.000)`;
- tactical slow-time camera command changes clock/camera state through runtime
  control path;
- pause, one-step tick, and resume semantics match `SessionRunner`;
- `cmd_wait` is admitted only after `cmd_resume`; this is slow mode, not paused
  mode, so it does not conflict with paused `Wait` rejecting as `SessionPaused`;
- save/encode/decode/load roundtrip preserves state hash and durable facts;
- save/load proof asserts `SaveSessionSection.nextCommandId` is restored, is
  greater than every restored command record `commandId`, assigns the next
  submitted proof command without collision, and advances by one only after
  append `Ok`;
- reset branch restores baseline, clears command log per first-build policy,
  clears pending execution sequences, restores `nextCommandId == 1`, and matches
  the fresh-scenario baseline hash;
- replay reproduces admissions/rejections and final state hash through normal
  command path;
- final state assertions include lifecycle `Complete`, outcome `DemoComplete`,
  `gold_key.active=false`, `tactical_marker_alpha.active=true`,
  `inventory.player0=gold_key:1`, `objective.collect_gold_key=Complete`,
  `camera.mode=ThirdPerson`, and `camera.previousRealtime=ThirdPerson`;
- final formatted summary exposes retry linkage fields and the test asserts they
  match command-log/effective-execution source truth:
  `retry.original_rejected_command_id=cmd_interact_oob.commandId`,
  `retry.retry_command_id=cmd_retry_key.commandId`,
  `retry.sourceCommandId=cmd_retry_key.commandId`,
  `retry.retrySourceCommandId=cmd_interact_oob.commandId`,
  `retry.executed.command_id=cmd_retry_key.commandId`, and
  `retry.executed.sequence=cmd_retry_key.sequence`;
- final formatted summary matches `fixtures/demos/first_room/expected_summary.txt`
  byte-for-byte.

The test must prove the public library runtime flow and must execute
`iggy3d_headless_demo` when the binary target is built. It must not require
renderer/window/GPU, network services, wall-clock sleeps, or old iggy code.
Failure output should name the exact phase and stable status/hash facts.

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime should stay small; fixture-level tests may scan all demo entities and commands.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- acceptance tests assert save/load/replay behavior through public APIs or tools;
- the test file owns assertions, not save payload policy or gameplay mutation;
- save/load/reset/post-load proof phases must not add command records to the ten
  gameplay command count in the completed source demo session;
- final proof requires save/load and replay to match the same state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/acceptance/complete_runtime_demo_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
