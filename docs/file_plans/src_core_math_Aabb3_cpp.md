# `src/core/math/Aabb3.cpp`

Updated: 2026-06-20

Exact purpose: implement axis-aligned bounds for collision/debug geometry, target metadata, broadphase-friendly collision, and scene projection.

## Build Position

- priority rank: 19
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- min/max storage
- validity checks
- contains/intersects helpers
- closest-point helper

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

- `Vec3 min`
- `Vec3 max`
- no empty/invalid factory in the first complete build

## Semantics

- bounds are world-space unless a caller explicitly documents local-space conversion
- AABB closest-point helpers are geometry utilities; complete-build reach does
  not use bounds and is defined by `ReachQuery`
- no rotated-box behavior is hidden here

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers and validation predicates only; do not return
   `Diagnostic`, `Result`, generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- Validation, center, extents, containment, intersection, and closest-point
  helpers are O(1).

## Diagnostics And Errors

- `Aabb3` helpers do not emit diagnostics.
- Callers validate with `isFinite` and `isValid` before accepting package,
  save, replay, or runtime state.
- Invalid bounds context is reported by content validation or world state code.

## Save Replay Multiplayer Notes

- `Aabb3` becomes save/replay truth only when stored by authoritative owners
  such as `EntityState`.
- This source owns pure bounds math only.
- State hash must quantize min/max vectors through `StableHash`.

## Tests And Verification

- `math_tests` covers valid/invalid bounds, finite rejection, center, extents,
  containment, and closest-point clamping;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Aabb3.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Aabb3.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- valid bounds require finite min/max and min <= max per axis;
- closest point clamps independently per axis;
- contains uses inclusive bounds;
- intersects returns false for invalid inputs and otherwise uses inclusive
  overlap on x/y/z, so touching faces count as intersection;
- no allocation.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1).

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
