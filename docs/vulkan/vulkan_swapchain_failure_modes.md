# Vulkan Swapchain Failure Modes

This document defines how `iggy3d` handles swapchain acquire, present, resize, recreate, minimized, zero-extent, device-lost, and surface-lost states.

Swapchain failure handling is renderer-owned presentation recovery. It may skip, recreate, or fail a render frame, but it must not mutate runtime truth, save data, replay results, command legality, camera mode truth, or projection semantics.

## Purpose

Define deterministic WSI failure behavior:

```text
acquire success -> record/submit/present
acquire suboptimal -> record/submit/present, mark recreate soon
acquire out-of-date -> skip current frame, mark swapchain dirty
present success -> presented
present suboptimal -> accepted present, mark recreate soon
present out-of-date -> frame was submitted, mark swapchain dirty for next frame
zero drawable extent -> skip before acquire
surface lost -> invalidate surface/swapchain path
device lost -> enter device-lost shutdown/recovery path
```

This document narrows [swapchain_contract.md](swapchain_contract.md), [render_loop.md](render_loop.md), [sync_contract.md](sync_contract.md), [lifetime.md](lifetime.md), [vulkan_surface_wsi_platforms.md](vulkan_surface_wsi_platforms.md), and [diagnostics_and_tests.md](diagnostics_and_tests.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for `vkAcquireNextImageKHR`, `vkQueuePresentKHR`, WSI result codes, semaphore/fence validity, and surface loss behavior.
- Khronos Vulkan Tutorial swapchain recreation chapter for first implementation shape, fence reset deadlock avoidance, explicit resize flag, and minimization handling.
- Khronos Vulkan Samples swapchain recreation sample for old-swapchain/present-semaphore lifetime refinement after first proof.
- Vulkan Guide WSI/synchronization topics for explanation.
- Platform shell docs for drawable/minimize/resize events.

## Scope

In scope:

- acquire result matrix;
- present result matrix;
- swapchain dirty state;
- recreate trigger policy;
- zero extent and minimized behavior;
- suboptimal handling;
- out-of-date handling;
- surface lost handling;
- device lost handling;
- fence/semaphore safety during early exits;
- old swapchain lifetime policy;
- diagnostics, reason codes, and smoke tests.

Out of scope:

- initial swapchain format/present-mode selection;
- native surface creation;
- platform event polling details;
- runtime tick pause policy while minimized;
- full device-lost renderer restart implementation;
- `VK_EXT_swapchain_maintenance1` adoption;
- multi-window swapchains.

## Local File Surface

Likely future files:

```text
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
tests/unit/render_swapchain_failure_policy_tests.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_minimize_smoke.cpp
tests/smoke/vulkan_device_lost_smoke.cpp
```

`Swapchain` owns swapchain state and recreate decisions. `FrameSync` owns fence/semaphore correctness around acquire/submit/present. `VulkanBackend` coordinates frame result policy.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| swapchain dirty flag | `Swapchain` or backend coordinator | set by resize/acquire/present |
| drawable extent | platform shell/app frame input | backend consumes read-only |
| acquire result | swapchain/backend | controls submit/present eligibility |
| present result | swapchain/backend | controls next-frame recreate/failure |
| frame fence state | `FrameSync` | must not deadlock on skipped submit |
| old swapchain | `Swapchain` | deferred or safe destruction |
| device-lost state | backend/lifetime policy | renderer failure, not runtime mutation |
| surface-lost state | backend/surface owner | may require surface recreation path |

Rules:

- runtime/projection never see WSI result codes;
- renderer result may include backend-neutral reason codes;
- renderer skip/failure never changes replay hash;
- platform resize/minimize state is input, not Vulkan-owned truth.

## State Model

Recommended swapchain states:

```text
uninitialized
ready
dirty_resize
dirty_out_of_date
dirty_suboptimal
not_drawable
recreating
surface_lost
device_lost
failed
```

Rules:

- `ready` is the only state where acquire/submit/present normal path may run;
- `not_drawable` skips before acquire;
- dirty states attempt recreate at a safe point when drawable extent is nonzero;
- `surface_lost` requires surface path recovery or backend failure;
- `device_lost` requires device-lost policy, not swapchain-only recreate.

## Frame Order Safety

Required first frame order around failure handling:

1. Wait for current frame fence.
2. Check drawable extent.
3. If not drawable, skip without acquire and without fence reset.
4. If swapchain dirty and drawable, recreate at safe point.
5. Acquire image.
6. If acquire skips/fails before submit, do not reset fence.
7. Record command buffer only after successful/accepted acquire.
8. Reset fence immediately before submit.
9. Submit work that will signal the fence.
10. Present submitted image.
11. Handle present result.

Hard rule: never reset a frame fence unless a submit that will signal it is about to happen.

## Acquire Result Matrix

Acquire result policy:

| Acquire result | Submit | Present | Fence reset | Frame status | Swapchain action |
| --- | --- | --- | --- | --- | --- |
| `VK_SUCCESS` | yes | yes | yes, before submit | continue | none |
| `VK_SUBOPTIMAL_KHR` | yes | yes | yes, before submit | presented/suboptimal | mark recreate soon |
| `VK_ERROR_OUT_OF_DATE_KHR` | no | no | no | skipped | mark dirty out-of-date |
| `VK_NOT_READY` | no | no | no | skipped or failed by strictness | no normal recreate |
| `VK_TIMEOUT` | no | no | no | skipped or failed by strictness | no normal recreate |
| `VK_ERROR_SURFACE_LOST_KHR` | no | no | no | failed/recoverable surface | mark surface lost |
| `VK_ERROR_DEVICE_LOST` | no | no | no | failed device lost | mark device lost |
| other error | no | no | no | failed | mark failed |

Rules:

- first implementation should avoid timeout-style acquire unless explicitly configured;
- `VK_SUBOPTIMAL_KHR` after acquire is an accepted acquire, but diagnostics must mark it;
- `VK_ERROR_OUT_OF_DATE_KHR` after acquire means no image can be used for this frame;
- acquire failure before submit must leave current frame fence signaled.

## Present Result Matrix

Present result policy:

| Present result | Submitted work happened | Frame status | Swapchain action |
| --- | --- | --- | --- |
| `VK_SUCCESS` | yes | presented | none |
| `VK_SUBOPTIMAL_KHR` | yes | presented_suboptimal | mark recreate soon |
| `VK_ERROR_OUT_OF_DATE_KHR` | yes | submitted_not_presented_or_outdated | mark dirty out-of-date |
| `VK_ERROR_SURFACE_LOST_KHR` | yes or unknown | failed/recoverable surface | mark surface lost |
| `VK_ERROR_DEVICE_LOST` | yes or unknown | failed device lost | mark device lost |
| `VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT` | yes | failed or recreate, if feature adopted | out of scope first path |
| other error | yes or unknown | failed | mark failed |

Rules:

- present happens only after a successful submit;
- present result handling must not reset frame sync objects incorrectly;
- `VK_SUBOPTIMAL_KHR` from present is accepted for that frame but should schedule recreate;
- `VK_ERROR_OUT_OF_DATE_KHR` from present schedules recreate for a later safe point;
- device/surface loss are not normal swapchain recreates.

## Suboptimal Policy

First policy:

```text
suboptimal_after_acquire=draw_and_present_then_recreate_soon
suboptimal_after_present=accept_present_then_recreate_soon
```

Rules:

- suboptimal is not a crash;
- strict smoke should report suboptimal but not fail if a present occurred successfully;
- repeated suboptimal should not spam logs every frame;
- recreate should be coalesced with resize/out-of-date state;
- if suboptimal persists after recreate, diagnostics should include platform, format, present mode, extent, and surface capabilities.

## Out-Of-Date Policy

Acquire out-of-date:

```text
skip_current_frame=true
submit=false
present=false
fence_reset=false
swapchain_dirty=true
```

Present out-of-date:

```text
submitted=true
present_result=out_of_date
swapchain_dirty=true
next_frame_recreate=true
```

Rules:

- out-of-date is expected during resize/display changes;
- out-of-date is not runtime failure;
- optional smoke may treat it as diagnosed skip/recreate;
- strict resize smoke passes only if recovery is validation-clean;
- repeated out-of-date with nonzero drawable extent should fail after a bounded retry count.

## Resize Event Policy

Resize may be detected by:

```text
platform_resize_event
drawable_extent_changed
VK_ERROR_OUT_OF_DATE_KHR from acquire
VK_ERROR_OUT_OF_DATE_KHR from present
VK_SUBOPTIMAL_KHR from acquire/present
surface_capabilities_changed
```

Rules:

- platform resize event marks swapchain dirty;
- renderer should coalesce resize events rather than recreate for every raw event;
- recreate happens only when drawable extent is nonzero;
- runtime/projection data should be rebuilt by app frame assembly on later frames, not by swapchain code;
- if selected color/depth format changes, dependent dynamic-rendering pipeline compatibility must be rechecked.

## Minimized And Zero Extent

Zero extent policy:

```text
if drawable_width == 0 or drawable_height == 0:
  drawable=false
  acquire=false
  submit=false
  present=false
  recreate=false unless a nonzero extent appears
  frame_status=skipped_not_drawable
```

Rules:

- do not call acquire on a known zero drawable extent;
- do not destroy/recreate swapchain repeatedly while minimized;
- app policy decides whether runtime ticks continue while minimized;
- renderer diagnostics must print `runtime_state_touched=false`;
- strict minimize smoke passes if renderer skips cleanly and resumes after nonzero extent.

## Recreate Preconditions

Before recreate:

- drawable extent is nonzero;
- device is not lost;
- surface is not lost;
- logical device and queues are valid;
- swapchain support is re-queried;
- current frame work is safe according to sync/lifetime policy;
- old swapchain handoff/destruction policy is selected.

If any precondition fails, recreate must not proceed.

## Recreate Sequence

First implementation safe sequence:

1. Stop normal frame submission for this frame.
2. Wait for relevant in-flight work or use a proven old-swapchain deferral path.
3. Re-query surface capabilities/formats/present modes.
4. Select extent/format/present mode/image count.
5. Create new swapchain with `oldSwapchain` when appropriate.
6. Create new image views.
7. Recreate depth resources for new extent.
8. Recreate or validate pipelines if color/depth format changed.
9. Destroy old swapchain resources only after safe.
10. Clear dirty flags.
11. Print recreate receipt.

Rules:

- first implementation may use `vkDeviceWaitIdle` during recreate for simplicity if documented;
- later implementations may defer old swapchain destruction for smoother resize;
- recreate must not allocate or mutate runtime state;
- recreate failure should leave backend in `failed`, `not_drawable`, `surface_lost`, or `device_lost`, not half-ready.

## Old Swapchain Policy

First policy:

```text
old_swapchain_mode=safe_wait_idle
oldSwapchain_field=allowed
deferred_present_semaphore_cleanup=deferred_later
```

Rules:

- `vkDeviceWaitIdle` during recreate is acceptable for first visual proof;
- using `oldSwapchain` is allowed but does not remove the need for lifetime correctness;
- old swapchain image views must be destroyed before old swapchain destruction;
- present semaphores tied to old swapchain must not be destroyed while presentation may still wait on them;
- refined deferred cleanup should be a later performance/lifetime packet.

## Surface Lost Policy

Surface lost means normal swapchain recreate is insufficient.

Policy:

```text
surface_lost=true
destroy_swapchain_when_safe=true
destroy_surface_when_safe=true
request_surface_recreate_or_fail_backend=true
runtime_state_touched=false
```

Rules:

- surface loss is a platform/render failure or recovery event, not gameplay failure;
- renderer must not continue presenting to a lost surface;
- platform shell may need to create a new surface for the existing/new instance depending on final app design;
- first implementation may fail the Vulkan backend on surface loss if recovery is not implemented;
- diagnostics must distinguish surface lost from out-of-date resize.

## Device Lost Policy

Device lost is not a swapchain-only failure.

Policy:

```text
device_lost=true
stop_submitting=true
stop_presenting=true
mark_all_device_resources_invalid=true
runtime_state_touched=false
```

Rules:

- no swapchain recreate attempt after device lost unless device recovery is explicitly implemented;
- smoke should fail with a device-lost reason;
- app may continue headless/runtime state only if visual backend is disabled or restarted by an explicit higher-level policy;
- save/replay truth is unaffected by device loss.

Full recovery belongs in a later `vulkan_device_loss_recovery.md` pass.

## Retry And Loop Policy

To avoid infinite failure loops:

```text
max_recreate_attempts_per_frame=1
max_consecutive_recreate_failures=small_explicit_count
max_consecutive_out_of_date_with_nonzero_extent=small_explicit_count
```

Rules:

- do not spin recreating in a tight loop while extent is zero;
- do not recursively call render-frame from recreate;
- after bounded repeated failure, return `Failed` with stable reason;
- platform event polling remains app-owned.

## Diagnostics Receipt

Swapchain failure-mode smoke should print:

```text
swapchain_failure_mode_policy=enabled
frame_index=
frame_slot=
drawable=true|false
drawable_extent=
window_minimized=true|false
swapchain_state=ready|dirty_resize|dirty_out_of_date|dirty_suboptimal|not_drawable|recreating|surface_lost|device_lost|failed
acquire_result=
acquire_action=continue|skip|fail|mark_dirty|surface_lost|device_lost
submit_attempted=true|false
fence_reset=true|false
present_attempted=true|false
present_result=
present_action=accepted|mark_dirty|surface_lost|device_lost|fail
swapchain_recreate_attempted=true|false
swapchain_recreate_result=success|skipped_not_drawable|failed|not_attempted
old_swapchain_mode=safe_wait_idle|deferred|none
suboptimal_count=
out_of_date_count=
surface_lost_count=
device_lost_count=
runtime_state_touched=false
reason=
```

Rules:

- acquire and present results must be separate fields;
- skip/fail/recreate actions must be explicit;
- `fence_reset=false` is mandatory for pre-submit skips;
- all platforms use the same field names.

## Failure Reason Codes

Use stable reason codes:

```text
swapchain_failure_scope_blocked
swapchain_not_drawable
swapchain_zero_extent
swapchain_minimized
swapchain_acquire_suboptimal
swapchain_acquire_out_of_date
swapchain_acquire_timeout
swapchain_acquire_not_ready
swapchain_acquire_failed
swapchain_present_suboptimal
swapchain_present_out_of_date
swapchain_present_failed
swapchain_surface_lost
swapchain_device_lost
swapchain_recreate_requested
swapchain_recreate_skipped_not_drawable
swapchain_recreate_failed
swapchain_recreate_loop_detected
swapchain_old_swapchain_lifetime_unsafe
swapchain_fence_reset_without_submit
swapchain_present_without_submit
swapchain_runtime_leak
```

Rules:

- reason codes should appear in diagnostics and smoke output;
- reason codes must not vary by platform;
- native `VkResult` names may be printed beside stable renderer reason codes.

## Validation Expectations

Validation should catch:

- acquiring with an invalid/retired swapchain;
- using signaled or pending semaphores incorrectly;
- resetting/reusing command buffers before frame fence is safe;
- destroying swapchain resources while still in use;
- creating swapchain with invalid surface/device relationship;
- presenting invalid image indices or swapchains.

Tests should catch:

- acquire out-of-date does not reset fence;
- zero extent skips before acquire;
- present out-of-date schedules next-frame recreate;
- suboptimal is accepted but diagnosed;
- surface lost differs from normal resize;
- device lost does not attempt swapchain-only recreate;
- runtime state remains untouched.

## Platform Notes

macOS/MoltenVK:

- surface capability changes may reflect window/display/Metal-layer behavior;
- portability constraints are diagnostics, not runtime architecture;
- minimized/zero drawable behavior must skip cleanly.

Linux:

- X11 and Wayland may report resize/minimize/presentation changes differently through SDL3/GLFW;
- native Vulkan proof is required for shipping;
- software Vulkan can be optional smoke only when labeled.

Windows:

- interactive resize may produce many events; coalesce recreates;
- Win32 surface loss is platform/render failure, not gameplay failure;
- strict smoke should prove resize recovery with validation enabled.

## Tests

Expected future tests:

```text
tests/unit/render_swapchain_failure_policy_tests.cpp
tests/unit/render_frame_sync_failure_policy_tests.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_minimize_smoke.cpp
tests/smoke/vulkan_swapchain_recreate_smoke.cpp
tests/smoke/vulkan_surface_lost_smoke.cpp
tests/smoke/vulkan_device_lost_smoke.cpp
```

Unit tests should cover:

- acquire result matrix;
- present result matrix;
- fence reset eligibility;
- zero-extent skip policy;
- recreate retry limits;
- failure reason stability.

Smoke tests should cover:

- resize marks dirty and recreates;
- minimized/zero extent skips without acquire;
- restored nonzero extent resumes rendering;
- suboptimal result receipt when injectable;
- out-of-date result receipt when injectable;
- validation-clean recreate.

## Acceptance Criteria

Swapchain failure handling is acceptable only when:

- zero drawable extent skips before acquire;
- acquire out-of-date skips without fence reset, submit, or present;
- acquire suboptimal is accepted and diagnosed;
- present suboptimal is accepted and schedules recreate;
- present out-of-date schedules recreate after submitted work;
- fence reset happens only when submit will signal it;
- device lost enters device-lost policy, not swapchain-only recreate;
- surface lost is diagnosed separately from resize/out-of-date;
- recreate is bounded and does not spin on minimized windows;
- diagnostics include separate acquire and present result fields;
- runtime state hash/replay result is unaffected by every skip/failure path.

## Open Detail Items

The next detailed pass should define:

- exact `RenderFrameStatus` values for each matrix row;
- exact retry counts for repeated out-of-date/recreate failures;
- exact injectable test hook for acquire/present result simulation;
- exact old-swapchain deferred destruction policy after first proof;
- exact behavior when format changes during recreate;
- exact surface recreation path after `VK_ERROR_SURFACE_LOST_KHR`;
- exact device-loss recovery boundary in `vulkan_device_loss_recovery.md`.
