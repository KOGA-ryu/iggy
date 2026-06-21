# `src/core/math/Aabb3.hpp`

Updated: 2026-06-20

Exact purpose: declare axis-aligned bounds for collision/debug geometry, target metadata, broadphase-friendly collision, and scene projection.

## Build Position

- priority rank: 18
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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
- empty/invalid factory if useful

## Semantics

- bounds are world-space unless a caller explicitly documents local-space conversion
- AABB closest-point helpers are geometry utilities; complete-build reach does
  not use bounds and is defined by `ReachQuery`
- no rotated-box behavior is hidden here

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1).
- Validation, containment, extents, and closest-point helpers are fixed-cost
  operations.

## Diagnostics And Errors

- `Aabb3` helpers do not emit diagnostics;
- callers validate with `isFinite` and `isValid` before accepting package, save,
  replay, or runtime state;
- invalid bounds context is reported by content validation or world state code.

## Save Replay Multiplayer Notes

- `Aabb3` becomes save/replay truth only when stored by authoritative owners
  such as `EntityState`.
- This file owns representation and pure bounds helpers only.
- State hash must quantize min/max vectors through `StableHash`.

## Tests And Verification

- `math_tests` covers valid/invalid bounds, finite rejection, center, extents,
  containment, and closest-point clamping;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Aabb3.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Aabb3` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Aabb3.hpp`.

Required semantics:
- fields: `Vec3 min`, `Vec3 max`;
- factories: `makeAabb3(Vec3 min, Vec3 max)`, `aabbFromCenterExtents(Vec3 center, Vec3 extents)`;
- helpers: valid, finite, center, extents, contains point, intersects, closest point.

Required signatures:

```cpp
struct Aabb3 {
  Vec3 min;
  Vec3 max;
};

Aabb3 makeAabb3(Vec3 min, Vec3 max);
Aabb3 aabbFromCenterExtents(Vec3 center, Vec3 extents);
bool isFinite(const Aabb3& bounds);
bool isValid(const Aabb3& bounds);
Vec3 center(const Aabb3& bounds);
Vec3 extents(const Aabb3& bounds);
bool contains(const Aabb3& bounds, Vec3 point);
bool intersects(const Aabb3& a, const Aabb3& b);
Vec3 closestPoint(const Aabb3& bounds, Vec3 point);
```

`isValid` requires finite bounds and `min.x <= max.x`, `min.y <= max.y`,
`min.z <= max.z`. `closestPoint` clamps each point component into the inclusive
min/max range. `intersects` returns false if either input is not valid. For two
valid boxes it returns true when their inclusive ranges overlap on all three
axes, including faces that merely touch.

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
`tests/unit/math_tests.cpp` must cover min/max construction, center/extents,
finite validation, invalid min-greater-than-max rejection, inclusive contains,
inclusive intersection including touching faces, invalid-input intersection
returning false, and closest-point clamping.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
