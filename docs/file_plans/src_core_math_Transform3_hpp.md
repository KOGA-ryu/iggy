# `src/core/math/Transform3.hpp`

Updated: 2026-06-20

Exact purpose: declare the gameplay transform value used for entity placement and camera targets.

## Build Position

- priority rank: 16
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/core/math/Transform3.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Transform3` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Transform3.hpp`.

Required semantics:
- fields: `Vec3 position`, `Vec3 rotationEulerRadians`, `Vec3 scale`;
- default: position zero, rotation zero, scale one;
- rotation component meaning is exact: `x=pitch`, `y=yaw`, `z=roll`, all in
  radians;
- finite validation covers every component and requires scale components to be
  finite and positive.

Required signatures:

```cpp
struct Transform3 {
  Vec3 position;
  Vec3 rotationEulerRadians;
  Vec3 scale = {1.0F, 1.0F, 1.0F};
};

Transform3 identityTransform3();
bool isFinite(const Transform3& transform);
bool hasPositiveFiniteScale(const Transform3& transform);
Vec3 transformPoint(const Transform3& transform, Vec3 localPoint);
```

`transformPoint` for first build applies scale then translation only; rotation
conversion is owned by `Mat4` helpers so entity placement cannot hide renderer
matrix policy.

### Invariants
- All public helpers must be deterministic and side-effect free.
- Non-finite values are invalid for runtime state, package seed data, saves, replay, and state hash.
- No helper may read wall-clock time, random state, files, renderer state, or platform state.

### Dependencies
Allowed: standard math/fixed-width headers and lower-level core math headers.
Forbidden: runtime, content, app, projection, renderer, tests, old iggy.

### Save Replay Multiplayer
Values from this type may be durable state when stored inside entities, camera, bounds, rays, or projections. State hash must use stable quantization rather than raw binary representation.

### Tests
`tests/unit/math_tests.cpp` must cover identity defaults, exact
`rotationEulerRadians` component meaning, finite validation, positive scale
validation, and scale-then-translation `transformPoint`.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
