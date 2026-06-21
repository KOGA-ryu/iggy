# `src/runtime/save/SaveCompatibility.hpp`

Updated: 2026-06-20

Exact purpose: declare version compatibility checks for loading save envelopes.

## Build Position

- priority rank: 112
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `header`

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

Declare `enum class SaveCompatibilityStatus : std::uint8_t` with
`Compatible`, `UnsupportedSchemaVersion`, `UnsupportedRuntimeVersion`,
`PackageMismatch`, `ScenarioMismatch`, `HashMetadataMalformed`, and
`MalformedEnvelope`.

Declare `SaveCompatibilityRequest` with a const envelope reference, expected
package id, expected scenario id, current schema version, minimum readable
schema version, and current runtime save version.

Declare `SaveCompatibilityResult` with status, offending field name, expected
value, and actual value.

Invariants:

- compatibility check is read-only and performs no session mutation;
- version/package/scenario rejection happens before any state replacement;
- first complete build rejects unsupported schema/runtime/package/scenario
  rather than migrating;
- `SaveCodec` owns required-field presence; missing hash fields return
  codec `MissingField` before envelope construction;
- `metadata.savedStateHashHex` must be present in the decoded envelope and be
  exactly 16 lowercase hex digits matching the formatting shape of
  `metadata.savedStateHash`; malformed or mismatched present metadata returns
  `HashMetadataMalformed`;
- payload hash recompute and loaded-hash comparison belong to `SaveLoad`;
- future migration must be added by a later explicit plan and must not change
  first-build rejection behavior.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/runtime/save/SaveCompatibility.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
