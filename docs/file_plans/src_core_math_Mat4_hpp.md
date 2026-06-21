# `src/core/math/Mat4.hpp`

Updated: 2026-06-20

Exact purpose: declare a 4x4 matrix value for camera/projection math and renderer-facing scene projection.

## Build Position

- priority rank: 24
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/core/math/Mat4.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned API
Declare `Mat4` as a small value type in namespace `iggy3d`. The header owns only data fields, constructors/factory helpers, validation predicates, and pure math signatures needed by runtime.

Required repo path: `src/core/math/Mat4.hpp`.

Required semantics:
- storage: `std::array<float, 16> m` in row-major order;
- index convention: element at row `r`, column `c` is `m[r * 4 + c]`;
- vector convention: column vectors multiplied as `result = matrix * vector`;
- composition convention: `A * B` applies `B` first, then `A`;
- factories: identity, translation, scale, Euler rotation;
- helpers: multiply matrix, transform point, finite validation.

Required signatures:

```cpp
struct Mat4 {
  std::array<float, 16> m;
};

Mat4 identityMat4();
Mat4 translationMat4(Vec3 translation);
Mat4 scaleMat4(Vec3 scale);
Mat4 rotationEulerRadiansMat4(Vec3 rotationEulerRadians);
Mat4 operator*(const Mat4& lhs, const Mat4& rhs);
Vec3 transformPoint(const Mat4& matrix, Vec3 point);
bool isFinite(const Mat4& matrix);
float at(const Mat4& matrix, std::uint32_t row, std::uint32_t column);
```

Euler rotation uses the `Transform3` mapping `x=pitch`, `y=yaw`, `z=roll` and
applies roll, then pitch, then yaw through the composition convention above.

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
`tests/unit/math_tests.cpp` must cover row-major storage, `at(row,column)`,
identity, translation, scale, Euler rotation factory presence, matrix
multiplication order, transform-point homogeneous policy, and finite validation.

### Completion Criteria
A builder can implement the header without adding any `.cpp` dependencies outside core math and without consulting chat history.
