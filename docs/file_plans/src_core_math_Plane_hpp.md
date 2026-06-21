# `src/core/math/Plane.hpp`

Updated: 2026-06-20

Exact purpose: declare a 3D plane helper for tactical camera, debug projection,
and ray intersection math.

## Build Position

- priority rank: 22
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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

- finite nonzero normal
- signed distance convention `dot(normal, point) + distance = 0`

## Semantics

- pure math only
- no camera or world ownership

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- Plane validation, signed distance, and ray intersection are O(1).

## Diagnostics And Errors

- `Plane` helpers do not emit diagnostics.
- Callers validate plane and ray inputs before use.
- Invalid plane context is reported by the owning camera/projection/targeting
  subsystem.

## Save Replay Multiplayer Notes

- `Plane` is derived math for camera/projection/targeting consumers in the
  first build, not durable runtime save truth.
- If future runtime state stores planes, the owning aggregate must define
  durable fields and hash order.
- This header owns pure plane math only.

## Tests And Verification

- `math_tests` covers plane construction, finite/valid checks, signed distance,
  invalid normal rejection, ray hit, and parallel/no-hit behavior;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Plane.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Plane` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Plane.hpp`.

Required semantics:
- fields: `Vec3 normal`, `float distance`;
- plane equation is dot(normal, point) + distance = 0;
- helpers: finite/valid, signed distance, construct from point and normal,
  ray intersection.

Required signatures:

```cpp
struct Plane {
  Vec3 normal;
  float distance = 0.0F;
};

struct RayPlaneHit {
  bool hit = false;
  float t = 0.0F;
  Vec3 point;
};

Plane makePlane(Vec3 normal, float distance);
Plane planeFromPointNormal(Vec3 point, Vec3 normal);
bool isFinite(const Plane& plane);
bool isValid(const Plane& plane);
float signedDistance(const Plane& plane, Vec3 point);
RayPlaneHit intersectRayPlane(const Ray3& ray, const Plane& plane);
```

`intersectRayPlane` returns no hit for parallel rays using epsilon
`0.000001F`, and no hit for negative `t`.

### Invariants
- All public helpers must be deterministic and side-effect free.
- Non-finite values are invalid for runtime state, package seed data, saves, replay, and state hash.
- No helper may read wall-clock time, random state, files, renderer state, or platform state.

### Dependencies
Allowed: standard math/fixed-width headers and lower-level core math headers.
Forbidden: runtime, content, app, projection, renderer, tests, old iggy.

### Save Replay Multiplayer
Plane values are derived math in the first complete build. If a future owner
stores planes as durable state, that owner defines save/hash field order.

### Tests
`tests/unit/math_tests.cpp` must cover construction, finite validation, signed
distance, ray hit, and parallel/no-hit behavior.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
