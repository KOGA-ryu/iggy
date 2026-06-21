# `src/core/math/Ray3.hpp`

Updated: 2026-06-20

Exact purpose: declare a 3D ray for target discovery, camera debug projection, and future renderer picking consumers.

## Build Position

- priority rank: 20
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

## Ownership

This file owns:

- origin
- normalized or documented direction
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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1).
- Validation and `pointAt` are fixed-cost operations.

## Diagnostics And Errors

- `Ray3` helpers do not emit diagnostics;
- callers validate with `isFinite` and `isValid` before using rays for
  targeting, picking, or projection;
- invalid ray context is reported by the owning higher-level subsystem.

## Save Replay Multiplayer Notes

- `Ray3` is not first-build save truth.
- If future runtime state stores rays, the owning aggregate must define durable
  fields and hash order.
- This file owns representation and pure ray math only.

## Tests And Verification

- `math_tests` covers ray validity, finite rejection, zero-direction rejection,
  and `pointAt`;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/math/Ray3.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Ray3` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Ray3.hpp`.

Required semantics:
- fields: `Vec3 origin`, `Vec3 direction`;
- direction must be finite and nonzero for valid rays;
- helpers: pointAt(t), finite/valid check.

Required signatures:

```cpp
struct Ray3 {
  Vec3 origin;
  Vec3 direction;
};

Ray3 makeRay3(Vec3 origin, Vec3 direction);
bool isFinite(const Ray3& ray);
bool isValid(const Ray3& ray);
Vec3 pointAt(const Ray3& ray, float t);
```

`pointAt` returns `origin + direction * t`. Direction is not normalized by
`makeRay3`; callers that require normalized rays must normalize before
construction in their own domain.

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
`tests/unit/math_tests.cpp` must cover construction, finite validation,
zero-direction invalidation, and `pointAt(ray, t)`.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
