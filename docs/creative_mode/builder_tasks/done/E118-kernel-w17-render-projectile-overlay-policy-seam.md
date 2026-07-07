# E118: Kernel W17 - Render Projectile Overlay Policy Seam

## Objective

Unblock E117 by extracting the render projectile overlay projection/layout policy
into a narrow testable helper, then pin the current behavior with focused unit
coverage.

This is still a behavior-preserving render policy seam. Do not route through
`iggy3d::projectPoint(...)` yet.

## Why This Exists

E117 could not honestly test render projection policy through receipts because
the relevant receipt fields are only appended after `RenderLoop::renderFrame(...)`
reaches its ready Vulkan render path. That path depends on concrete
`Swapchain`, `FrameSync`, and `CommandRecording` collaborators and is not
currently fakeable through the public API.

The useful logic is pure enough to test separately:

- `projectWorldToScreen(...)`
- `centeredOverlayRect(...)`
- `projectileOverlayLayoutFor(...)`

Those helpers are currently private in `src/render/vulkan/RenderLoop.cpp`.

## Required Work

1. Extract the projectile overlay projection/layout helpers into a small
   render-side module.
   - Suggested files:
     - `src/render/vulkan/ProjectileOverlayProjection.hpp`
     - `src/render/vulkan/ProjectileOverlayProjection.cpp`
   - Suggested public/testable API:
     - `ProjectileOverlayLayout`
     - `projectileOverlayLayoutFor(const FrameInput&)`
   - The layout should continue to expose:
     - `std::vector<OverlayRect> rects`
     - `projectileCount`
     - `markerCount`
     - `trailRectCount`
     - `projected`
     - `impactVisible`
   - Keep namespace and type ownership consistent with existing
     `iggy3d::vulkan` render code.
2. Keep `RenderLoop.cpp` behavior identical.
   - It should consume the extracted `projectileOverlayLayoutFor(frame)` result
     exactly as before.
   - Do not change receipt field names or counts.
3. Preserve the current projection policy exactly:
   - zero viewport width/height rejects projection;
   - projection uses `transformPoint(frame.camera.clipFromWorld, world)`;
   - non-finite NDC rejects;
   - NDC z outside `[-0.05, 1.05]` rejects;
   - NDC x/y outside `[-1.20, 1.20]` rejects;
   - accepted screen coordinates are clamped to viewport bounds;
   - raw clip `w` is not inspected in this slice.
4. Add focused tests.
   - Prefer a new target such as `render_projectile_overlay_projection_tests`,
     or the nearest focused existing render test if cleaner.
   - Pin at least:
     - visible finite projectile produces marker/trail overlay counts;
     - out-of-range NDC x/y rejects;
     - out-of-range NDC z rejects;
     - negative finite raw `w` follows the current behavior because raw `w` is
       not inspected;
     - zero viewport rejects projection/layout.
   - Tests should call the extracted helper directly, not instantiate
     `RenderLoop` or Vulkan collaborators.
5. Update CMake only as needed for the new `.cpp` and focused test target.

## Do Not

- Do not route render projection through `iggy3d::projectPoint(...)` in this
  slice.
- Do not change render/Vulkan backend setup, swapchain, command recording,
  frame submission, receipts, screenshot capture, Mat4 core behavior,
  standalone picking, Creative, RoomBake, save/load, mutation, or input.
- Do not expose a broad render utility dumping ground.
- Do not launch a window or run broad CTest.
- Do not stage, commit, or push.

## Suggested Verification

Use the narrowest target that covers the new helper. Likely:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d render_projectile_overlay_projection_tests product_vulkan_room_frame_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(render_projectile_overlay_projection_tests|product_vulkan_room_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Extraction shape:
- Policy pinned:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `CMakeLists.txt`
  - `cmake/iggy3d_tests.cmake`
  - `src/render/vulkan/RenderLoop.cpp`
  - `src/render/vulkan/ProjectileOverlayProjection.hpp`
  - `src/render/vulkan/ProjectileOverlayProjection.cpp`
  - `tests/unit/render_projectile_overlay_projection_tests.cpp`
- Extraction shape:
  - Added `iggy3d::vulkan::ProjectileOverlayLayout` and
    `projectileOverlayLayoutFor(const FrameInput&)` in a narrow render/Vulkan
    helper module.
  - Moved the pure private logic previously in `RenderLoop.cpp`:
    `projectWorldToScreen(...)`, `centeredOverlayRect(...)`, and
    `projectileOverlayLayoutFor(...)`.
  - `RenderLoop.cpp` still consumes `projectileOverlayLayoutFor(frame)` and
    emits the same `projectile_visual_*` receipt fields.
  - Added the new helper source to the Vulkan backend source list and registered
    `render_projectile_overlay_projection_tests`.
- Policy pinned:
  - Zero viewport width/height rejects projection.
  - Projection still uses `transformPoint(frame.camera.clipFromWorld, world)`;
    this slice does not route through `projectPoint(...)`.
  - Non-finite NDC rejects.
  - NDC x/y outside `[-1.20, 1.20]` rejects.
  - NDC z outside `[-0.05, 1.05]` rejects.
  - Negative finite raw clip `w` still projects when the resulting NDC is in
    range, because render-side policy does not inspect raw `w` yet.
  - Accepted screen coordinates still clamp through the centered overlay rect
    policy.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d render_projectile_overlay_projection_tests product_vulkan_room_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(render_projectile_overlay_projection_tests|product_vulkan_room_frame_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n '[[:blank:]]$' /Users/kogaryu/iggy3d/CMakeLists.txt /Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake /Users/kogaryu/iggy3d/src/render/vulkan/RenderLoop.cpp /Users/kogaryu/iggy3d/src/render/vulkan/ProjectileOverlayProjection.hpp /Users/kogaryu/iggy3d/src/render/vulkan/ProjectileOverlayProjection.cpp /Users/kogaryu/iggy3d/tests/unit/render_projectile_overlay_projection_tests.cpp || true`
- Concerns/deferred:
  - E118 intentionally preserved `transformPoint(Mat4, ...)` use. A later card
    can now route `ProjectileOverlayProjection.cpp` through
    `iggy3d::projectPoint(...)` against the guard tests added here.
