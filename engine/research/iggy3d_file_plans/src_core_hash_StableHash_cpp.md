# `src/core/hash/StableHash.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic non-cryptographic hashing for state hashes, replay verification, and fixture summaries.

## Build Position

- priority rank: 13
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- hash seed
- byte append rules
- primitive append helpers
- quantized float append policy

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- C++ standard library only unless explicitly justified
- no `src/runtime`, `src/content`, `src/projection`, or app includes

## Data Contract

- 64-bit FNV-1a or equivalent fixed algorithm documented in the header
- helpers for integers, bools, strings, ids, and quantized meters

## Semantics

- same logical state must hash identically on supported platforms
- floating point values are quantized to millimeters before hashing
- hash input order is explicitly sorted or stored order is deterministic

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(number of hashed fields plus bytes of strings).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use `Diagnostic` or stable status/rejection values for expected failures;
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

- `src/core/hash/StableHash.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
