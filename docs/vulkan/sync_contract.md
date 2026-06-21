# Vulkan Synchronization Contract

This document defines the first Vulkan synchronization policy for `iggy3d`.

Synchronization is renderer-owned. It protects GPU resources, swapchain images, command buffers, uploads, and presentation. It must never become runtime authority. A synchronization failure can fail a frame or renderer smoke test, but it cannot mutate gameplay truth, save data, replay results, or camera mode truth.

## Purpose

Define the exact first sync baseline:

```text
frames_in_flight=2
sync_policy=binary_wsi
image_available_semaphore per frame slot
render_finished_semaphore per frame slot
in_flight_fence per frame slot
explicit attachment/resource barriers
timeline semaphores deferred
```

This document sits below [render_loop.md](render_loop.md) and beside [command_recording.md](command_recording.md). The render loop decides when sync objects are used. Command recording names the resource/layout expectations. This contract defines the waits, signals, ownership safety, and validation gates.

## Source Priority

Use these sources before implementation:

- Vulkan Guide synchronization chapters for concepts and common patterns.
- Vulkan Specification/Registry for exact stage masks, access masks, image layouts, and WSI rules.
- LunarG Synchronization Validation docs for validation setup and expected diagnostics.
- Khronos Vulkan Tutorial frame synchronization chapters for first binary semaphore/fence shape.
- Vulkan Samples only after the first path is clean.
- How to Vulkan in 2026 as a secondary practical reference for first render-loop ordering and where synchronization2 can simplify later command/barrier code.

If source examples disagree, the Vulkan Specification/Registry wins for exact API behavior.

## Scope

In scope:

- binary WSI semaphores;
- per-frame fences;
- command-buffer reset safety;
- acquire/submit/present wait/signal rules;
- swapchain image ownership and layout expectations;
- depth image layout and reuse;
- upload completion before draw;
- resize/recreate safety;
- destruction safety;
- sync validation expectations;
- diagnostics fields and failure reason codes.

Out of scope:

- gameplay tick scheduling;
- runtime replay determinism;
- multithreaded rendering;
- async compute;
- transfer queue ownership transfers beyond optional future growth;
- timeline semaphore adoption;
- descriptor update-after-bind policy.

## Local File Surface

Likely future files:

```text
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/VulkanBackend.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`FrameSync` owns frame semaphores/fences. Resource modules own upload synchronization helpers. Swapchain owns acquire/present result handling. Backend coordinates them per frame.

## Ownership

| Object | Owner | Lifetime |
| --- | --- | --- |
| image-available semaphores | `FrameSync` | frame-slot/device lifetime |
| render-finished semaphores | `FrameSync` | frame-slot/device lifetime |
| in-flight fences | `FrameSync` | frame-slot/device lifetime |
| upload fences or wait path | resource/upload helper | upload lifetime |
| image barriers | command/resource helpers | command recording lifetime |
| wait-idle calls | backend/lifetime policy | shutdown/recreate/fallback only |

Rules:

- sync objects are private Vulkan backend state;
- runtime/projection never see semaphores, fences, barriers, stages, access masks, or layouts;
- sync objects are destroyed before logical device destruction;
- fence wait/reset policy must be explicit;
- every resource destruction path must prove the GPU is done with that resource.

## Baseline Policy

First implementation:

```text
sync_policy=binary_wsi
frames_in_flight=2
timeline_semaphores=deferred
wait_idle_normal_frame=false
sync_validation=enabled_when_available
```

Rationale:

- binary semaphores are the standard WSI acquire/present path;
- two frames in flight is enough for first visual proof without hiding bugs behind deeper buffering;
- fences give clear command-buffer reset and frame-slot reuse rules;
- timeline semaphores are useful later for internal uploads/async work but not required for first room.

Hard rule: if binary WSI sync fails, the renderer is not ready. Do not hide broken acquire/present sync behind timeline semaphores.

## Frame Slot State

Each frame slot owns:

```text
frame_slot_index
image_available_semaphore
render_finished_semaphore
in_flight_fence
primary_command_buffer
last_acquired_image_index
```

Frame slot states:

```text
idle
waiting_for_fence
acquiring
recording
submitted
presented_or_skipped
failed
```

Rules:

- frame slot starts with fence signaled so the first frame can proceed;
- command buffer reset requires the frame slot fence to be signaled;
- fence is reset only immediately before submitting work that will signal it;
- if acquire fails before submit, do not reset the fence for a non-submitted frame;
- if command recording fails after fence reset but before submit, the implementation must avoid deadlocking the next wait.

The last rule needs careful implementation. Prefer resetting the fence immediately before `vkQueueSubmit`, after command recording succeeds.

## Per-Frame Sync Order

Recommended order:

1. Choose current frame slot.
2. Wait for frame slot fence.
3. Acquire swapchain image, signaling image-available semaphore.
4. Handle acquire failure/skip without touching the fence if no submit will happen.
5. Reset frame slot fence.
6. Reset and record command buffer.
7. Submit command buffer:
   - wait on image-available semaphore;
   - signal render-finished semaphore;
   - signal frame slot fence.
8. Present:
   - wait on render-finished semaphore.
9. Advance frame slot index.

If command recording is done before fence reset in the final implementation, document how the command buffer reset remains legal. The first implementation should keep the order simple and validation-clean.

## Acquire Synchronization

Acquire rules:

- acquire uses current frame slot image-available semaphore;
- acquire timeout should be effectively infinite or otherwise explicitly diagnosed;
- successful acquire provides image index for this frame only;
- out-of-date acquire marks swapchain dirty and skips submit/present;
- suboptimal acquire may draw but must mark diagnostics;
- acquire failure does not mutate runtime.

Acquire result behavior:

| Result | Submit? | Present? | Fence reset? | Action |
| --- | --- | --- | --- | --- |
| success | yes | yes | yes, before submit | normal frame |
| suboptimal | yes or next-frame recreate | yes if drawing | yes if submit | diagnose |
| out of date | no | no | no | recreate swapchain |
| timeout/not ready | no | no | no | diagnose/fail according to strictness |
| device lost | no | no | no | device-loss policy |
| other error | no | no | no | fail frame |

## Submit Synchronization

Submit waits:

```text
wait_semaphore=image_available_semaphore
wait_stage=color_attachment_output_or_appropriate_rendering_stage
```

Submit signals:

```text
signal_semaphore=render_finished_semaphore
signal_fence=in_flight_fence
```

Rules:

- submit happens only after command buffer recording succeeds;
- fence reset happens only for a submit that will signal the fence;
- submit failure is renderer failure;
- if submit fails after fence reset, renderer should enter failed/device-lost cleanup rather than continuing into a deadlock;
- command buffer and per-frame resources stay in use until fence signals.

The exact wait stage/access masks must be validated against the chosen dynamic rendering/barrier implementation.

## Present Synchronization

Present waits:

```text
wait_semaphore=render_finished_semaphore
```

Rules:

- present happens only after submit succeeds;
- present queue is explicit;
- present result is diagnosed;
- out-of-date present marks swapchain dirty for recreate;
- suboptimal present is allowed but diagnosed;
- present failure does not mutate runtime.

If graphics and present queues differ, the swapchain/command file plan must define sharing mode or queue-family ownership transfer. Do not leave this implicit.

## Image Layout And Barrier Policy

First implementation must use explicit layout expectations.

Swapchain color image:

```text
before_render=present_or_undefined_after_acquire
during_render=color_attachment_optimal
before_present=present_src
```

Depth image:

```text
after_create=undefined
during_render=depth_attachment_optimal
after_render=depth_attachment_optimal
```

Rules:

- command recording must transition attachments or use render pass/dynamic rendering layout declarations according to Vulkan rules;
- old swapchain image views cannot be used after recreate;
- depth image layout is recreated/re-established with depth resource recreation;
- validation-clean layout handling is required before first-room smoke passes.

Exact helper names belong in file plans, but required barrier diagnostics should include source/destination stage, access, old layout, and new layout when a barrier is emitted.

## Upload Synchronization

First-room upload path may be simple and blocking:

1. Create staging resource.
2. Record copy into upload command buffer.
3. Submit upload work.
4. Wait for upload completion.
5. Destroy staging resource.
6. Mark destination resource ready for draw.

Rules:

- uploaded vertex/index/depth/texture resources cannot be used by draw until upload completion is proven;
- staging cannot be destroyed before copy completes;
- upload wait may use fence wait or device idle for the first bootstrap, but diagnostics must name which one;
- upload sync failure is renderer/resource failure, not runtime failure.

Growth path:

- batched upload fence;
- reusable staging buffer;
- timeline semaphore for internal upload only after binary WSI is clean;
- transfer queue support only after ownership transfer policy is designed.

## Resize And Recreate Synchronization

Before destroying swapchain-dependent resources:

- stop submitting new frames that use the old swapchain;
- wait for in-flight frames that may reference old swapchain images/views/depth;
- then destroy/recreate swapchain-dependent resources;
- keep device-lifetime resources alive when allowed by [lifetime.md](lifetime.md).

Allowed first implementation:

```text
resize_sync_policy=device_wait_idle
```

Preferred growth path:

```text
resize_sync_policy=wait_affected_frame_fences
```

Rules:

- no command buffer may record against old image views after recreate begins;
- no old depth image may be destroyed while in use;
- recreate failure leaves renderer in diagnosed not-drawable or failed state;
- runtime/projection state is unchanged by resize synchronization.

## Destruction Synchronization

Before destroying GPU resources:

- if resource is frame-slot-owned, wait for that frame fence;
- if resource is swapchain-dependent, follow resize/recreate wait policy;
- if resource is upload staging, wait for upload completion;
- before full renderer shutdown, wait for device idle or all outstanding fences.

Forbidden:

- freeing allocation while GPU may read/write it;
- destroying command pool while command buffers may still be in use;
- destroying semaphore/fence while a queue operation may signal/wait it;
- destroying surface/swapchain while present may still reference it.

## Fence Timeout Policy

First file plans must define a fence wait timeout.

Recommended first smoke policy:

```text
fence_wait_timeout_ns=1000000000
fence_wait_retry_count=5
```

Behavior:

- timeout emits diagnostics with frame slot and stage;
- repeated timeout fails strict smoke;
- interactive demo may attempt device idle or fail renderer cleanly;
- timeout does not mutate runtime.

Open: exact timeout values may change during implementation, but infinite waits in tests should be avoided because they hide deadlocks.

## Timeline Semaphore Gate

Timeline semaphores are deferred.

Adoption gate:

- binary WSI path passes validation/syncval;
- first-room render is stable;
- upload/resource growth needs better internal scheduling;
- device feature support is queried and diagnosed;
- fallback behavior exists when unavailable;
- sync validation remains clean.

Allowed future policy:

```text
sync_policy=mixed
wsi_sync=binary_semaphores
internal_sync=timeline_semaphore
```

Timeline semaphores must not replace required WSI acquire/present binary behavior unless Vulkan platform evidence and docs justify it.

## Sync Validation

Strict Vulkan lanes require synchronization validation when available.

Required behavior:

- print whether sync validation is enabled, unavailable, or disabled;
- strict sync lane fails if required sync validation is unavailable;
- any sync validation error fails strict smoke;
- validation messages are renderer diagnostics only and cannot affect runtime truth.

Common blockers:

- missing image layout transition;
- wrong stage/access masks;
- command buffer reset while in use;
- submit waiting on wrong semaphore;
- present without render-finished wait;
- draw reads upload destination before copy completes;
- resize destroys in-use image/view/depth resource.

## Diagnostics Receipt Fields

Sync diagnostics should include:

```text
sync_policy=binary_wsi
frames_in_flight=2
frame_slot=
frame_fence_state=
fence_wait_timeout_ns=
fence_wait_result=
image_available_semaphore_count=
render_finished_semaphore_count=
in_flight_fence_count=
acquire_wait_object=image_available_semaphore
submit_wait_stage=
submit_signal_semaphore=render_finished_semaphore
present_wait_semaphore=render_finished_semaphore
barrier_count=
layout_transition_count=
upload_sync_policy=
upload_wait_count=
resize_sync_policy=
device_wait_idle_count=
sync_validation=enabled|unavailable|disabled
sync_validation_error_count=
reason=
```

Use `unavailable` only for fields not applicable to the current lane.

## Failure Reason Codes

Recommended reason codes:

```text
fence_wait_failed
fence_wait_timeout
fence_reset_failed
semaphore_create_failed
fence_create_failed
acquire_sync_failed
submit_sync_failed
present_sync_failed
barrier_invalid
layout_transition_invalid
upload_sync_failed
resize_wait_failed
destroy_in_use_blocked
sync_validation_failed
sync_validation_required_missing
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during file planning.

## Platform Notes

macOS/MoltenVK:

- validate dynamic rendering and sync behavior through MoltenVK diagnostics;
- keep sync simple because translation through Metal can obscure mistakes;
- strict local smoke should print sync validation availability honestly.

Linux:

- native Vulkan lane should run sync validation on a real display-backed path where possible;
- Wayland/X11 differences should not change frame sync policy, only swapchain/surface setup;
- software Vulkan fallback is optional and must be diagnosed as such.

Windows:

- native Vulkan lane should test resize/minimize/restore with sync validation;
- installed Vulkan loader/runtime checks are packaging/platform concerns;
- fence timeouts should produce diagnostics instead of hanging indefinitely.

## Tests

Future tests:

```text
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_memory_smoke.cpp
```

Smoke expectations:

- frame sync objects create/destroy validation-clean;
- two frame slots can render multiple frames without command-buffer reset errors;
- acquire/submit/present use expected wait/signal objects;
- upload completion is proven before draw;
- resize waits before destroying old swapchain/depth resources;
- strict sync validation reports zero errors.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_sync|vulkan_empty_frame|vulkan_first_room|vulkan_resize|vulkan_memory'
```

## Acceptance Criteria

This sync contract is ready for file plans when:

- binary WSI baseline is explicit;
- frame-slot fence/semaphore ownership is defined;
- acquire/submit/present wait/signal rules are documented;
- command-buffer reset safety is defined;
- image layout/barrier expectations are named;
- upload completion rules are explicit;
- resize/destruction wait rules are explicit;
- fence timeout policy is required;
- timeline semaphore adoption gate is explicit;
- sync validation behavior is defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- renderer sync failure cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact `FrameSync` C++ type shape;
- exact fence timeout constants;
- exact barrier helper API and source/destination masks;
- exact dynamic rendering attachment layout transitions;
- exact queue-family ownership policy when graphics/present differ;
- exact upload fence helper;
- exact resize wait implementation;
- exact sync validation enablement flags per SDK/platform;
- exact Linux and Windows GPU smoke commands.
