# `tests/unit/save_load_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove save/load envelope durability behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 118
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
- test framework selected by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- session round-trips
- incompatible version rejects without mutation
- derived projection excluded
- state hash preserved

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests assert exact rejection/status codes when behavior is part of runtime contract

## Detailed Design Contract

Test cases required:

- saving the completed first-room demo state contains inactive `gold_key`,
  inventory `gold_key:1`, objective `collect_gold_key=Complete`, and command log
  records for both rejected `cmd_interact_oob` and accepted `cmd_retry_key`;
- saved envelope sections are present by name: metadata, session, world,
  players, clock, camera, commandLog, inventory, combat, ai, and objectives;
- save envelope does not contain projection/debug projection, summary, renderer,
  runtime events, metrics, app paths, wall-clock, raw input, or socket/platform
  state;
- encode/decode roundtrip returns an equivalent envelope and identical hash;
- encoding the same envelope twice produces byte-identical text;
- encoded save starts with `iggy3d.save_envelope.v1`;
- encoded section order is metadata, session, world, players, clock, camera,
  commandLog, inventory, combat, ai, objectives;
- encoded repeated keys include `world.entity.0.*`,
  `commandLog.record.0.*`, and `inventory.player.0.stack.0.*`;
- encoded save includes exact keys for metadata/session identity,
  `world.entity.0.localBounds.min`, `clock.tickIndex`,
  absence of `clock.stepRequested`, `camera.inputClearRequested` absence,
  `session.nextCommandId`, `commandLog.nextSequence`, `commandLog.epoch`, and
  `objectives.record.0.status`;
- save/load tests assert flattened canonical entity interaction fields
  round-trip from `EntityState`, including `interactionKind`,
  `interactionPrimaryEffect`, `interactionItemId`, `interactionItemCount`,
  `interactionObjectiveId`, `interactionRepeatable`, and
  `interactionDeactivateTargetOnSuccess`;
- codec tests cover empty strings and escaped `%`, LF, CR, and `=` values;
- decode rejects duplicate keys, missing required keys, unsupported version,
  invalid enum, invalid number, invalid id/sequence, invalid section order, and
  `SaveTooLarge`;
- decode rejects unknown keys and malformed percent escapes;
- `SaveEncodeResult` exposes exact `SaveCodecStatus`, encoded text, saved hash,
  and diagnostic section/key/line/offset fields;
- `SaveDecodeResult` exposes exact `SaveCodecStatus`, decoded envelope on `Ok`,
  duplicate/missing field context, and diagnostic key/line/offset fields;
- compatible load into a fresh session produces the source hash and observable
  runtime facts and returns `SaveLoadStatus::Ok`;
- saving an invalid source state returns `SaveLoadStatus::InvalidSourceState`;
- encoded save failure returns `SaveLoadStatus::EncodeFailed`, preserves codec
  status/diagnostics, leaves encoded text invalid, and does not mutate the source
  session;
- malformed encoded load input returns `SaveLoadStatus::DecodeFailed`, preserves
  codec status/diagnostics, leaves `loadedHash` invalid, and leaves the
  destination session unchanged;
- package, scenario, schema, and runtime-version incompatibility return
  `SaveLoadStatus::CompatibilityFailed`, preserve the exact
  `SaveCompatibilityStatus`, and reject before candidate mutation;
- metadata/session package mismatch, metadata/session scenario mismatch, and
  session current tick/clock tick index mismatch return
  `SaveLoadStatus::InvalidEnvelope`;
- missing entity, player slot, item, objective, actor, controlled actor, target,
  or command target references return `SaveLoadStatus::InvalidReference`;
- duplicate command ids, invalid/non-monotonic command sequences, invalid
  admission/rejection pairs, invalid retry source links, and invalid
  `nextSequence`/`epoch` command-log restore data return
  `SaveLoadStatus::InvalidCommandLog`;
- zero `session.nextCommandId`, or `session.nextCommandId` not greater than every
  restored command record `commandId`, returns `SaveLoadStatus::InvalidEnvelope`;
- `load_restores_next_command_id_for_future_submission`: build/load a save whose
  restored command log has a known highest `CommandRecord::commandId`, restore
  `SessionState::nextCommandId` to the next valid id, then submit one gameplay
  command through normal `Session::submitCommand`; the stored new command
  `commandId` equals the restored cursor value, does not collide with any
  restored command id, and successful `CommandLog::append` advances
  `SessionState::nextCommandId` by one;
- the same post-load cursor test must inject or simulate an unrecordable
  `CommandLogAppendStatus` failure through the session boundary and assert the
  command-id cursor does not advance when append fails before storage;
- hash mismatch returns `SaveLoadStatus::HashMismatch`, leaves the destination
  session unchanged by pre-load hash and key facts, and reports the recomputed
  candidate hash as `loadedHash`;
- an already validated candidate rejected by the session replacement boundary
  returns `SaveLoadStatus::ReplacementFailed`, preserves the exact
  `SessionLoadStatus`, and leaves the destination unchanged;
- every non-`Ok` load status leaves the destination session unchanged by
  pre-load hash and key facts;
- `loadedHash` is valid only for `SaveLoadStatus::Ok` and
  `SaveLoadStatus::HashMismatch`; other load failures leave it `0`/invalid;
- command-log restore tests prove non-monotonic sequence and duplicate command id
  are rejected before mutation;
- load clears excluded transient pending-step (`ClockState::stepRequested`),
  input-clear, projection, events, metrics, and summary state according to the
  save/load docs;
- malformed codec input returns structured decode errors without mutation;
- reset branch followed by load proves baseline and loaded branch remain
  separate deterministic states.

Tests use public runtime/save APIs and fixed fixtures only. No test writes
production fixture files outside an explicit golden update workflow.

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime stays small; fixture-level tests may scan all demo entities and commands.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use exact save/load, codec, compatibility, and session-load status values for
  expected failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- tests assert the named save sections and exclusions without owning runtime
  serialization policy;
- replay-sensitive command log and hash assertions are regression coverage only;
- no test may treat projection/events/metrics/summary as save truth.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/save_load_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
