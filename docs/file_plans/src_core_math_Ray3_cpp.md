# `src/core/math/Ray3.cpp`

Updated: 2026-06-20

Exact purpose: implement a 3D ray for target discovery, camera debug projection, and future renderer picking consumers.

## Build Position

- priority rank: 21
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- origin
- documented non-normalized direction
- point-at-distance helper

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

- `Vec3 origin`
- `Vec3 direction`
- constructor or factory that rejects zero-length direction

## Semantics

- ray math is pure and has no world lookup
- targeting code decides which entities are tested against a ray

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers and validation predicates only; do not return
   `Diagnostic`, `Result`, generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- Validation and `pointAt` are O(1).

## Diagnostics And Errors

- `Ray3` helpers do not emit diagnostics.
- Callers validate with `isFinite` and `isValid` before using rays for
  targeting, picking, or projection.
- Invalid ray context is reported by the owning higher-level subsystem.

## Save Replay Multiplayer Notes

- `Ray3` is not first-build save truth.
- If future runtime state stores rays, the owning aggregate must define durable
  fields and hash order.
- This source owns pure ray math only.

## Tests And Verification

- `math_tests` covers ray validity, finite rejection, zero-direction rejection,
  and `pointAt`;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Ray3.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Ray3.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- `pointAt(t)` returns origin + direction * t;
- valid check rejects non-finite origin/direction and zero direction;
- no implicit renderer camera dependency.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1).

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
