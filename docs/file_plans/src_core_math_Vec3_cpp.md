# `src/core/math/Vec3.cpp`

Updated: 2026-06-20

Exact purpose: implement the basic 3D vector value used by gameplay positions, camera targets, rays, bounds, and projections.

## Build Position

- priority rank: 15
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- component storage
- basic arithmetic
- length/distance helpers
- finite-value validation

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

- `float x`, `float y`, `float z`
- zero/unit helpers
- epsilon comparison only for tests and validation

## Semantics

- coordinate system is X right, Y up, Z forward
- runtime gameplay uses explicit commands rather than frame-rate integration
- state hash quantizes vectors through `StableHash`

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers and validation predicates only; do not return
   `Diagnostic`, `Result`, generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1) per operation.
- Operations do not allocate and do not depend on container size.

## Diagnostics And Errors

- `Vec3` helpers do not emit diagnostics;
- callers validate with `isFinite` before accepting package, save, replay, or
  runtime state;
- invalid vector context is reported by the owning higher-level subsystem.

## Save Replay Multiplayer Notes

- `Vec3` is a value component inside save/replay/runtime owners such as
  transforms, bounds, rays, and camera state.
- This file defines representation and validation only; durable ownership is
  determined by the aggregate that stores the vector.
- State hash uses stable quantization through `StableHash`, not raw float bytes.

## Tests And Verification

- `math_tests` covers construction, operators, finite validation, dot, length,
  distance, and epsilon comparison;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Vec3.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Vec3.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- implement dot/length/distance using deterministic float operations;
- implement `nearlyEqual` as per-component absolute difference `<= epsilon`;
- scalar division divides each component by the scalar; callers must not pass
  zero because the function does not emit diagnostics;
- `isFinite` must reject NaN and infinities.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1). Matrix multiplication is fixed-size O(1) with explicit 4x4 loops or unrolled code.

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
