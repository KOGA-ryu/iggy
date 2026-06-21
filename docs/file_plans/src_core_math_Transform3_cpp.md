# `src/core/math/Transform3.cpp`

Updated: 2026-06-20

Exact purpose: implement the gameplay transform value used for entity placement and camera targets.

## Build Position

- priority rank: 17
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- position
- Euler-radian rotation policy
- scale if required by fixtures
- composition helpers

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

- position is always present
- rotation defaults to identity/yaw zero
- scale defaults to one and is not used for gameplay reach unless documented

## Semantics

- world owns entity transforms
- movement mutates transforms only through `WorldState` APIs
- save/load persists transforms exactly through codec rules

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. implement pure helpers and validation predicates only; do not return
   `Diagnostic`, `Result`, generic status values, or structured diagnostics;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1).
- Validation and point transform are fixed-cost operations.

## Diagnostics And Errors

- `Transform3` helpers do not emit diagnostics;
- callers validate with finite/scale helpers before accepting package, save,
  replay, or runtime state;
- invalid transform context is reported by content validation, world state, or
  save/load code.

## Save Replay Multiplayer Notes

- `Transform3` becomes save/replay truth only when stored by authoritative
  owners such as `EntityState` or `CameraState`.
- This file owns representation, defaults, and pure helpers only.
- State hash must quantize `position`, `rotationEulerRadians`, and `scale`
  through `StableHash`.

## Tests And Verification

- `math_tests` covers identity defaults, Euler field naming, finite validation,
  positive scale validation, and point transform behavior;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Transform3.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Scope
Implement the non-inline operations declared by `src/core/math/Transform3.hpp`. Keep simple constexpr constructors and trivial accessors in the header when appropriate; put operations with math functions or validation branches here.

Required behavior:
- implement identity transform factory with `position=(0,0,0)`,
  `rotationEulerRadians=(0,0,0)`, and `scale=(1,1,1)`;
- implement finite validation;
- implement `hasPositiveFiniteScale`;
- implement `transformPoint` as component-wise scale then translation:
  `position + localPoint * scale`; do not apply rotation in this helper;
- do not implement renderer matrix ownership here beyond pure conversion helpers.

### Failure Behavior
Core math functions do not emit diagnostics. They return finite numeric results for valid inputs and expose explicit validation helpers so callers can reject invalid package/runtime data before mutation.

### Compute Cost
All operations are O(1). Matrix multiplication is fixed-size O(1) with explicit 4x4 loops or unrolled code.

### Tests
`tests/unit/math_tests.cpp` must cover this implementation file through public API only.

### Completion Criteria
The source compiles into `iggy3d`, has no global mutable state, and does not include runtime, app, renderer, or old iggy headers.
