# `src/runtime/save/SaveLoad.hpp`

Updated: 2026-06-20

Exact purpose: declare conversion between `SessionState` and `SaveEnvelope` plus safe application of loaded state.

## Build Position

- priority rank: 116
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `header`

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

Declare:

```cpp
enum class SaveLoadStatus : std::uint8_t {
  Ok,
  InvalidSourceState,
  EncodeFailed,
  DecodeFailed,
  CompatibilityFailed,
  InvalidEnvelope,
  InvalidReference,
  InvalidCommandLog,
  HashMismatch,
  ReplacementFailed,
};
```

Declare `SaveStateResult` with `SaveLoadStatus status`, envelope, encoded save
text field, saved state hash, codec status, and diagnostics.
`SaveStateResult::status` uses:

- `Ok` when the source state maps to a valid envelope and, for encoded save
  entry points, encoding succeeds;
- `InvalidSourceState` when the source `SessionState` cannot be mapped to a
  durable `SaveEnvelope`;
- `EncodeFailed` when the envelope maps successfully but `SaveCodec` rejects or
  cannot encode it.

`SaveStateResult` field validity:

- `status == Ok`: envelope and `savedStateHash` are valid; encoded save text is
  valid for encoded save entry points and ignored for envelope-only entry
  points;
- `status == InvalidSourceState`: envelope, encoded save text, and
  `savedStateHash` are invalid/ignored;
- `status == EncodeFailed`: envelope and `savedStateHash` are valid for
  diagnostics, encoded save text is invalid/ignored, and codec status is valid.

Declare `LoadStateResult` with `SaveLoadStatus status`, previous hash, loaded
hash, compatibility status, session load status, and diagnostics.
`LoadStateResult::status` uses:

- `DecodeFailed` when encoded input cannot be decoded by `SaveCodec`;
- `CompatibilityFailed` when `SaveCompatibility` returns any status other than
  `Compatible`; preserve the exact `SaveCompatibilityStatus` in the result;
- `InvalidEnvelope` for malformed cross-section facts not owned by
  compatibility, including metadata/session package mismatch,
  metadata/session scenario mismatch, and session current tick/clock tick index
  mismatch, or `session.nextCommandId` being zero or not greater than every
  restored command record `commandId`;
- `InvalidReference` for missing or invalid entity, player slot, item,
  objective, actor, target, or controlled-actor references;
- `InvalidCommandLog` for duplicate command ids, invalid/non-monotonic command
  sequences, invalid command admission/rejection invariants, invalid retry
  source links, or invalid `nextSequence`/`epoch` restore data;
- `HashMismatch` when a fully mapped candidate recomputes to a hash that does
  not equal `metadata.savedStateHash` or `metadata.savedStateHashHex`;
- `ReplacementFailed` only if the session-owned all-or-nothing replacement
  rejects an already validated candidate; preserve the exact
  `SessionLoadStatus` in the result diagnostics/detail field;
- `Ok` only after replacement succeeds.

`LoadStateResult` field validity:

- `status == Ok`: `previousHash` and `loadedHash` are valid, and the destination
  session was replaced exactly once;
- `status == HashMismatch`: `previousHash`, expected saved hash, and actual
  candidate hash are valid for diagnostics, `loadedHash` is the actual candidate
  hash, and the destination session is unchanged;
- `status == DecodeFailed`: codec status and decode diagnostics are valid,
  `previousHash` is valid if a destination session was supplied, and
  `loadedHash` is invalid;
- every other non-`Ok` status: destination session is unchanged, `previousHash`
  is the pre-load hash, `loadedHash` is `0`/invalid unless explicitly named
  above, and diagnostics name the failing section.

Declare pure boundaries:

- `SaveStateResult saveSessionState(const SessionState& state)`;
- `SaveStateResult saveSessionStateEncoded(const SessionState& state)`;
- `LoadStateResult loadEnvelopeIntoSession(Session& session, const SaveEnvelope&
  envelope, const SaveCompatibilityRequest& request)`;
- `LoadStateResult loadEncodedSaveIntoSession(Session& session,
  std::string_view encodedSave, const SaveCompatibilityRequest& request)`.

Ownership:

- this file owns mapping between `SessionState` and `SaveEnvelope`;
- `SaveCodec` owns bytes;
- app tools own paths/files;
- `Session` owns the actual all-or-nothing state replacement;
- projection/debug/summary/renderer/raw input are excluded and regenerated.

Save mapping uses the `SaveEnvelope` sections by name: metadata, session, world,
players, clock, camera, commandLog, inventory, combat, ai, and objectives.

Load invariants: validate compatibility before mutation, build a complete
candidate `SessionState` from every named section plus a rebuilt
`BaselineSnapshot` from package/scenario seed data, validate cross-section
references, recompute the candidate state hash, compare it to
`metadata.savedStateHash` and `metadata.savedStateHashHex`, then replace via one
session-owned API call. Metadata/session package id, metadata/session scenario
id, and session current tick/clock tick index must match before replacement.
`session.nextCommandId` must be nonzero and greater than every restored command
record `commandId` before replacement. Any failure leaves the destination session
equivalent by hash and observable runtime facts.

The command-log section is restored through `CommandLog::restoreForLoad`.
Any restore status other than `CommandLogRestoreStatus::Restored` maps to
`SaveLoadStatus::InvalidCommandLog`.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(session state size).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- `SaveLoadStatus` is the machine-readable outcome for every save/load
  orchestration result.
- `SaveStateResult` and `LoadStateResult` carry all diagnostics and source
  status detail; no additional result channel is part of this header.
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

- `src/runtime/save/SaveLoad.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
