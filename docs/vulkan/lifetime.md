# Vulkan Lifetime

This document defines Vulkan object lifetime, teardown order, resize/recreate order, wait-idle points, and failure cleanup policy.

The goal is boring correctness. Vulkan lifetime mistakes produce validation errors, device loss, black frames, or shutdown crashes. Every renderer file plan that owns Vulkan objects must point back here or explain why it differs.

## Ownership Principle

The module that creates a Vulkan object owns destroying it unless ownership is explicitly transferred.

Rules:

- no raw Vulkan object has ambiguous owner;
- parent objects outlive child objects;
- swapchain-dependent resources are separated from device-lifetime resources;
- renderer shutdown is valid even after partial initialization failure;
- runtime/projection/save do not participate in Vulkan lifetime.

## Lifetime Classes

| Class | Examples | Recreated on resize | Destroyed on shutdown |
| --- | --- | --- | --- |
| Process/app lifetime | SDL initialization, app config | no | yes |
| Vulkan instance lifetime | instance, debug messenger | no | yes |
| Device lifetime | physical/logical device, queues, allocator | no | yes |
| Surface lifetime | `VkSurfaceKHR` | usually no | yes before instance |
| Swapchain lifetime | swapchain, swapchain image views | yes | yes |
| Frame lifetime | semaphores, fences, command buffers | sometimes | yes |
| Pipeline lifetime | pipeline layout, pipeline, shader modules | if format/layout changes | yes |
| Resource lifetime | depth image, vertex/index buffers, textures | depth yes, mesh usually no | yes |
| Transient upload lifetime | staging buffers, one-time command buffers | no, per upload | after upload completes |

## Creation Order

Recommended first full creation order:

1. App parses config and runtime fixture path.
2. Runtime/content/projection systems initialize independently of renderer.
3. SDL video subsystem initializes.
4. SDL window is created.
5. Platform shell reports required Vulkan instance extensions.
6. Vulkan instance is created.
7. Debug messenger is created if validation is enabled.
8. Surface is created from platform shell callback.
9. Physical device is selected.
10. Logical device and queues are created.
11. VMA allocator or bootstrap allocation helper is created.
12. Swapchain is created.
13. Swapchain image views are created.
14. Depth resources are created.
15. Command pools and command buffers are created.
16. Frame sync objects are created.
17. Shader modules are created from SPIR-V.
18. Pipeline layout is created.
19. Graphics pipeline is created.
20. Shader modules are destroyed if no longer needed after pipeline creation.
21. First-room vertex/index resources are created and uploaded.
22. Renderer is ready for frames.

Notes:

- steps 11 and 21 may be narrow manual bootstrap before VMA adoption;
- pipeline may be created before or after first-room buffers as long as dependencies are explicit;
- shader compile is build-time, not runtime startup, unless running a shader policy tool.

## Shutdown Order

Recommended full shutdown order:

1. Stop submitting new frames.
2. Wait for device idle or wait for all in-flight frame fences.
3. Destroy transient upload resources that remain.
4. Destroy first-room buffers and persistent GPU resources.
5. Destroy descriptor pools/sets if introduced.
6. Destroy graphics pipelines.
7. Destroy pipeline layouts.
8. Destroy shader modules if any remain.
9. Destroy frame sync objects.
10. Destroy command buffers/pools.
11. Destroy depth resources.
12. Destroy swapchain image views.
13. Destroy swapchain.
14. Destroy VMA allocator or bootstrap allocation helper.
15. Destroy logical device.
16. Destroy surface.
17. Destroy debug messenger.
18. Destroy Vulkan instance.
19. Destroy SDL window.
20. Quit SDL video subsystem.
21. Shut down app-level renderer wrapper.

Rules:

- device must outlive resources allocated from it;
- allocator must outlive allocations it owns;
- surface must outlive swapchain;
- instance must outlive surface/debug messenger;
- SDL window must outlive the surface if the platform implementation requires it;
- shutdown must tolerate partially initialized modules.

## Partial Initialization Cleanup

Every initialization step must have a cleanup path for failure after it.

Pattern:

```text
create A
create B
create C fails
destroy B
destroy A
return stable diagnostic
```

Rules:

- failure cleanup follows reverse order of completed creation;
- cleanup functions must tolerate empty/null handles;
- failure cleanup emits one primary reason code and may emit detail fields;
- failure cleanup must not mutate runtime state;
- visual app exits nonzero in strict Vulkan lane.

Common failure reason codes:

```text
instance_create_failed
surface_create_failed
device_select_failed
device_create_failed
swapchain_create_failed
shader_module_create_failed
pipeline_create_failed
allocation_failed
upload_failed
```

## Resize/Recreate Order

Swapchain resize/recreate affects swapchain-dependent resources only.

Trigger examples:

- SDL window framebuffer size changed;
- swapchain out of date;
- surface suboptimal;
- present/acquire indicates recreate needed;
- format/extent changed.

Recommended recreate order:

1. Mark framebuffer resized from platform event or WSI result.
2. If framebuffer size is zero, mark renderer not drawable and skip presentation.
3. Wait for in-flight frames that use swapchain resources.
4. Destroy depth resources.
5. Destroy framebuffers if render-pass fallback exists.
6. Destroy swapchain image views.
7. Create new swapchain, passing the previous swapchain as `oldSwapchain` if the implementation path supports it.
8. Destroy old swapchain after new swapchain creation succeeds or after the failure path has finished using it.
9. Create new swapchain image views.
10. Create depth resources for new extent.
11. Recreate framebuffers if render-pass fallback exists.
12. Recreate or validate pipelines if color/depth format changed.
13. Update diagnostics.
14. Resume drawing.

Resources that should survive resize:

- Vulkan instance;
- physical/logical device;
- queues;
- surface;
- allocator;
- first-room vertex/index buffers;
- shader source/artifacts;
- most pipelines if format-compatible;
- runtime/projection state.

Resources usually recreated on resize:

- swapchain;
- swapchain image views;
- depth image/view/allocation;
- framebuffers if render-pass fallback exists;
- pipeline if render target formats change.

Failure policy:

- if new swapchain creation fails but the old swapchain is still valid, keep renderer in `not_drawable` state and report the failure;
- if old swapchain ownership has already been transferred to a failed creation path and cannot be safely reused, destroy dependent resources and require full renderer recreation;
- strict smoke treats repeated recreate failure as a Vulkan failure, not a runtime failure.

Diagnostic fields:

```text
swapchain_old_handle_used=true|false
swapchain_recreate_reason=
swapchain_recreate_result=
drawable=true|false
```

## Platform Lifetime Notes

The renderer must handle macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan without moving platform ownership into runtime.

macOS/MoltenVK:

- SDL owns the native window and Metal-compatible view/layer details;
- Vulkan backend owns the `VkSurfaceKHR` created through the SDL/provider callback;
- MoltenVK-specific setup belongs in platform shell or Vulkan backend diagnostics, not runtime;
- surface/window destruction order must be tested because Apple window/layer lifetime errors often show up as black frames or shutdown crashes.

Linux:

- Wayland and X11 surface behavior can differ; the platform shell must report which WSI path is active;
- renderer code should not assume X11-only handles;
- minimize, compositor resize, and zero-size drawable behavior must be part of smoke or manual validation;
- surface capabilities may change across recreate and must be re-queried.

Windows:

- Win32 surface lifetime is tied to the native window handle staying valid;
- DPI and resize events may arrive separately from drawable extent changes;
- swapchain recreate must tolerate minimize/restore and alt-tab behavior;
- installed package validation must confirm Vulkan loader availability separately from renderer code correctness.

Cross-platform rules:

- platform-specific native handles are not exposed to runtime/projection/save;
- surface creation is the only approved bridge from platform shell into Vulkan backend;
- each platform lane must print `platform=`, `wsi_backend=`, `surface_created=`, and `drawable_extent=`;
- platform differences may change diagnostics and fallback behavior, but not deterministic runtime state.

## Wait-Idle Policy

Use wait-idle sparingly but deliberately in first implementation.

Required wait/sync points:

- before full renderer shutdown;
- before destroying swapchain-dependent resources if simpler fence tracking is not yet implemented;
- before destroying upload staging resources if upload completion is not otherwise fenced;
- before device destruction.

Preferred long-term policy:

- wait only for affected frame fences;
- avoid device-wide idle during normal frames;
- avoid device-wide idle for every upload after first-room bootstrap;
- record when device idle is used in diagnostics if it affects performance.

Diagnostics:

```text
wait_idle_count=
swapchain_recreate_count=
upload_wait_count=
```

## Frame-In-Flight Lifetime

Default:

```text
frames_in_flight=2
sync_policy=binary_wsi
```

Per-frame owns:

- image-available/acquire semaphore;
- render-finished semaphore;
- in-flight fence;
- command buffer or command buffer slot;
- transient per-frame state if introduced.

Rules:

- frame resources are reused only after fence signals;
- command buffer reset/record happens only when safe;
- acquired swapchain image ownership is tracked;
- present happens only after render-finished semaphore is signaled;
- runtime tick scheduling is not owned by frame sync.

## Upload Lifetime

First-room upload path may be simple:

1. Create staging buffer.
2. Map/copy data.
3. Record copy command.
4. Submit copy.
5. Wait for completion.
6. Destroy staging buffer.
7. Keep device-local buffer for drawing.

Rules:

- staging buffer cannot be destroyed before copy completes;
- destination buffer/image cannot be used for draw before upload completes;
- upload failure produces renderer diagnostic;
- upload does not mutate runtime truth.

Growth path:

- reusable staging buffers;
- batched uploads;
- transfer queue if useful;
- no per-frame allocation churn;
- upload diagnostics.

## Shader/Pipeline Lifetime

Shader modules:

- created from SPIR-V bytes;
- used for graphics pipeline creation;
- destroyed after pipeline creation unless debugging requires retention.

Pipeline layout:

- owns push constant ranges and descriptor set layout compatibility;
- outlives pipelines using it;
- destroyed after pipelines.

Graphics pipeline:

- owns compiled pipeline state;
- depends on render target formats for dynamic rendering metadata or render-pass fallback compatibility;
- destroyed before pipeline layout and device.

Recreate pipeline when:

- swapchain color format changes;
- depth format changes;
- render path changes;
- shader artifacts change and hot reload is explicitly introduced later.

## Resource Lifetime

Depth resources:

- swapchain-extent lifetime;
- recreated on resize;
- destroyed before swapchain/allocator/device teardown.

Vertex/index buffers:

- device/resource lifetime;
- survive swapchain resize;
- destroyed before allocator/device teardown.

Textures/materials:

- deferred until VMA/resource diagnostics gate;
- survive swapchain resize unless descriptor/pipeline policy requires otherwise.

Descriptor pools/sets:

- deferred;
- destroyed before descriptor set layouts/pipeline layout/device;
- must not be owned by runtime/projection.

## Surface Lifetime

Surface is created after instance and before device selection.

Rules:

- platform shell creates surface through approved provider/callback;
- Vulkan backend owns destroying `VkSurfaceKHR`;
- surface outlives swapchain;
- surface is destroyed before instance;
- SDL window should outlive surface unless platform implementation proves otherwise;
- surface is not runtime data.

Surface recreation is not part of the normal resize path. If a platform reports that the native window/surface is invalid, the first implementation should tear down and recreate the renderer backend rather than silently attempting an ad hoc surface swap.

Surface diagnostics:

```text
surface_created=true|false
surface_recreated=true|false
surface_recreate_policy=full_backend_recreate
surface_platform=
wsi_backend=
```

## Device Loss Policy

Device loss is not a normal fallback path.

If Vulkan reports device loss:

1. Stop submitting frames.
2. Emit a renderer diagnostic with the failing operation.
3. Do not mutate runtime state.
4. Destroy renderer objects using best-effort cleanup.
5. Exit nonzero in strict visual smoke.

First implementation should not attempt automatic device recovery. Recovery can be planned later after the renderer has stable ownership, diagnostics, and platform smoke coverage.

Reason codes:

```text
device_lost
queue_submit_failed
present_failed
acquire_failed
```

Required receipt fields:

```text
device_lost=true|false
device_lost_stage=
renderer_recovery_attempted=false
```

## Validation Expectations

Lifetime-related validation failures are packet blockers.

Examples:

- destroying parent before child;
- destroying in-use resource;
- using old swapchain image after recreate;
- command buffer reset while in use;
- freeing allocation before GPU work completes;
- presenting without proper semaphore wait.

Strict smoke must fail on validation errors.

## Diagnostics Fields

Recommended lifetime receipt fields:

```text
lifetime_stage=
objects_created=
objects_destroyed=
partial_init_cleanup=true|false
swapchain_recreate_count=
wait_idle_count=
upload_wait_count=
frames_in_flight=
in_flight_frame_count=
resource_destroy_order=normal|partial|recreate
```

Use `unavailable` for fields that do not apply yet.

## Tests

Smoke tests that touch lifetime:

```text
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Required coverage over time:

- create and destroy backend once;
- create and destroy backend repeatedly;
- fail device/surface/swapchain creation and cleanup partially;
- resize swapchain repeatedly;
- minimize/restore zero-size drawable;
- run sustained frames with two frames in flight;
- upload first-room buffers and destroy staging after completion.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|vulkan_swapchain|vulkan_sync|vulkan_memory'
```

## Acceptance Criteria

Lifetime policy is satisfied when:

- each Vulkan object has one owner;
- creation order is documented;
- destruction order is documented;
- resize/recreate order is documented;
- partial initialization cleanup is documented;
- wait-idle/fence policy is documented;
- validation errors fail strict smoke;
- runtime/projection/save do not participate in Vulkan lifetime.

## Open Detail Items

These belong in future file plans:

- exact RAII wrapper strategy;
- exact null-handle cleanup helper pattern;
- exact old-swapchain handoff behavior;
- exact fence wait timeout policy;
- exact upload command helper API;
- exact validation test for repeated create/destroy;
- exact diagnostics counter implementation.
