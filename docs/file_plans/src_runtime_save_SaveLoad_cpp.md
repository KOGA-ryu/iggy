# `src/runtime/save/SaveLoad.cpp`

Updated: 2026-06-20

Exact purpose: implement conversion between `SessionState` and `SaveEnvelope` plus safe application of loaded state.

## Build Position

- priority rank: 117
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `source`

## Ownership

This file owns:

- session-to-envelope mapping
- envelope-to-session mapping
- load transaction semantics
- round-trip hash verification

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

- captures save truth sections only
- rebuilds derived summaries after load
- preserves command log needed for replay

## Semantics

- load is all-or-nothing
- failed load leaves existing session unchanged
- save/load does not require renderer or old iggy data

## Detailed Design Contract

Save algorithm:

1. validate source `SessionState`; invalid durable structure returns
   `SaveLoadStatus::InvalidSourceState`;
2. copy durable `SessionState` data into `SaveEnvelopeMetadata`,
   `SaveSessionSection`, `SaveWorldSection`, `SavePlayerSection`,
   `SaveClockSection`, `SaveCameraSection`, `SaveCommandLogSection`,
   `SaveInventorySection`, `SaveCombatSection`, `SaveAiSection`, and
   `SaveObjectiveSection`;
3. exclude events, metrics, projection, summary, renderer/app/raw input state;
4. compute and store `StateHash` for the copied durable payload;
5. for `saveSessionState`, return `SaveLoadStatus::Ok` with envelope and hash
   without file IO;
6. for `saveSessionStateEncoded`, call `SaveCodec::encodeSaveEnvelope` after
   envelope construction; codec failure returns `SaveLoadStatus::EncodeFailed`
   with codec status/diagnostics and without producing partial encoded text;
7. encoding success returns `SaveLoadStatus::Ok` with envelope, encoded text, and
   hash.

Load algorithm:

1. record destination pre-load hash;
2. for `loadEncodedSaveIntoSession`, call `SaveCodec::decodeSaveEnvelope`;
   codec failure returns `SaveLoadStatus::DecodeFailed` before candidate
   construction or destination mutation;
3. run `SaveCompatibility`; any non-`Compatible` result returns
   `SaveLoadStatus::CompatibilityFailed` and preserves the exact
   `SaveCompatibilityStatus`;
4. construct a complete candidate `SessionState` from metadata, session, world,
   players, clock, camera, commandLog, inventory, combat, ai, and objectives;
   rebuild candidate `BaselineSnapshot` from package/scenario seed data before
   validation;
5. validate deterministic ids, player roster, and section references, including
   slot actor ids, entity ids, item/objective ids, and command targets; missing
   or invalid references return `SaveLoadStatus::InvalidReference`;
6. validate metadata/session package id equality, metadata/session scenario id
   equality, and session current tick/clock tick index equality; mismatches
   return `SaveLoadStatus::InvalidEnvelope`;
7. restore the command-log section through `CommandLog::restoreForLoad(records,
   nextSequence, epoch)`; any status other than `Restored` returns
   `SaveLoadStatus::InvalidCommandLog`;
8. validate `SaveSessionSection.nextCommandId` is nonzero and greater than every
   restored command record `commandId`; violations return
   `SaveLoadStatus::InvalidEnvelope` before session replacement;
9. recompute candidate hash and compare with `metadata.savedStateHash` and
   `metadata.savedStateHashHex`; mismatches return
   `SaveLoadStatus::HashMismatch` and set `loadedHash` to the recomputed
   candidate hash;
10. set excluded transient state on the candidate according to `SessionState`
   rules before replacement:
   `ClockState::stepRequested=false`, `CameraState::inputClearRequested=false`,
   projection/debug projection empty, summary regenerated, events empty, metrics
   reset, and pending execution queues empty;
11. call `Session::replaceStateFromLoad` once with the complete candidate; if
   it returns any `SessionLoadStatus` other than `Ok`, return
   `SaveLoadStatus::ReplacementFailed`, preserve the exact session load status
   and diagnostic detail, and leave the destination unchanged;
12. return `SaveLoadStatus::Ok` with the loaded hash from `SessionLoadResult`.

All errors are all-or-nothing failures. The implementation must not partially
merge inventories, entities, objectives, command log, camera, or clock state.
For every failure after a destination session is supplied, `previousHash` is the
destination pre-load hash. `loadedHash` is valid only for `Ok` and
`HashMismatch`. `DecodeFailed` happens before candidate construction and leaves
the destination unchanged.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return the exact `SaveLoadStatus` and structured diagnostics for expected
   failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(session state size).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- return `SaveLoadStatus` for every expected save/load failure.
- populate only the diagnostics and source status fields declared by
  `SaveStateResult` and `LoadStateResult`; do not add a second result mechanism
  in the implementation.
- logging text is never the only machine-readable outcome.

## Save Replay Multiplayer Notes

- save/load maps every named `SaveEnvelope` section to or from `SessionState`;
- load regenerates excluded projection/debug/summary/transient state;
- replay relies on the saved command log and loaded hash matching normal session
  execution.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/save/SaveLoad.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
