# E120: Kernel W18 - Render Projectile Overlay `projectPoint` Routing

## Objective

Route the extracted render projectile overlay projection helper through the E115
`iggy3d::projectPoint(...)` result while preserving the render-side projection
policy pinned by E118.

This completes the render/Vulkan consumer-routing step that E117/E118 prepared.

## Current Seam

E118 extracted the pure helper:

- `src/render/vulkan/ProjectileOverlayProjection.hpp`
- `src/render/vulkan/ProjectileOverlayProjection.cpp`
- `ProjectileOverlayLayout projectileOverlayLayoutFor(const FrameInput&)`

The private `projectWorldToScreen(...)` helper in
`ProjectileOverlayProjection.cpp` still calls:

```cpp
const Vec3 ndc = transformPoint(frame.camera.clipFromWorld, world);
```

E118 tests now pin the current behavior:

- zero viewport width/height rejects projection;
- non-finite NDC rejects;
- NDC z outside `[-0.05, 1.05]` rejects;
- NDC x/y outside `[-1.20, 1.20]` rejects;
- accepted screen coordinates clamp through centered overlay rects;
- negative finite raw clip `w` still projects when resulting NDC is in range,
  because render-side policy currently does **not** inspect raw `w`.

## Required Work

1. In `src/render/vulkan/ProjectileOverlayProjection.cpp`, route
   `projectWorldToScreen(...)` through `iggy3d::projectPoint(...)`.
   - Call `projectPoint(frame.camera.clipFromWorld, world)` once.
   - Use `.ndc` for the existing NDC policy checks.
   - Do **not** reject on `.w <= 0`.
   - Do **not** change `.finite` into a front/behind policy.
2. Preserve behavior exactly.
   - Existing E118 tests should remain green without expectation changes.
   - If any E118 test changes, stop and explain why; do not silently update it.
3. Remove any now-unneeded includes caused by the routing.
4. Do not touch `RenderLoop.cpp` receipt behavior except if a trivial include
   cleanup is necessary.
5. No standalone projection changes in this card; E116 already handled those.

## Do Not

- Do not add a `w > 0` render policy.
- Do not change projectile overlay counts, colors, marker/trail sizes, viewport
  clamp behavior, or receipt field names.
- Do not change `Mat4`, standalone picking, Creative, RoomBake, save/load,
  mutation, input, swapchain, command recording, or renderer backend behavior.
- Do not launch a window or run broad CTest.
- Do not stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d render_projectile_overlay_projection_tests product_vulkan_room_frame_tests math_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(render_projectile_overlay_projection_tests|product_vulkan_room_frame_tests|math_tests)$' --output-on-failure
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

## Completion Brief

- Files changed:
  - `src/render/vulkan/ProjectileOverlayProjection.cpp`
  - `docs/creative_mode/builder_tasks/done/E120-kernel-w18-render-projectile-overlay-projectpoint-routing.md`
- Routing shape:
  - `projectWorldToScreen(...)` now calls `projectPoint(frame.camera.clipFromWorld, world)` once.
  - The render helper continues to apply its existing policy to `ProjectedPoint3::ndc`.
  - Raw clip `w` is intentionally not inspected; `.finite` is not converted into a front/behind policy.
- Behavior preserved:
  - Existing zero-viewport, non-finite NDC, NDC z range, NDC x/y range, viewport clamp, and negative finite raw-`w` behavior are unchanged.
  - No projectile overlay count, color, marker size, trail size, receipt, swapchain, or command-recording behavior was changed.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d render_projectile_overlay_projection_tests product_vulkan_room_frame_tests math_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(render_projectile_overlay_projection_tests|product_vulkan_room_frame_tests|math_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files.
- Concerns/deferred:
  - Render policy still intentionally does not reject `w <= 0`; this card only routes through the core result helper.
  - Standalone/render-loop projection routing remains outside this card.
