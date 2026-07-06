# E112: Kernel W11 - Transform3 Explicit Point Helpers

## Objective

Implement the safe helper split recommended by E111.

Make `Transform3` point-transform semantics explicit without changing current
runtime behavior.

## Current Seam

E111 found:

- `transformPoint(const Transform3&, Vec3)` currently applies only component
  scale + translation.
- `Transform3::rotationEulerRadians` is honored by `OrientedBox`, not by this
  helper.
- Direct runtime caller `EntityHitQuery::worldBoundsForEntity(...)` expects the
  current scale+translate AABB behavior.
- `Mat4::transformPoint(...)` is a separate homogeneous projection overload and
  should not be changed in this slice.

## Required Work

1. Add explicit helper declarations/definitions in `src/core/math/Transform3.*`:
   - `transformPointScaleTranslate(const Transform3&, Vec3)`
   - `transformPointTrs(const Transform3&, Vec3)`
2. Keep existing `transformPoint(const Transform3&, Vec3)` as a compatibility
   wrapper that calls `transformPointScaleTranslate(...)`.
3. Implement `transformPointTrs(...)` using the same Euler convention as
   `OrientedBox`:
   - intrinsic X-then-Y-then-Z,
   - `R = Rz * Ry * Rx`,
   - Y-up,
   - `world = position + R * (scale . local)`.
4. Update direct `Transform3` call sites to explicit helpers:
   - `src/runtime/collision/EntityHitQuery.cpp` should call
     `transformPointScaleTranslate(...)`.
   - `tests/unit/math_tests.cpp` should test both explicit helpers.
   - Do not touch `Mat4` call sites.
5. Add guard coverage:
   - nonzero rotation is ignored by `transformPointScaleTranslate(...)`;
   - nonzero rotation is honored by `transformPointTrs(...)`;
   - compatibility `transformPoint(...)` still matches
     `transformPointScaleTranslate(...)`;
   - runtime entity hit-query behavior remains scale+translate for a nonzero
     rotation case.

## Important Constraints

- Do not change public struct layout or existing save/load/hash semantics.
- Do not change `transformPoint(const Transform3&, Vec3)` behavior in this
  slice.
- Do not change `Mat4` overloads or projection/render call sites.
- Do not change `OrientedBox` behavior except optionally reusing a shared helper
  if the diff is smaller and tests prove no change.
- Do not introduce matrix/quaternion systems.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Tests/Targets

Likely focused tests:

- `math_tests`
- `oriented_box_tests`
- `entity_hit_query_tests`

Suggested commands:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests oriented_box_tests entity_hit_query_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(math_tests|oriented_box_tests|entity_hit_query_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Helper/API shape:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

---

## Completion Brief - 2026-07-06

- Files changed:
  - `src/core/math/Transform3.hpp`
  - `src/core/math/Transform3.cpp`
  - `src/runtime/collision/EntityHitQuery.cpp`
  - `tests/unit/math_tests.cpp`
  - `tests/unit/entity_hit_query_tests.cpp`

- Helper/API shape:
  - Added `Vec3 transformPointScaleTranslate(const Transform3&, Vec3)`.
    - This is the explicitly named helper for the old/current behavior:
      component scale followed by translation.
    - It intentionally ignores `rotationEulerRadians`.
  - Added `Vec3 transformPointTrs(const Transform3&, Vec3)`.
    - This applies `world = position + R * (scale . local)`.
    - Euler convention matches `OrientedBox`: intrinsic X-then-Y-then-Z,
      `R = Rz * Ry * Rx`, Y-up.
  - Kept `Vec3 transformPoint(const Transform3&, Vec3)` as a compatibility
    wrapper to `transformPointScaleTranslate(...)`.
  - Did not change `Mat4::transformPoint(...)`, `Transform3` layout,
    save/load, hashing, or `OrientedBox`.

- Behavior preserved:
  - Runtime entity hit-query bounds now call
    `transformPointScaleTranslate(...)` directly.
  - Added a nonzero-rotation entity hit-query guard proving the runtime
    target bounds still ignore rotation and hit at the existing
    scale+translate AABB location.
  - Added math guards proving:
    - `transformPointScaleTranslate(...)` ignores nonzero rotation;
    - `transformPointTrs(...)` honors the same Y-rotation convention used by
      `OrientedBox`;
    - compatibility `transformPoint(...)` still matches
      `transformPointScaleTranslate(...)`.
  - Existing OrientedBox behavior was left untouched and its focused tests
    remained green.

- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests oriented_box_tests entity_hit_query_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(math_tests|oriented_box_tests|entity_hit_query_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files.

- Concerns/deferred:
  - `Transform3.cpp` now has a private Euler helper that intentionally mirrors
    `OrientedBox.cpp`. A later cleanup could extract a shared internal Euler
    primitive if more kernels need it, but this slice avoided changing
    `OrientedBox`.
  - Runtime entity bounds remain axis-aligned/scale+translate by policy. A
    future rotated-runtime-entity bounds feature should be explicit and should
    not reuse the compatibility `transformPoint(...)` name as the migration
    path.
