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
- transform-point helper

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

- row-major storage in `std::array<float, 16>`
- multiplication helpers
- point/vector transform helpers

## Semantics

- matrix code is pure math
- runtime gameplay truth remains in `Transform3` and `CameraState`
- renderer can consume matrices later but cannot mutate runtime through them

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers and validation predicates only; do not return
   `Diagnostic`, `Result`, generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1) fixed 4x4 operations.
- Matrix multiply uses fixed 4x4 loops; transform point and validation are
  fixed-cost operations.

## Diagnostics And Errors

- `Mat4` helpers do not emit diagnostics;
- callers validate with `isFinite` before accepting derived projection or
  camera math;
- invalid matrix context is reported by camera/projection code.

## Save Replay Multiplayer Notes

- `Mat4` is derived math for projection/camera consumers in the first build,
  not durable runtime save truth.
- Durable camera state remains in `CameraState`; matrices are regenerated from
  authoritative values.
- This file owns row-major representation and pure helpers only.

## Tests And Verification

- `math_tests` covers row-major indexing, identity, translation, scale,
  multiplication order, transform point, and finite validation;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Mat4.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Mat4.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- implement row-major storage where element `(row,column)` is `m[row * 4 + column]`;
- identity has diagonal 1;
- multiplication computes `out[row,column] = sum(lhs[row,k] * rhs[k,column])`;
- transform point treats input as `(x,y,z,1)`, computes row-major matrix times
  column vector, and divides by `w` only when `w` is finite and not exactly
  `0.0F` or `1.0F`;
- no renderer/GPU ownership.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1). Matrix multiplication is fixed-size O(1) with explicit 4x4 loops or unrolled code.

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
