# `tests/unit/replay_state_hash_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove replay and state hashing behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 123
- tier: Tier 7: Durability Replay Multiplayer
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

- canonical hash stable
- replay reaches expected hash
- rejected command reason matches during replay
- divergence reports first mismatch

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Detailed Design Contract

Test cases required:

- two equivalent sessions created from the same fixture produce identical hashes;
- changing durable gameplay truth changes the hash;
- runtime events, metrics, projection, summary text, and app paths do not change
  the hash;
- `ClockState::stepRequested` does not change the hash; two otherwise identical
  paused states with `stepRequested=false` and `stepRequested=true` hash
  identically;
- save/load roundtrip preserves the final demo hash;
- replay of the first-room command log reproduces `OutOfRange` rejection, retry
  success, objective completion, tactical camera/clock state, final hash, and
  expected final summary;
- replay divergence reports the first mismatching command when a command is
  removed, reordered, or has the wrong rejection reason;
- replay returns `CommandReplayStatus::SummaryDiverged` with expected and actual
  summary facts when the final hash matches but the acceptance summary differs;
- command-log append malformed records return exact `CommandLogAppendStatus`
  values and leave record count, `nextSequence`, and `epoch` unchanged;
- state hash changes when `SessionState::nextCommandId` changes while all command
  log records, `nextSequence`, and `epoch` remain unchanged;
- append tests cover at least `InvalidCommandId`, `DuplicateCommandId`,
  `InvalidSequence`, `InvalidAdmissionState`, `InvalidRetrySource`, and
  `InvalidCommandPayload`;
- command-log restore rejects duplicate command ids and non-monotonic sequences
  before mutation;
- `CameraState::inputClearRequested` does not change the hash; two otherwise
  identical camera states with the flag cleared/set hash identically;
- `StateHashValue` is exactly `std::uint64_t` and can be assigned to/from
  `SaveMetadataSection::savedStateHash` without wrapper conversion;
- state hash formatting is 16 lowercase hex digits.

Tests must drive replay through public runtime APIs and deterministic fixtures
only. No test may depend on renderer, wall-clock time, network, or old iggy code.

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

- tests assert save/replay/hash contracts but own no save truth;
- tests must prove projection, runtime events, metrics, summary text, app paths,
  raw input, `ClockState::stepRequested`, and
  `CameraState::inputClearRequested` do not affect `StateHash`;
- replay assertions must use normal command admission/session execution.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/replay_state_hash_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
