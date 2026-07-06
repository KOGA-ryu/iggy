# E113: Kernel W12 - Shared Euler Rotation Helper

## Objective

Remove the duplicated private Euler rotation math now present in
`Transform3.cpp` and `OrientedBox.cpp` by introducing one small core math helper.

This is a core-kernel primitive cleanup. It should preserve behavior exactly and
make the rotation convention explicit in one place.

## Current Seam

After E112:

- `src/core/math/Transform3.cpp` has a private `rotateEuler(...)` used by
  `transformPointTrs(...)`.
- `src/core/math/OrientedBox.cpp` has a private `rotateEuler(...)` used by
  oriented box corners, world AABB, contains, and ray tests.
- Both are intended to use the same convention:
  - intrinsic X-then-Y-then-Z,
  - `R = Rz * Ry * Rx`,
  - Y-up.

## Required Work

1. Add a shared helper under `src/core/math/`.
   - Suggested files: `EulerRotation.hpp/.cpp` or `Rotation3.hpp/.cpp`.
   - Suggested API:
     - `Vec3 rotateEulerXyz(Vec3 point, Vec3 eulerRadians);`
   - Use repo naming/style if a clearer name already exists.
2. Route both `Transform3.cpp` and `OrientedBox.cpp` through the shared helper.
3. Preserve all existing `Transform3`, `OrientedBox`, and `Mat4` public APIs.
4. Add focused tests for the shared helper:
   - identity rotation returns the same point;
   - +90 degrees around Y maps the existing E112 expected vector consistently;
   - at least one combined-axis case proves the order is X then Y then Z, not
     a commuted rotation.
5. Keep existing `math_tests` and `oriented_box_tests` behavior green.

## Do Not

- Do not introduce quaternions, matrices, or a broad transform system.
- Do not change `Transform3::transformPoint(...)` compatibility behavior.
- Do not change `OrientedBox` semantics.
- Do not touch runtime collision policy, render, save/load, Creative
  descriptors, RoomBake, standalone app, or input.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests oriented_box_tests entity_hit_query_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(math_tests|oriented_box_tests|entity_hit_query_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Shared helper/API shape:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

---

## Completion Brief - 2026-07-06

- Files changed:
  - `CMakeLists.txt`
  - `src/core/math/EulerRotation.hpp`
  - `src/core/math/EulerRotation.cpp`
  - `src/core/math/Transform3.cpp`
  - `src/core/math/OrientedBox.hpp`
  - `src/core/math/OrientedBox.cpp`
  - `tests/unit/math_tests.cpp`

- Shared helper/API shape:
  - Added `Vec3 rotateEulerXyz(Vec3 point, Vec3 eulerRadians)` under
    `src/core/math/EulerRotation.*`.
  - The helper owns the shared convention:
    - intrinsic X-then-Y-then-Z Euler;
    - `R = Rz * Ry * Rx`;
    - Y-up.
  - Added `EulerRotation.cpp` to the core `iggy3d` source list in
    `CMakeLists.txt`.
  - Routed `Transform3::transformPointTrs(...)` through `rotateEulerXyz(...)`.
  - Routed `OrientedBox` local-point placement through `rotateEulerXyz(...)`.
  - Kept all `Transform3`, `OrientedBox`, and `Mat4` public APIs unchanged.

- Behavior preserved:
  - Removed the duplicated private Euler rotation implementations from
    `Transform3.cpp` and `OrientedBox.cpp`.
  - `transformPoint(const Transform3&, Vec3)` still remains a compatibility
    wrapper to `transformPointScaleTranslate(...)`.
  - `OrientedBox` source behavior was unchanged apart from consuming the shared
    helper; existing oriented-box tests still pass.
  - `Mat4` overloads and projection/render call sites were untouched.

- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests oriented_box_tests entity_hit_query_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(math_tests|oriented_box_tests|entity_hit_query_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files.

- Concerns/deferred:
  - No matrix/quaternion or broader transform system was introduced.
  - If future kernels need inverse Euler rotation or basis extraction, that
    should be added deliberately to the same small core math seam rather than
    reintroducing private duplicated rotation code.
