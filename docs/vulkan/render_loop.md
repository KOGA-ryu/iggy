# Vulkan Render Loop

This document defines the per-frame execution contract for the Vulkan backend.

The render loop is the handoff line between the app/runtime/projection side and Vulkan. Runtime produces truth. Projection produces backend-neutral frame data. Vulkan consumes one `FrameInput` and attempts to present it. Vulkan may skip or fail a frame, but it may not advance gameplay, change save truth, change replay results, or mutate runtime state.

## Purpose

Define the exact frame order:

```text
platform events
runtime tick and command admission
projection build
FrameInput assembly
renderer frame validation
swapchain acquire
command recording
queue submit
present
diagnostics receipt
```

This document does not define command buffer internals, descriptor layout, material policy, or GPU resource allocation. Those belong in separate Vulkan docs and file plans.

## Required Inputs

Each visual frame consumes:

- current platform/window status from the app shell;
- current drawable size;
- optional resize/minimize/focus events;
- runtime state that has already accepted legal commands;
- `SceneProjection` result;
- `DebugProjection` result when enabled;
- derived `CameraState` presentation data;
- assembled `FrameInput`;
- renderer configuration such as validation, sync validation, strict smoke, and diagnostics output path.

The renderer must not pull state directly from runtime/content/projection internals. It receives the frame through the public renderer API.

## Owners

| Owner | Owns | Does not own |
| --- | --- | --- |
| Platform shell | window, app event pump, drawable size, surface provider | Vulkan device, swapchain policy, runtime truth |
| Runtime | gameplay truth, command legality, camera mode truth, deterministic tick | GPU resources, presentation timing |
| Projection | backend-neutral scene/debug outputs | Vulkan handles, swapchain, command buffers |
| App frame assembly | builds `FrameInput` from runtime/projection/platform data | raw Vulkan objects |
| Vulkan backend | acquire, record, submit, present, GPU diagnostics | runtime mutation, save/replay truth |

## Frame Ownership Boundary

The renderer frame call should be shaped like this in future file plans:

```cpp
RenderFrameResult RenderBackend::renderFrame(const FrameInput& input);
```

Rules:

- `input` is read-only;
- renderer may keep backend-private GPU state between frames;
- renderer must not retain raw projection pointers after `renderFrame` returns;
- renderer must return a result/diagnostic instead of mutating runtime on failure;
- app/runtime decides whether another runtime tick happens after a render failure.

## High-Level App Loop

Recommended visual app loop:

1. Poll platform events.
2. Translate input events into app/controller actions.
3. Submit runtime commands through public runtime command APIs.
4. Let runtime command admission accept/reject commands.
5. Advance runtime tick according to app policy.
6. Build `SceneProjection` from stable runtime state.
7. Build `DebugProjection` if enabled.
8. Derive renderer camera frame from runtime camera truth.
9. Query current drawable size from platform shell.
10. Assemble `FrameInput`.
11. Call `RenderBackend::renderFrame(input)`.
12. Record renderer diagnostics.
13. Continue, skip, or exit according to app strictness and result code.

Runtime may be headless and fully functional without steps 6 through 13. Vulkan integration must not become a prerequisite for runtime acceptance.

## Renderer Frame Order

Within `RenderBackend::renderFrame`, the first Vulkan implementation should use this order:

1. Increment or observe presentation frame index.
2. Validate `FrameInput`.
3. If viewport is not drawable, return `skipped_not_drawable`.
4. If swapchain is dirty, recreate swapchain at a safe point.
5. Wait for the current frame-in-flight fence.
6. Acquire next swapchain image.
7. Handle acquire result.
8. Reset current frame command buffer.
9. Begin command buffer.
10. Begin dynamic rendering or render-pass fallback.
11. Bind first-room pipeline.
12. Bind vertex/index resources.
13. Push or bind frame camera/projection data.
14. Record first-room draw calls from projection data.
15. Record debug projection draw calls if enabled and available.
16. End rendering.
17. End command buffer.
18. Submit command buffer to graphics queue.
19. Present image to present queue.
20. Handle present result.
21. Emit frame diagnostics.
22. Return `presented`, `skipped`, or `failed`.

This order assumes binary WSI synchronization for the first implementation.

## Frame Result Shape

Future file plans should define a small renderer result type.

Proposed shape:

```cpp
enum class RenderFrameStatus {
  Presented,
  SkippedNotDrawable,
  SkippedNoProjection,
  SwapchainRecreated,
  Failed
};

struct RenderFrameResult {
  RenderFrameStatus status;
  RenderReasonCode reason;
  bool runtimeStateTouched;
  bool presented;
};
```

Rules:

- `runtimeStateTouched` must always be `false` for Vulkan backend results;
- `Presented` means a present call succeeded or returned an accepted suboptimal result;
- `SwapchainRecreated` may mean the frame did not present but renderer state recovered;
- `Failed` in strict smoke exits nonzero from the app/test harness.

## Drawable And Minimized Behavior

If `FrameInput.viewport.width == 0` or `FrameInput.viewport.height == 0`:

- renderer must not acquire a swapchain image;
- renderer must not submit a draw frame;
- renderer returns `SkippedNotDrawable`;
- diagnostics print `drawable=false`;
- runtime state is unchanged by the skip.

Minimized behavior is app policy, not renderer authority.

Allowed app policies:

- continue runtime ticking while rendering is skipped;
- pause runtime ticking while minimized;
- run fixed headless ticks for deterministic smoke.

Whichever policy is chosen must be explicit in visual app diagnostics. Vulkan must not silently decide it.

## Resize And Swapchain Dirty Flow

Resize can be discovered from:

- platform drawable-size event;
- acquire result;
- present result;
- explicit app notification.

Required behavior:

1. Renderer records `swapchain_dirty=true`.
2. Renderer waits for affected in-flight work at a safe point.
3. Renderer recreates swapchain-dependent resources using [lifetime.md](lifetime.md).
4. Renderer updates swapchain diagnostics.
5. Renderer draws the next drawable frame or returns a diagnosed skip/failure.

The renderer must not rebuild runtime/projection data because of resize. The app assembles a later `FrameInput` with the new viewport.

## Acquire Handling

Acquire result policy:

| Result | Required behavior |
| --- | --- |
| success | record and submit frame |
| suboptimal | record frame, mark recreate soon, or recreate before next frame |
| out of date | mark swapchain dirty, skip current frame, recreate |
| timeout/not ready | diagnose according to strictness; first implementation should avoid timeout acquire |
| device lost | follow device-loss policy in [lifetime.md](lifetime.md) |
| other error | return `Failed` with reason code |

Acquire must use the current frame's image-available semaphore or a documented WSI synchronization object.

## Submit Handling

Submit rules:

- wait on image-available semaphore at the correct pipeline stage;
- signal render-finished semaphore for present;
- signal the current frame fence;
- never reset/reuse a command buffer before its fence is signaled;
- any submit failure is a renderer failure, not a runtime failure.

First implementation may use `vkQueueSubmit`. Timeline semaphore adoption is a later explicit decision gate.

## Present Handling

Present rules:

- present waits on render-finished semaphore;
- present queue and graphics queue ownership are explicit;
- out-of-date result marks swapchain dirty;
- suboptimal result is accepted but diagnosed;
- device loss follows device-loss policy;
- presentation failure does not mutate runtime.

If graphics and present queues differ, queue ownership or sharing mode must be defined in the swapchain/file plan before implementation.

## Frames In Flight

Default:

```text
frames_in_flight=2
sync_policy=binary_wsi
```

Per-frame state includes:

- frame fence;
- image-available semaphore;
- render-finished semaphore;
- command buffer slot;
- frame index;
- acquired image index when valid.

Rules:

- CPU may prepare a later frame only when that frame slot is available;
- GPU resource destruction must respect in-flight use;
- resize must wait for affected frame slots before destroying swapchain-dependent resources;
- diagnostics must report frame index and current in-flight slot.

## Projection Consumption

The renderer consumes projection data in this order for first-room work:

1. Validate frame camera and viewport.
2. Validate scene projection pointer or explicit no-scene mode.
3. Draw opaque room/floor/wall proxies.
4. Draw interactable/objective/pickup proxies.
5. Draw debug projection overlays if enabled.

Rules:

- renderer may reorder draw calls for backend-private efficiency only when diagnostics remain stable enough for testing;
- renderer may skip an unsupported projection item with a diagnostic fallback;
- renderer may not write corrected data back into projection or runtime;
- missing asset references produce fallback visuals/diagnostics, not runtime mutation.

## Camera And Matrix Consumption

For first-room rendering:

- `FrameInput.camera.clipFromWorld` is renderer-ready;
- renderer does not derive gameplay camera truth;
- renderer validates matrix finiteness before writing GPU data;
- viewport/scissor come from current swapchain extent or validated frame viewport;
- Vulkan clip-space/depth conventions follow [frame_input_contract.md](frame_input_contract.md).

Invalid camera data causes frame rejection or strict-smoke failure. It does not trigger renderer-side camera repair.

## Validation And Sync Validation

Strict Vulkan lanes require:

- validation enabled;
- sync validation enabled where available;
- zero validation errors during smoke;
- renderer receipt on failure;
- nonzero exit for strict smoke failure.

Frame-level validation blockers:

- acquire/present semaphore misuse;
- resetting in-use command buffers;
- destroying in-use resources during resize;
- using stale swapchain images;
- image layout errors;
- missing dynamic rendering attachments or render-pass compatibility;
- drawing with invalid pipeline/resource state.

## Diagnostics Receipt Fields

Frame diagnostics should include:

```text
render_frame_begin=true
frame_index=
frame_slot=
source_tick=
drawable=true|false
viewport=
swapchain_dirty=true|false
swapchain_recreated=true|false
acquire_result=
acquired_image_index=
command_recorded=true|false
submit_result=
present_result=
presented=true|false
draw_count=
debug_draw_count=
validation_error_count=
sync_validation_error_count=
runtime_state_touched=false
render_frame_status=
reason=
```

Use `unavailable` only when the field does not apply to that frame.

## Reason Codes

Recommended frame reason codes:

```text
ok
skipped_not_drawable
skipped_no_projection
frame_input_invalid
swapchain_dirty
swapchain_recreate_failed
acquire_out_of_date
acquire_failed
command_record_failed
submit_failed
present_out_of_date
present_failed
device_lost
validation_failed
sync_validation_failed
```

These codes should align with `diagnostics_and_tests.md` when implementation begins.

## Strict And Non-Strict Behavior

Strict smoke mode:

- invalid `FrameInput` fails;
- validation/syncval errors fail;
- acquire/submit/present unexpected errors fail;
- missing required first-room draw output fails;
- skip is allowed only for diagnosed not-drawable/minimized lanes.

Interactive visual demo:

- may continue after suboptimal swapchain;
- may skip while minimized;
- may show fallback visuals for unsupported projection items;
- must still print diagnostics on renderer failure.

Headless runtime:

- does not call Vulkan render loop;
- must remain green when Vulkan build options are off;
- remains source of truth for deterministic acceptance.

## Threading Model

First implementation should use one render thread: the app/main thread.

Rules:

- platform event polling, frame assembly, and `renderFrame` happen serially;
- runtime state is not mutated while projection is being built;
- projection data outlives `renderFrame`;
- renderer does not retain runtime/projection references after return;
- multi-threaded command recording is deferred.

Future threaded rendering needs a separate design document because it changes lifetime, ownership, and synchronization assumptions.

## Performance Expectations

First-room render loop cost target:

- one runtime/projection frame assembled per visual frame;
- two frames in flight;
- no per-frame pipeline creation;
- no per-frame shader module creation;
- no per-frame swapchain recreation except resize/out-of-date cases;
- no per-frame device-wide idle during normal drawing;
- no per-frame GPU allocation churn after bootstrap resources exist.

Allowed early simplifications:

- device-wide idle during shutdown;
- device-wide idle during initial simple resize implementation;
- simple one-time uploads for first-room geometry;
- CPU-side projection iteration each frame.

Any simplification that would be unacceptable long-term must be diagnosed or called out in the file plan.

## Tests

Future tests should include:

```text
tests/unit/render_frame_input_validation_tests.cpp
tests/unit/render_loop_result_tests.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_minimize_restore_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'render_loop|vulkan_empty_frame|vulkan_first_room|vulkan_resize|vulkan_sync'
```

Platform validation:

- macOS/MoltenVK: local first smoke lane;
- Linux: native Vulkan smoke lane with display or documented headless WSI strategy;
- Windows: native Vulkan smoke lane with installed loader/runtime checks.

## Acceptance Criteria

The render loop design is ready for file plans when:

- per-frame ownership is explicit;
- runtime tick and renderer frame are separated;
- `FrameInput` is the only scene/camera input to Vulkan;
- acquire/record/submit/present order is documented;
- resize/minimize behavior is documented;
- strict versus interactive behavior is documented;
- frame diagnostics fields are stable enough for smoke tests;
- validation/syncval failures are blockers;
- renderer failure cannot mutate runtime truth;
- Linux and Windows native Vulkan lanes are included beside macOS/MoltenVK.

## Open Detail Items

These belong in future file plans:

- exact `RenderFrameResult` C++ type;
- exact reason-code enum and text conversion;
- command buffer helper API;
- dynamic rendering begin/end helper shape;
- queue-family ownership policy when graphics and present queues differ;
- exact suboptimal-present policy;
- first resize smoke implementation;
- screenshot or pixel-readback proof for first-room smoke;
- CI strategy for Linux/Windows GPU lanes.
