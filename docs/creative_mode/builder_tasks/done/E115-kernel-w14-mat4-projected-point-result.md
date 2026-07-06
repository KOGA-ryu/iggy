# E115: Kernel W14 - Mat4 Projected Point Result

## Objective

Add a core `Mat4` projection result helper that computes clip x/y/z/w once and
preserves current `transformPoint(const Mat4&, Vec3)` behavior.

This is a core-only contract slice. Do not route standalone picking or render
callers yet.

## Current Seam

E114 found:

- `transformPoint(const Mat4&, Vec3)` computes clip x/y/z/w.
- It divides x/y/z by `w` only when `w` is finite and not `0` and not `1`.
- For `w == 1`, `w == 0`, or non-finite `w`, it returns raw x/y/z.
- Negative finite `w` is divided normally.
- The function does not expose raw `w`, validity, or failure reason.

## Required Work

1. Add a small result type in `src/core/math/Mat4.hpp`.
   - Suggested shape:
     ```cpp
     struct ProjectedPoint3 {
       Vec3 ndc{};
       float w = 1.0F;
       bool finite = false;
     };
     ```
   - `finite` must mean returned `ndc` x/y/z and raw `w` are finite.
   - Do not encode `w > 0` into the core result. That is consumer policy.
2. Add a helper:
   - Suggested name: `projectPoint(const Mat4&, Vec3)`.
   - It computes clip x/y/z/w once.
   - It fills `.ndc` using the exact current `transformPoint(Mat4)` divide /
     fallback rules.
   - It fills `.w` with raw clip `w`.
   - It fills `.finite` after `.ndc` is computed.
3. Keep `transformPoint(const Mat4&, Vec3)` as a compatibility wrapper that
   returns `projectPoint(...).ndc`.
4. Add core math tests in `tests/unit/math_tests.cpp` for:
   - finite `w != 0 && w != 1` divides;
   - `w == 1` returns raw/same values;
   - `w == 0` preserves current raw x/y/z compatibility and marks
     `finite=true` if x/y/z and w are finite;
   - non-finite `w` preserves current raw x/y/z compatibility and marks
     `finite=false`;
   - negative finite `w` divides and leaves front/behind policy to callers.

## Do Not

- Do not route `StandalonePicking`, standalone labels, `RenderLoop`, Vulkan,
  projection, or UI call sites in this slice.
- Do not change `Mat4` public matrix layout or multiplication.
- Do not change `Transform3`, `OrientedBox`, save/load, render, input,
  Creative, or RoomBake behavior.
- Do not rename/remove `transformPoint(Mat4, ...)`.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^math_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Helper/API shape:
- Compatibility behavior:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/core/math/Mat4.hpp`
  - `src/core/math/Mat4.cpp`
  - `tests/unit/math_tests.cpp`
- Helper/API shape:
  - Added `ProjectedPoint3 { Vec3 ndc; float w; bool finite; }`.
  - Added `projectPoint(const Mat4&, Vec3)` to compute clip x/y/z/w once, return raw clip `w`, and report whether the returned NDC plus raw `w` are finite.
  - `finite` is only a finiteness flag; it does not encode front/behind or `w > 0` policy.
- Compatibility behavior:
  - `projectPoint(...)` preserves the existing `transformPoint(Mat4, ...)` divide/fallback rules exactly: divide only when `w` is finite and not `0` and not `1`; otherwise return raw x/y/z.
  - `transformPoint(const Mat4&, Vec3)` now delegates to `projectPoint(...).ndc`.
  - No standalone picking, render, UI, Transform3, OrientedBox, Creative, or RoomBake callers were routed in this slice.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d math_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^math_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n '[[:blank:]]$' /Users/kogaryu/iggy3d/src/core/math/Mat4.hpp /Users/kogaryu/iggy3d/src/core/math/Mat4.cpp /Users/kogaryu/iggy3d/tests/unit/math_tests.cpp || true`
- Concerns/deferred:
  - Future W15-style routing can consume `projectPoint(...)` from standalone picking/labels/render projection, but this card intentionally left all consumers unchanged.
