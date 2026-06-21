# `src/core/math/Plane.cpp`

Updated: 2026-06-20

Exact purpose: implement a 3D plane helper for tactical camera, debug projection, and later picking math.

## Build Position

- priority rank: 23
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- normal
- distance/offset representation
- distance-to-point helper
- ray intersection helper

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

- normalized normal
- signed distance or point+normal with documented convention

## Semantics

- pure math only
- no camera or world ownership

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers, validation predicates, and declared math value-result
   structs such as `RayPlaneHit` only; do not return `Diagnostic`, `Result`,
   generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1).
- Signed distance and ray intersection are fixed-cost operations.

## Diagnostics And Errors

- plane helpers do not emit diagnostics;
- callers validate plane and ray inputs before use;
- invalid plane context is reported by the owning camera/projection/targeting
  subsystem.

## Save Replay Multiplayer Notes

- `Plane` is derived math for camera/projection/targeting consumers in the
  first build, not durable runtime save truth.
- If future runtime state stores planes, the owning aggregate must define
  durable fields and hash order.
- This file owns pure plane math only.

## Tests And Verification

- `math_tests` covers signed distance, invalid normal rejection, ray hit, and
  parallel/no-hit behavior;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Plane.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Plane.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- valid plane requires finite nonzero normal and finite distance;
- signed distance is deterministic dot + distance;
- ray-plane intersection is implemented as `intersectRayPlane`.
- `intersectRayPlane` computes denominator `dot(plane.normal, ray.direction)`;
  if `abs(denominator) <= 0.000001F`, return `{hit=false, t=0.0F, point={}}`
  without dividing;
- otherwise compute `t = -signedDistance(plane, ray.origin) / denominator`;
  negative `t` returns no hit; nonnegative `t` returns hit and
  `pointAt(ray, t)`.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1). Matrix multiplication is fixed-size O(1) with explicit 4x4 loops or unrolled code.

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
