# `src/core/math/Vec3.hpp`

Updated: 2026-06-20

Exact purpose: declare the basic 3D vector value used by gameplay positions, camera targets, rays, bounds, and projections.

## Build Position

- priority rank: 14
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/core/math/Vec3.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Vec3` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Vec3.hpp`.

Required semantics:
- fields: `float x`, `float y`, `float z`;
- factories: zero, unit X/Y/Z;
- operators: equality, add, subtract, scalar multiply, scalar divide;
- helpers: dot, lengthSquared, distanceSquared, finite check, epsilon compare;
- coordinate contract: X right, Y up, Z forward.

Required signatures:

```cpp
struct Vec3 {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

Vec3 vec3Zero();
Vec3 vec3UnitX();
Vec3 vec3UnitY();
Vec3 vec3UnitZ();
Vec3 operator+(Vec3 lhs, Vec3 rhs);
Vec3 operator-(Vec3 lhs, Vec3 rhs);
Vec3 operator*(Vec3 value, float scalar);
Vec3 operator*(float scalar, Vec3 value);
Vec3 operator/(Vec3 value, float scalar);
bool nearlyEqual(Vec3 lhs, Vec3 rhs, float epsilon = 0.0001F);
float dot(Vec3 lhs, Vec3 rhs);
float lengthSquared(Vec3 value);
float distanceSquared(Vec3 lhs, Vec3 rhs);
bool isFinite(Vec3 value);
```

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
`tests/unit/math_tests.cpp` must cover default construction, zero/unit helpers,
add, subtract, scalar multiply/divide, dot, length squared, distance squared,
epsilon comparison, and finite rejection.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
