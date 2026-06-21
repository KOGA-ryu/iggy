# `src/runtime/save/SaveCompatibility.cpp`

Updated: 2026-06-20

Exact purpose: implement version compatibility checks for loading save envelopes.

## Build Position

- priority rank: 113
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `source`

## Ownership

This file owns:

- supported schema versions
- rejection diagnostics
- first-build compatibility status shape

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

- current schema version constant
- minimum compatible version
- package id compatibility check

## Semantics

- incompatible load rejects before mutating session
- first complete build has no save migration path

## Detailed Design Contract

Implementation algorithm:

1. validate envelope structural status for fields that are present in the
   decoded `SaveEnvelope`;
2. reject unsupported schema/runtime versions with exact status;
3. compare expected package and scenario ids when provided;
4. assume required hash fields were present because `SaveCodec` rejected
   missing required fields before envelope construction;
5. validate `metadata.savedStateHashHex` is exactly 16 lowercase hex digits and
   matches `formatStateHash(metadata.savedStateHash)`;
6. return `Compatible` only for the original envelope.

The implementation must not allocate candidate sessions, write migrated
envelopes, or call `Session::replaceStateFromLoad`.
It must not recompute payload hash or compare loaded runtime hash; that belongs
to `SaveLoad` after envelope-to-state mapping. All failure statuses are stable
machine-readable values used by save/load tests and CLI tools.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1) for first-build compatibility checks.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- compatibility validates envelope metadata only;
- save payload mapping and loaded-hash comparison belong to `SaveLoad`;
- replay observes only successfully loaded state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/save/SaveCompatibility.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
