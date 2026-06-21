# Vulkan Command Recording

This document defines how the Vulkan backend records draw commands for one frame.

Command recording is renderer-owned. It consumes already validated `FrameInput`, already selected swapchain/depth attachments, already created pipelines, and already uploaded GPU resources. It does not poll input, advance runtime, inspect save data, or fix projection output.

## Purpose

Define the exact command-buffer contract for first-room rendering:

```text
frame slot available
reset command buffer
begin command buffer
transition/render attachments as needed
begin dynamic rendering
set viewport/scissor
bind pipeline
bind vertex/index buffers
push clip matrix
draw scene/debug items
end rendering
end command buffer
return command recording result
```

This document sits below [render_loop.md](render_loop.md). The render loop decides when recording happens. This document decides what recording means.

## Scope

In scope:

- graphics command pool ownership;
- primary command buffer allocation;
- reset/begin/end rules;
- dynamic rendering command sequence;
- render-pass fallback boundary;
- viewport and scissor setup;
- color/depth attachment use;
- pipeline bind order;
- vertex/index bind order;
- push constant update point;
- draw call ordering;
- debug draw recording;
- diagnostics and failure behavior.

Out of scope:

- runtime command admission;
- projection generation;
- shader compilation;
- pipeline creation;
- GPU memory allocation;
- upload command buffer details except where draw safety depends on them;
- multithreaded secondary command buffers.

## Local File Surface

Likely future files:

```text
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/BuffersImagesMemory.hpp
tests/smoke/vulkan_command_recording_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`CommandBuffers` owns command pools and command buffers. `VulkanBackend` decides frame order and calls command recording. Pipeline/resource modules provide handles but do not own recording policy.

## Ownership

| Object | Owner | Lifetime |
| --- | --- | --- |
| graphics command pool | `CommandBuffers` | device lifetime, recreated only if queue family/device changes |
| per-frame primary command buffers | `CommandBuffers` | frame-slot lifetime |
| one-time upload command buffers | resource/upload helper | upload lifetime, not normal draw frame |
| command recording result | `CommandBuffers` or backend-private helper | one frame |
| command diagnostics | `RenderDiagnostics` | emitted after frame or failure |

Rules:

- command pool belongs to the graphics queue family;
- command buffers are reset only after the owning frame fence is signaled;
- command buffers do not own pipelines, buffers, images, or swapchain images;
- command recording must not retain `FrameInput` pointers after return;
- runtime/projection code never sees Vulkan command objects.

## Dependency Rules

Allowed dependencies:

- `CommandBuffers` may include Vulkan private types;
- `CommandBuffers` may depend on backend-private swapchain attachment views;
- `CommandBuffers` may depend on backend-private pipeline/resource handles;
- `CommandBuffers` may read backend-neutral `FrameInput` through renderer public types.

Forbidden dependencies:

- `src/runtime/**` including Vulkan headers;
- `src/projection/**` including Vulkan headers;
- command recording calling runtime mutation APIs;
- command recording owning platform window handles;
- command recording compiling shaders or allocating per-frame pipelines.

Review grep:

```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result: no runtime/content/projection/save/public-render Vulkan leaks.

## Command Pool Policy

First implementation:

```text
graphics_command_pool_count=1
command_pool_queue_family=graphics_queue_family
command_pool_reset_policy=reset_individual_buffers_or_pool_per_frame
```

Required command pool capabilities:

- allocate primary command buffers for each frame slot;
- allow reset at a safe point;
- support normal graphics command recording;
- be destroyed before logical device destruction.

Recommended flags:

- use a reset-capable pool or per-command-buffer reset policy for frame command buffers;
- do not optimize command pool flags until profiling proves a need.

Forbidden early complexity:

- one command pool per subsystem;
- worker-thread command pools;
- secondary command buffers for first-room proof;
- rebuilding command pools every frame.

## Command Buffer Allocation

Default:

```text
frames_in_flight=2
primary_command_buffers=2
command_buffer_per_frame_slot=1
```

Rules:

- command buffer count follows frame slots, not swapchain image count;
- command buffer is associated with the current frame slot;
- acquired swapchain image index selects attachments, not command buffer ownership;
- command buffer is reset only after the frame slot fence is signaled;
- command buffer is recorded once for the acquired image.

If a future path records per-swapchain-image command buffers, it must be a deliberate decision because resize, resource lifetime, and synchronization get more complicated.

## Recording Preconditions

Before recording begins:

- `FrameInput` validation passed;
- viewport is drawable;
- swapchain exists and is not dirty;
- acquired image index is valid;
- current frame fence has signaled;
- current command buffer is safe to reset;
- color image view for acquired swapchain image exists;
- depth image/view exists if depth testing is enabled;
- first-room pipeline exists;
- first-room vertex/index resources exist or fallback no-geometry skip is diagnosed;
- shader/pipeline layout matches command-time push constants.

If any precondition fails, command recording returns failure or diagnosed skip. It must not call runtime to repair the data.

## Record Order

Recommended first-room record order:

1. Reset command buffer.
2. Begin command buffer.
3. Prepare color attachment state.
4. Prepare depth attachment state.
5. Begin dynamic rendering.
6. Set viewport.
7. Set scissor.
8. Bind first-room graphics pipeline.
9. Bind first-room vertex buffer.
10. Bind first-room index buffer if indexed drawing is used.
11. For each scene draw item:
    - compute `clipFromModel`;
    - push constants;
    - issue draw or indexed draw.
12. For each debug draw item, if enabled:
    - bind debug-compatible pipeline if distinct;
    - push constants;
    - issue draw.
13. End dynamic rendering.
14. End command buffer.

The first implementation should keep one graphics pipeline for first-room geometry unless debug drawing requires a separate pipeline. Do not introduce material pipelines until the descriptor/material phase.

## Dynamic Rendering Baseline

Preferred baseline:

```text
rendering_path=dynamic_rendering
color_attachment=swapchain_image_view[acquired_image_index]
depth_attachment=depth_image_view
load_op=clear
store_op=store
depth_load_op=clear
depth_store_op=store_or_dont_care
```

Rules:

- pipeline creation must name color and depth formats compatible with dynamic rendering;
- command recording must use the current swapchain extent;
- color clear value must be stable and diagnosed for first smoke;
- depth clear should be `1.0`;
- dynamic rendering failure is a Vulkan failure unless fallback was explicitly selected before startup.

Render-pass fallback:

- allowed only through [fallbacks.md](fallbacks.md);
- must be diagnosed as `rendering_path=render_pass`;
- must define framebuffer lifetime in [lifetime.md](lifetime.md);
- must not change runtime/projection contracts.

## Attachment Layout Policy

First implementation should use simple, explicit layout handling.

Color attachment:

- acquired swapchain image must be usable as color attachment;
- command recording transitions or relies on synchronization setup according to the selected implementation;
- final layout must be present-compatible before present.

Depth attachment:

- depth image must be in depth-attachment layout before drawing;
- depth image is swapchain-extent lifetime;
- depth layout transitions are renderer/resource-owned.

Validation blockers:

- drawing with attachment in wrong layout;
- presenting image not transitioned for present;
- depth image missing required usage;
- using image view from old swapchain after recreate.

Exact barrier helper shape belongs in the sync contract/file plan, but command recording must state which layouts it expects.

## Viewport And Scissor

First pipeline uses dynamic viewport and scissor.

Rules:

- viewport width and height come from current swapchain extent or validated drawable viewport;
- scissor covers the full drawable extent for first-room proof;
- viewport/scissor are set every recorded frame;
- zero-size viewport is rejected before command recording;
- runtime camera truth does not own viewport size.

Default:

```text
viewport.x=0
viewport.y=0
viewport.width=swapchain_extent.width
viewport.height=swapchain_extent.height
viewport.minDepth=0.0
viewport.maxDepth=1.0
scissor.offset=0,0
scissor.extent=swapchain_extent
```

If vertical viewport flipping is needed for the chosen matrix convention, it must be documented in [frame_input_contract.md](frame_input_contract.md) or the pipeline file plan. Do not hide coordinate repair inside command recording.

## Pipeline Binding

First-room pipeline assumptions:

```text
pipeline_family=first_room
vertex_format=position_color
layout=push_constants_only
depth_test=enabled
depth_write=enabled
cull_mode=back_or_disabled_until_winding_confirmed
front_face=chosen_by_pipeline_plan
viewport=dynamic
scissor=dynamic
```

Rules:

- bind graphics pipeline after beginning rendering;
- bound pipeline must match current rendering path and attachment formats;
- pipeline layout must match push constant range;
- command recording does not create or recreate pipelines;
- pipeline format mismatch returns `command_record_failed` or earlier pipeline validation failure.

## Vertex And Index Binding

First-room geometry path:

- vertex buffer contains position and color;
- index buffer is allowed but not required;
- vertex stride and attribute layout match shader interface;
- draw count is reported in diagnostics.

Rules:

- bind vertex buffer before draw;
- bind index buffer before indexed draw;
- do not allocate or upload vertex data during command recording;
- missing required buffer causes diagnosed failure or fallback no-geometry skip depending on smoke strictness;
- fallback debug geometry must be labeled as renderer fallback, not runtime output.

## Push Constants

First-room shader uses one matrix push constant:

```text
push_constant_name=clipFromModel
push_constant_size=64
push_constant_stage=vertex
```

Rules:

- compute `clipFromModel = FrameInput.camera.clipFromWorld * drawItem.modelFromLocal`;
- validate matrix finiteness before push;
- push constants before each draw item whose transform differs;
- keep push constant use under guaranteed Vulkan minimum unless device diagnostics prove otherwise;
- large frame/material data moves to descriptors later.

Command recording may compute presentation matrices from renderer-neutral frame data. It may not update runtime camera truth.

## Draw Ordering

First-room ordering:

1. Opaque room/floor/wall proxy geometry.
2. Opaque interactable/objective/pickup proxy geometry.
3. Debug projection geometry.

Rules:

- depth test handles basic occlusion;
- translucent sorting is deferred;
- material sorting is deferred;
- renderer may batch/reorder only when diagnostics remain understandable;
- draw order must not affect runtime state or replay hash.

Diagnostics should count:

```text
scene_draw_count=
debug_draw_count=
indexed_draw_count=
nonindexed_draw_count=
fallback_draw_count=
```

## Debug Draw Recording

Debug projection rendering is presentation-only.

Rules:

- debug draw enable is app/render configuration, not runtime truth;
- debug draw items come from `DebugProjection`;
- debug draw may use the same first-room pipeline if vertex-color geometry is compatible;
- unsupported debug item kinds are skipped with diagnostics;
- debug labels/text are not required for first-room proof.

If debug drawing requires a different pipeline later, pipeline binding order and diagnostics must make that explicit.

## Failure Policy

Command recording failures are renderer failures.

Failure examples:

- command buffer reset failed;
- command buffer begin failed;
- dynamic rendering begin invalid;
- missing color/depth attachment;
- pipeline not compatible with attachment formats;
- vertex/index buffer missing;
- push constant range mismatch;
- invalid draw count;
- command buffer end failed.

Rules:

- no failure mutates runtime;
- strict smoke exits nonzero;
- interactive demo may keep running only if renderer can recover safely;
- validation/syncval error during command recording is a blocker;
- failure receipt must name the command-recording stage.

Reason codes:

```text
command_reset_failed
command_begin_failed
rendering_begin_failed
attachment_invalid
pipeline_bind_invalid
vertex_buffer_missing
index_buffer_invalid
push_constant_invalid
draw_record_failed
command_end_failed
```

## Diagnostics Receipt Fields

Command recording diagnostics should include:

```text
command_pool_created=true|false
command_pool_queue_family=
command_buffer_count=
command_buffer_level=primary
command_reset_policy=
frame_slot=
acquired_image_index=
record_begin=true|false
rendering_path=dynamic_rendering|render_pass
color_attachment_format=
depth_attachment_format=
viewport=
scissor=
pipeline_family=
pipeline_bound=true|false
vertex_buffer_bound=true|false
index_buffer_bound=true|false
push_constant_bytes=
scene_draw_count=
debug_draw_count=
command_recorded=true|false
command_record_stage=
reason=
```

Use `unavailable` only when the field truly does not apply.

## Validation And Sync Validation

Validation blockers:

- recording commands into a command buffer that is already in use;
- beginning rendering without compatible attachments;
- missing image layout transitions;
- invalid viewport/scissor extent;
- binding pipeline outside compatible render scope;
- binding buffers with incorrect usage flags;
- push constant range/stage mismatch;
- issuing draw with invalid vertex/index buffers.

Sync validation blockers:

- command buffer reset while GPU may still read it;
- drawing from uploaded buffers before upload completion;
- using swapchain/depth resources while they are being recreated/destroyed;
- presenting before rendering completes.

Command recording file plans must coordinate with the future sync contract before implementation.

## Platform Notes

macOS/MoltenVK:

- dynamic rendering support must be proven through device/feature diagnostics;
- validation-clean command recording matters because Metal translation errors can otherwise look like black frames;
- first smoke should keep command recording simple and avoid optional features.

Linux:

- native Vulkan validation must prove the same command order on at least one display-backed WSI lane;
- Wayland/X11 differences should not alter command recording, only platform/swapchain setup.

Windows:

- native Vulkan validation must prove command recording after resize/minimize/restore;
- installed runtime/loader checks are packaging concerns, not command recording behavior.

## Tests

Future tests:

```text
tests/smoke/vulkan_command_recording_smoke.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Smoke expectations:

- command pool creates and destroys validation-clean;
- command buffers allocate for each frame slot;
- one empty clear frame records and presents;
- first-room frame records at least one draw when projection data exists;
- resize does not record against old swapchain image views;
- validation and sync validation are clean.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_command_recording|vulkan_empty_frame|vulkan_first_room|vulkan_resize|vulkan_sync'
```

## Acceptance Criteria

This command recording contract is ready for file plans when:

- command pool ownership is explicit;
- command buffer count and frame-slot policy are defined;
- reset/begin/end safety rules are documented;
- dynamic rendering command order is documented;
- render-pass fallback boundary is explicit;
- attachment layout expectations are named;
- viewport/scissor policy is defined;
- pipeline, vertex/index, push constant, and draw order are defined;
- debug draw behavior is scoped;
- command recording failures have reason codes;
- validation/syncval blockers are named;
- Linux, Windows, and macOS/MoltenVK lanes are included.

## Open Detail Items

These belong in future file plans:

- exact C++ command buffer wrapper;
- exact reset mode: command buffer reset versus pool reset;
- exact barrier helper API;
- exact dynamic rendering begin helper;
- exact render-pass fallback helper if selected;
- exact first-room vertex/index buffer handle types;
- exact debug draw primitive mapping;
- exact screenshot or pixel-readback proof for command-recorded frame;
- exact platform smoke command for Linux and Windows GPU lanes.
