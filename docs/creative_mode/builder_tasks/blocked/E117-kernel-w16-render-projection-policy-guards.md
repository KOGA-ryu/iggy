# E117: Kernel W16 - Render Projection Policy Guards

## Objective

Add focused guard coverage for the render/Vulkan `projectWorldToScreen(...)`
policy before routing it through the E115 `iggy3d::projectPoint(...)` helper.

This is a test/audit-first card. Do not change render projection behavior in
this slice.

## Current Seam

`src/render/vulkan/RenderLoop.cpp` still has the remaining render-side Mat4
projection consumer:

- `projectWorldToScreen(const FrameInput&, Vec3, ScreenPoint&)`
- it currently calls `transformPoint(frame.camera.clipFromWorld, world)`
- it does not inspect raw clip `w`
- it rejects non-finite NDC, NDC z outside `[-0.05, 1.05]`, and NDC x/y outside
  `[-1.20, 1.20]`
- it clamps screen coordinates to viewport bounds

The helper is private to `RenderLoop.cpp`. The narrow public behavior seam is
the projectile overlay path:

- `projectileOverlayLayoutFor(...)` calls `projectWorldToScreen(...)` for
  projectile marker and trail samples.
- `RenderLoop::renderFrame(...)` exposes receipt fields:
  - `projectile_visual_projected`
  - `projectile_visual_count`
  - `projectile_marker_count`
  - `projectile_trail_rect_count`
  - `projectile_overlay_rect_count`
  - `projectile_impact_visible`
  - `projectile_rendered`
  - `projectile_record_mode`

## Required Work

1. Inspect existing render frame tests, especially:
   - `tests/unit/product_vulkan_room_frame_tests.cpp`
   - `tests/unit/render_camera_frame_tests.cpp`
2. Add focused tests that pin current render projection policy through public
   frame/receipt behavior.
   - Prefer exercising projectile overlay receipt fields, not exposing the
     private helper.
   - Pin at least:
     - visible finite projected projectile produces marker/trail overlay counts;
     - out-of-range NDC x/y or z does not project;
     - negative finite raw `w` follows current behavior, whatever the current
       receipt output proves, because render currently cannot inspect raw `w`;
     - zero/non-finite viewport remains rejected if not already covered.
3. If a direct helper extraction is absolutely necessary for testing, stop and
   report why; do not widen this into routing or API design without evidence.
4. Record the exact policy discovered in the completion brief so E118 can route
   through `projectPoint(...)` without changing behavior accidentally.

## Do Not

- Do not route `RenderLoop.cpp` through `iggy3d::projectPoint(...)` in this
  slice.
- Do not change `projectWorldToScreen(...)` behavior.
- Do not change render backend, Vulkan resources, swapchain, frame submission,
  Mat4, standalone picking, Creative, RoomBake, save/load, mutation, or input.
- Do not launch a window or run broad CTest.
- Do not stage, commit, or push.

## Suggested Verification

Use the narrowest target that covers the added tests. Likely:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target product_vulkan_room_frame_tests render_camera_frame_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_vulkan_room_frame_tests|render_camera_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Policy pinned:
- Receipt/test seam used:
- Tests/checks run:
- Concerns/deferred:

## Blocked

- Files inspected:
  - `src/render/vulkan/RenderLoop.cpp`
  - `src/render/vulkan/RenderLoop.hpp`
  - `src/render/vulkan/FrameSync.hpp`
  - `src/render/vulkan/Swapchain.hpp`
  - `src/render/vulkan/CommandRecording.hpp`
  - `src/render/FrameInput.hpp`
  - `src/projection/scene/SceneProjection.hpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/render_camera_frame_tests.cpp`
  - `tests/unit/render_null_renderer_tests.cpp`
  - `tests/unit/render_replay_invariance_tests.cpp`
  - `cmake/iggy3d_tests.cmake`
- Blocker:
  - The desired public receipt seam exists only inside `RenderLoop::renderFrame(...)`.
  - `projectileOverlayLayoutFor(...)` and `projectWorldToScreen(...)` are private to `RenderLoop.cpp`.
  - The `projectile_visual_*` receipt fields are appended only after `RenderLoop::renderFrame(...)` reaches its ready render path.
  - There is no existing no-window/fake-ready `RenderLoop` test harness. Existing product Vulkan frame tests build projection frames and `FrameInput`, but they do not call `RenderLoop::renderFrame(...)`.
  - Driving `RenderLoop::renderFrame(...)` through the receipt seam requires initialized `Swapchain`, `FrameSync`, `CommandRecording`, and Vulkan submit/present collaborators. These classes are concrete/non-virtual and not fakeable through the current public API.
  - Testing this card meaningfully would therefore require extracting a helper/layout seam or adding a fake render-loop harness. The task explicitly says to stop and report if direct helper extraction is necessary.
- Policy discovered but not pinned by new tests:
  - Current render-side projection calls `transformPoint(frame.camera.clipFromWorld, world)`.
  - It rejects non-finite NDC, NDC z outside `[-0.05, 1.05]`, and NDC x/y outside `[-1.20, 1.20]`.
  - It does not inspect raw clip `w`, so negative finite `w` follows whatever `transformPoint(Mat4, ...)` returns today.
  - It rejects zero viewport width/height before projection.
  - It clamps screen coordinates to viewport bounds after projection.
- Recommended next unblock:
  - Add a narrow render projection policy seam, for example a small render-side helper that returns projectile overlay layout/counts from `FrameInput` without Vulkan submit/present collaborators, while preserving current behavior.
  - Then add the requested policy guards against that helper.
  - Only after those guards exist should a later card route the helper through `iggy3d::projectPoint(...)`.
- Tests/checks run:
  - `rg`/`sed` inspection only.
  - No source or test files changed.
- Concerns/deferred:
  - I did not add a private `#include RenderLoop.cpp` style test or cast/hack `RenderLoop` internals because that would be brittle and outside the card's stop-and-report instruction.
