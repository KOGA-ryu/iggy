# E116: Kernel W15 - Standalone Projection Routing

## Objective

Route standalone creative screen-projection helpers through the E115 core
`iggy3d::projectPoint(...)` result so standalone code stops duplicating clip-W
math.

This is a consumer-routing slice for the standalone creative app only. It should
not change render/Vulkan projection behavior.

## Current Seam

E114 found duplicated projection work in the standalone app:

- `apps/iggy3d_creative/StandalonePicking.cpp` has a local `clipW(...)`
  helper that manually computes matrix row 3.
- `projectBoxToScreen(...)` currently calls local `clipW(...)`, then calls
  `iggy3d::transformPoint(...)`, which recomputes clip x/y/z/w.
- `projectPointToScreen(...)` does the same.
- `buildPathPointHandleHits(...)` uses `clipW(...)` for handle depth.
- `apps/iggy3d_creative/main.cpp` imports `clipW` and uses it with
  `transformPoint(...)` for the selected-object dimension label.

E115 added the correct core primitive:

- `iggy3d::ProjectedPoint3 projectPoint(const Mat4&, Vec3)`
- `.ndc` preserves existing `transformPoint(Mat4)` divide/fallback semantics.
- `.w` exposes raw clip W.
- `.finite` is finiteness only and does not encode `w > 0`.

## Required Work

1. In `StandalonePicking.cpp`, route projection helpers through
   `iggy3d::projectPoint(...)`.
   - `projectBoxToScreen(...)` should call `projectPoint(...)` once per corner.
   - `projectPointToScreen(...)` should call `projectPoint(...)` once.
   - Preserve the existing standalone front/behind policy:
     - skip projected points when raw `w` is non-finite or `w <= 0`;
     - keep existing x/y finite checks for screen coordinates.
   - Do not move `w > 0` into core.
2. Remove or reduce the local `clipW(...)` duplication.
   - Prefer deleting the public standalone `clipW(...)` helper if all callers
     can consume `projectPoint(...)` or existing screen helpers directly.
   - If a compatibility wrapper is temporarily simpler, it must delegate to
     `projectPoint(...).w` and must not duplicate matrix row math.
3. Route path-handle center depth through the core result.
   - `centerDepth` should remain raw clip `w` ordering, matching the existing
     behavior.
4. Route the selected-object dimension label in
   `apps/iggy3d_creative/main.cpp`.
   - Remove `using iggy3d_creative_app::clipW;` if no longer needed.
   - Use `iggy3d::projectPoint(...)` directly, or reuse
     `projectPointToScreen(...)` only if behavior is intentionally equivalent.
   - Preserve the existing label front/behind policy of finite raw `w` and
     `w > 0`. Do not add a new behind-camera policy.
5. Add or update focused tests only where useful.
   - Prefer `standalone_picking_tests` guards that prove non-unit/negative/
     zero/non-finite `w` policies remain consumer-owned and that projected
     screen points still match previous coordinates.
   - Do not add broad render tests in this slice.

## Do Not

- Do not route `src/render/vulkan/RenderLoop.cpp` or product render overlays.
- Do not change `Mat4`, `Transform3`, `OrientedBox`, RoomBake, Creative
  document/model policy, save/load, mutation, input routing, or renderer
  backend behavior.
- Do not change consumer `w > 0` policy into core validity.
- Do not launch a window or run broad CTest.
- Do not stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests math_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|math_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Routing shape:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `apps/iggy3d_creative/StandalonePicking.hpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `tests/unit/standalone_picking_tests.cpp`
- Routing shape:
  - Removed the standalone public `clipW(...)` helper and its duplicated matrix row-3 math.
  - Routed `projectBoxToScreen(...)` through `iggy3d::projectPoint(...)` once per corner.
  - Routed `projectPointToScreen(...)` through `iggy3d::projectPoint(...)` once per point.
  - Routed path point handle `centerDepth` through `iggy3d::projectPoint(...).w`.
  - Routed the selected-object dimension label in `main.cpp` through `iggy3d::projectPoint(...)`.
- Behavior preserved:
  - Standalone still owns the front/behind policy: projected points are skipped when raw clip `w` is non-finite or `w <= 0`.
  - Screen-coordinate validity still checks finite NDC x/y only, matching the previous helper behavior.
  - Path handle depth ordering still uses raw clip `w`.
  - No render/Vulkan/product projection callers were routed.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests math_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|math_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n '[[:blank:]]$' /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePicking.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePicking.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/tests/unit/standalone_picking_tests.cpp || true`
- Concerns/deferred:
  - Render/Vulkan projection helpers still have their own policy and were intentionally left for a later card.
