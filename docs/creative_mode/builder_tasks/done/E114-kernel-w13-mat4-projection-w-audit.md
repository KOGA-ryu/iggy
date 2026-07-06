# E114: Kernel W13 - Mat4 Projection/W Semantics Audit

## Objective

Do a read-only decision packet for `Mat4` point projection and clip-`w`
semantics before adding or changing helpers.

This is a core-kernel audit card. The likely problem is that
`transformPoint(const Mat4&, Vec3)` performs a homogeneous transform and
perspective divide, while some callers separately compute clip `w` with local
helpers. Before implementation, map whether the core needs a result type that
returns both NDC and `w`.

## Scope

Read-only except moving this task card through the bucket and appending the
completion brief.

Do not edit source, tests, CMake, docs outside this task card, or commit.

## Required Reads

- `src/core/math/Mat4.hpp`
- `src/core/math/Mat4.cpp`
- `tests/unit/math_tests.cpp`
- `apps/iggy3d_creative/StandalonePicking.hpp`
- `apps/iggy3d_creative/StandalonePicking.cpp`
- `apps/iggy3d_creative/main.cpp` projection label call site
- `src/render/vulkan/RenderLoop.cpp` `projectWorldToScreen(...)`
- any other direct `transformPoint(const Mat4&, ...)` call sites found by `rg`

Useful scans:

```sh
rg -n "transformPoint\\(|clipW|clipFromWorld|projectPointToScreen|projectWorldToScreen" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit
rg -n "\\.w\\b|perspective|homogeneous|ndc|clip" /Users/kogaryu/iggy3d/src/core /Users/kogaryu/iggy3d/src/render /Users/kogaryu/iggy3d/apps/iggy3d_creative /Users/kogaryu/iggy3d/tests/unit
```

## Questions To Answer

1. What exactly does `transformPoint(const Mat4&, Vec3)` do today when `w` is:
   - finite and nonzero;
   - `1`;
   - `0`;
   - non-finite?
2. Which callers need the raw clip `w` in addition to NDC?
3. Which callers are doing duplicate clip-`w` math locally?
4. Would a helper such as `projectPoint(const Mat4&, Vec3)` or
   `transformPointHomogeneous(...)` reduce duplication without changing
   behavior?
5. What should happen for `w <= 0`, `w == 0`, and non-finite `w` in standalone
   picking and render overlay projection?

## Expected Analysis Shape

Produce a table like:

```text
Surface: Mat4 projection helper
File/function:
Current behavior:
Needs raw w:
Failure policy:
Duplicate math:
Suggested owner/API:
Recommended next slice:
```

## Likely Outcomes To Evaluate

Do not implement these yet; rank them.

- Add a small result type, e.g. `ProjectedPoint3 { Vec3 ndc; float w; bool valid; }`.
- Add a core helper that computes clip x/y/z/w once and applies the same
  perspective divide as current `transformPoint(...)`.
- Keep `transformPoint(Mat4, ...)` as compatibility wrapper.
- Route standalone `clipW(...)` / `projectPointToScreen(...)` and render
  `projectWorldToScreen(...)` only after guard tests pin current behind-camera
  and invalid-`w` behavior.

## Do Not

- Do not change `Mat4` behavior in this card.
- Do not alter projection, picking, render overlay, camera, Vulkan, standalone,
  or UI behavior.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

Read-only checks only:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Mat4 call-site table:
- Local clip-W duplicate findings:
- Failure-policy findings:
- Recommended next implementation slice:
- Risks/blockers:

---

## Completion Brief - 2026-07-06

- Files inspected:
  - `src/core/math/Mat4.hpp`
  - `src/core/math/Mat4.cpp`
  - `tests/unit/math_tests.cpp`
  - `apps/iggy3d_creative/StandalonePicking.hpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `src/render/vulkan/RenderLoop.cpp`
  - `src/render/FrameInput.hpp`
  - `src/render/FrameInput.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `tests/unit/standalone_picking_tests.cpp`
  - `tests/unit/render_camera_frame_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`

- Mat4 call-site table:

```text
Surface: Core Mat4 point transform
File/function: src/core/math/Mat4.cpp:32, src/core/math/Mat4.hpp:16
Current behavior: computes clip x/y/z/w from a row-major 4x4 matrix. If w is finite and not 0 and not 1, returns x/w, y/w, z/w. If w is exactly 1, exactly 0, or non-finite, returns raw x/y/z without division. It does not expose w.
Needs raw w: no caller can get it from this API.
Failure policy: no validity flag; non-finite x/y/z and unusual w values are left for callers to inspect after the fact.
Duplicate math: local clip-w helpers recompute the same fourth-row dot product where w matters.
Suggested owner/API: core/math should own one projection helper that returns both NDC and raw w while preserving transformPoint(Mat4) as a wrapper.
Recommended next slice: add a result type and guard tests for w=finite nonzero, w=1, w=0, and non-finite w before routing callers.

Surface: Standalone box projection for object pick bounds
File/function: apps/iggy3d_creative/StandalonePicking.cpp:176
Current behavior: for each AABB corner, computes clipW(...), skips the corner if w is non-finite or <= 0, then calls transformPoint(...) to compute NDC and converts finite x/y to pixels. The box is valid if at least one corner survives.
Needs raw w: yes, for behind-camera/camera-plane filtering.
Failure policy: w <= 0 and non-finite w are invalid/skipped. Non-finite NDC x/y also skipped.
Duplicate math: yes. clipW computes row 3 once; transformPoint recomputes x/y/z/w and divides.
Suggested owner/API: consume ProjectedPoint-style core helper and keep the same w > 0 corner filter.
Recommended next slice: route after guard tests pin the current partial-corner-valid behavior.

Surface: Standalone single point projection
File/function: apps/iggy3d_creative/StandalonePicking.cpp:217
Current behavior: computes clipW(...), rejects non-finite w or w <= 0, then calls transformPoint(...) and converts finite NDC x/y to pixels.
Needs raw w: yes.
Failure policy: no screen point for w <= 0, w non-finite, or non-finite NDC x/y.
Duplicate math: yes.
Suggested owner/API: consume ProjectedPoint-style core helper; keep w > 0 policy local to screen projection.
Recommended next slice: route together with projectBoxToScreen(...) and main label projection.

Surface: Standalone path handle depth
File/function: apps/iggy3d_creative/StandalonePicking.cpp:381
Current behavior: handle screen AABB uses projectBoxToScreen(...). centerDepth stores clipW(...) of the handle visual-bounds center and is used to choose the closest clicked handle.
Needs raw w: yes, specifically for depth ordering, not NDC.
Failure policy: pickPathPointHandle skips invalid screen AABBs, then compares centerDepth. There is no separate non-finite centerDepth guard, so invalid AABB filtering is the effective gate.
Duplicate math: raw w is separate by design today.
Suggested owner/API: core projection result can supply w for depth without redoing fourth-row math.
Recommended next slice: preserve existing handle-depth ordering and add a focused test if this path is routed.

Surface: Standalone dimension-label projection
File/function: apps/iggy3d_creative/main.cpp:1541
Current behavior: computes clipW(...) for the selected bounds center, requires finite w > 0, then calls transformPoint(...) and maps NDC to pixels. Unlike StandalonePicking helpers, it does not explicitly re-check NDC finiteness at this site.
Needs raw w: yes, to avoid labeling behind-camera selections.
Failure policy: finite positive w gate; no local NDC finite gate.
Duplicate math: yes.
Suggested owner/API: use the same core projection result through a small standalone helper or direct call, with finite positive w and NDC finite checks if behavior is intentionally tightened by test.
Recommended next slice: route after adding a label projection guard or keep NDC finite policy unchanged.

Surface: Vulkan debug/projectile overlay projection
File/function: src/render/vulkan/RenderLoop.cpp:193
Current behavior: calls transformPoint(frame.camera.clipFromWorld, world) directly. Rejects if NDC is non-finite or outside loose ranges: z outside [-0.05, 1.05], x/y outside [-1.20, 1.20]. It never checks raw w.
Needs raw w: likely yes for explicit behind-camera/camera-plane rejection, but current behavior relies only on NDC range.
Failure policy: no w <= 0 check. w == 0 or non-finite w can return raw x/y/z from transformPoint(...) and then may be rejected only by finite/range checks.
Duplicate math: no local clipW duplicate here; the issue is missing raw w visibility.
Suggested owner/API: use the same core projection result only after tests pin current behind-camera and invalid-w overlay behavior.
Recommended next slice: add guard tests first. Do not silently add w > 0 rejection in render without deciding whether that is a behavior fix.

Surface: Mat4 tests
File/function: tests/unit/math_tests.cpp:95
Current behavior: only identity matrix transform is tested, so only w == 1 is covered.
Needs raw w: not currently.
Failure policy: no tests for w == 0, finite w != 1, negative w, or non-finite w.
Duplicate math: none.
Suggested owner/API: core/math tests should pin current transformPoint(Mat4) compatibility and the new projection result.
Recommended next slice: add the missing w-policy tests before changing callers.
```

- Local clip-W duplicate findings:
  - `apps/iggy3d_creative/StandalonePicking.cpp:169` defines the only local
    `clipW(...)` helper found by the scan.
  - `projectBoxToScreen(...)` computes `clipW(...)` per corner and then calls
    `transformPoint(...)`, which recomputes the same fourth-row dot product.
  - `projectPointToScreen(...)` does the same for individual points.
  - `apps/iggy3d_creative/main.cpp:1545` imports and uses standalone `clipW`
    for selection dimension labels, then calls `transformPoint(...)`.
  - `buildPathPointHandleHits(...)` uses `clipW(...)` for center-depth ordering;
    this is not exactly duplicate NDC projection, but it would naturally consume
    the raw `w` field from a core projection result.
  - `src/render/vulkan/RenderLoop.cpp:193` has no duplicate clip-w math. Its
    problem is the opposite: it cannot inspect raw `w`.

- Failure-policy findings:
  - Core `transformPoint(Mat4, Vec3)`:
    - finite `w` where `w != 0` and `w != 1`: returns perspective-divided NDC.
    - `w == 1`: returns raw x/y/z, which is mathematically the same as divide by 1.
    - `w == 0`: returns raw x/y/z instead of flagging invalid.
    - non-finite `w`: returns raw x/y/z instead of flagging invalid.
    - negative finite `w`: divides by the negative value; there is no built-in
      behind-camera policy.
  - Standalone picking/screen projection:
    - `w <= 0` and non-finite `w` are invalid/skipped before NDC is used.
    - non-finite NDC x/y are invalid/skipped.
    - `projectBoxToScreen(...)` can still produce a valid screen AABB if at
      least one corner has finite positive `w`.
  - Standalone dimension label:
    - finite positive `w` is required, but the site does not explicitly check
      NDC finiteness before converting to pixels.
  - Vulkan debug/projectile overlay:
    - no raw `w` policy exists today.
    - finite NDC and loose NDC range checks are the only gate.
    - before changing it to reject `w <= 0`, add tests that pin current
      behind-camera/camera-plane behavior and decide if alignment with
      standalone is intended.
  - Frame input validation rejects non-finite camera matrices but does not
    validate per-point clip `w`, which is correct; this belongs at projection
    consumption sites.

- Recommended next implementation slice:
  1. Add a core result type in `Mat4.hpp`, for example:
     `ProjectedPoint3 { Vec3 ndc; float w; bool finite; }`.
     `finite` should report finite NDC x/y/z and finite `w`; do not bake
     `w > 0` into the core helper because that is consumer policy.
  2. Add `projectPoint(const Mat4&, Vec3)` or
     `transformPointHomogeneous(...)` that computes x/y/z/w once and sets
     `ndc` using the exact current `transformPoint(Mat4)` divide rules.
  3. Keep `transformPoint(const Mat4&, Vec3)` as a compatibility wrapper that
     returns `projectPoint(...).ndc`.
  4. Add core math tests for:
     - finite `w != 0 && w != 1` divides;
     - `w == 1` returns raw/same values;
     - `w == 0` preserves current raw x/y/z compatibility;
     - non-finite `w` preserves current raw x/y/z compatibility while the new
       result marks `finite=false`;
     - negative finite `w` divides and leaves consumer policy to callers.
  5. Route standalone `clipW(...)`, `projectPointToScreen(...)`,
     `projectBoxToScreen(...)`, selection-label projection, and path-handle
     center-depth through the new result while preserving current `w > 0`
     policies.
  6. Add render overlay guard tests before routing
     `RenderLoop::projectWorldToScreen(...)`; either preserve its current
     NDC-only gate initially or explicitly choose a follow-up behavior fix to
     reject `w <= 0`.

- Risks/blockers:
  - A core helper that names `valid` as `w > 0` would be too opinionated; w
    front/behind policy differs from generic homogeneous transform validity.
  - Routing standalone is low risk if tests pin the current `w > 0` filters.
    Routing Vulkan overlay is higher risk because it lacks raw-w checks today.
  - Current `math_tests` do not cover the surprising `w == 0` and non-finite
    fallback behavior, so compatibility tests should land before changing the
    implementation shape.

- Verification commands run:
  - `git -C /Users/kogaryu/iggy3d status --short`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "transformPoint\\(|clipW|clipFromWorld|projectPointToScreen|projectWorldToScreen" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit`
  - `rg -n "\\.w\\b|perspective|homogeneous|ndc|clip" /Users/kogaryu/iggy3d/src/core /Users/kogaryu/iggy3d/src/render /Users/kogaryu/iggy3d/apps/iggy3d_creative /Users/kogaryu/iggy3d/tests/unit`
