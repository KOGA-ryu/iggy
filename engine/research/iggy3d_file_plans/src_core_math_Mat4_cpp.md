# `src/core/math/Mat4.cpp`

Updated: 2026-06-20

Exact purpose: implement a 4x4 matrix value for camera/projection math and renderer-facing scene projection.

## Build Position

- priority rank: 25
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- storage convention
- identity
- translation/rotation helpers
- look-at helper if needed

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

- row-major storage unless explicitly documented otherwise
- multiplication helpers
- point/vector transform helpers

## Semantics

- matrix code is pure math
- runtime gameplay truth remains in `Transform3` and `CameraState`
- renderer can consume matrices later but cannot mutate runtime through them

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1) fixed 4x4 operations.
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

- `src/core/math/Mat4.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
