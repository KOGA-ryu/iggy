# Vulkan Image Layouts And Barriers

This document defines the first `iggy3d` Vulkan image layout and barrier policy.

Image layouts and barriers are renderer-owned GPU synchronization details. Runtime/content/projection may describe what should be drawn, but they must not own image layouts, access masks, pipeline stages, barriers, queue-family ownership transfers, or Vulkan image state.

## Purpose

Define a validation-clean first-room image path:

```text
acquire swapchain image
transition swapchain image to color attachment layout
transition depth image to depth attachment layout
begin dynamic rendering with color/depth attachments
draw first room
end dynamic rendering
transition swapchain image to present layout
present
```

Define first texture upload path:

```text
host writes staging buffer
copy staging buffer to image in transfer-dst layout
transition texture image to shader-read layout
sample texture from fragment shader later
```

This document narrows [command_recording.md](command_recording.md), [sync_contract.md](sync_contract.md), [gpu_resource_upload.md](gpu_resource_upload.md), [swapchain_contract.md](swapchain_contract.md), [depth_and_coordinates.md](depth_and_coordinates.md), [render_assets_and_materials.md](render_assets_and_materials.md), and [vulkan_feature_baseline.md](vulkan_feature_baseline.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification synchronization chapter for image layout transition rules, access availability/visibility, and old-layout requirements.
- Vulkan Guide synchronization examples for practical barrier patterns.
- Vulkan Guide `VK_KHR_synchronization2` page for later sync2 migration.
- Khronos Vulkan Tutorial texture image chapters for staging upload and sampled image layout transitions.
- Khronos Vulkan Tutorial depth/dynamic rendering chapter for depth attachment layout and clear behavior.
- LunarG Synchronization Validation docs for smoke validation.

## Scope

In scope:

- first-room swapchain color attachment layouts;
- first-room depth attachment layouts;
- staging-buffer-to-texture upload barriers;
- sampled texture steady-state layout;
- future render target layout names;
- queue-family ownership policy;
- sync1 vs sync2 baseline;
- diagnostics and failure reason codes.

Out of scope:

- full synchronization2 adoption;
- async transfer queue ownership transfers;
- render-pass implicit transitions;
- MSAA resolve layouts;
- transient attachments;
- shadow maps;
- storage images;
- compute image writes;
- image compression/BC/ASTC policy;
- bindless texture arrays.

## Local File Surface

Likely future files:

```text
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_image_layout_policy_tests.cpp
tests/unit/render_texture_upload_policy_tests.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

`CommandBuffers` owns per-frame swapchain/depth layout transitions around dynamic rendering. `BuffersImagesMemory` owns upload texture image transitions and sampled texture steady-state. `Swapchain` owns selected images/views, not the command-time transitions.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| swapchain image layout transitions | `CommandBuffers` or render pass helper | per-frame presentation path |
| depth image layout transitions | `CommandBuffers` or resource helper | per-frame or recreate-time |
| texture upload image transitions | `BuffersImagesMemory` | upload command path |
| sampled texture steady-state layout | `BuffersImagesMemory` | material/descriptor path |
| image layout diagnostics | `RenderDiagnostics` | names and counts |
| queue-family ownership transfers | resource/sync module | deferred until separate queues |

Rules:

- runtime/content/projection never store layout names;
- command recording must not guess texture upload state;
- resource upload must not mutate scene/runtime truth;
- barriers are renderer correctness, not gameplay behavior.

## Baseline Barrier API

First implementation:

```text
barrier_api=vkCmdPipelineBarrier
synchronization2=deferred
queue_family_ownership=single_graphics_queue_or_concurrent_swapchain_policy
```

Rationale:

- current sync baseline defers `synchronization2` as a hard gate;
- `vkCmdPipelineBarrier` is enough for first-room and first texture upload;
- sync2 migration can happen after first visual proof and sync validation are clean.

Rules:

- do not mix sync1 and sync2 barrier APIs in one helper without a deliberate file plan;
- if `synchronization2` is adopted later, update this document and `sync_contract.md`;
- diagnostics should print `barrier_api`.

## Layout Tracking Policy

First policy:

```text
track_swapchain_layout_per_acquired_image=diagnostic_only_initially
track_texture_layout_per_texture_record=required
track_depth_layout=resource_state_or_discard_each_frame_policy
```

Rules:

- swapchain color is cleared every first-room frame, so pre-render transition may use `oldLayout=VK_IMAGE_LAYOUT_UNDEFINED` to discard previous contents;
- if future code needs to preserve swapchain image contents, it must track and use the real old layout;
- texture records must know their steady-state layout before descriptor binding;
- any sampled image bound to a descriptor must be in shader-read layout.

## First-Room Swapchain Color Policy

First-room color attachment path:

```text
before_render:
  image=acquired_swapchain_image
  oldLayout=VK_IMAGE_LAYOUT_UNDEFINED
  newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  srcStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
  srcAccess=0
  dstStage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  dstAccess=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT

dynamic_rendering:
  color_attachment.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  loadOp=clear
  storeOp=store

after_render:
  oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  srcStage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  srcAccess=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  dstStage=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
  dstAccess=0
```

Rules:

- using `UNDEFINED` before render is allowed only because first-room color contents are discarded and fully cleared;
- after-render transition to present layout is required before `vkQueuePresentKHR`;
- present wait semaphore ensures the presentation engine waits for submitted rendering work;
- if preserve/load color contents is introduced later, `UNDEFINED` is no longer acceptable.

## First-Room Depth Policy

First-room depth attachment path:

```text
before_render:
  image=depth_image_for_swapchain_extent
  oldLayout=VK_IMAGE_LAYOUT_UNDEFINED
  newLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL or VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  srcStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
  srcAccess=0
  dstStage=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
  dstAccess=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT

dynamic_rendering:
  depth_attachment.imageLayout=selected_depth_attachment_layout
  loadOp=clear
  storeOp=dont_care for first room
  clearDepth=1.0
```

Rules:

- `UNDEFINED` is acceptable because first-room depth is cleared and previous depth contents are not used;
- depth attachment layout must match the format/aspect selected in [depth_and_coordinates.md](depth_and_coordinates.md);
- stencil layout/state is deferred unless a depth-stencil format and stencil use are explicitly adopted;
- if depth is sampled later, a new transition to read-only layout is required.

## Texture Upload Layout Policy

First sampled texture upload path:

```text
create_image:
  initialLayout=VK_IMAGE_LAYOUT_UNDEFINED
  usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT

transition_for_copy:
  oldLayout=VK_IMAGE_LAYOUT_UNDEFINED
  newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  srcStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
  srcAccess=0
  dstStage=VK_PIPELINE_STAGE_TRANSFER_BIT
  dstAccess=VK_ACCESS_TRANSFER_WRITE_BIT

copy:
  vkCmdCopyBufferToImage
  imageLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL

transition_for_sampling:
  oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  srcStage=VK_PIPELINE_STAGE_TRANSFER_BIT
  srcAccess=VK_ACCESS_TRANSFER_WRITE_BIT
  dstStage=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
  dstAccess=VK_ACCESS_SHADER_READ_BIT

steady_state:
  layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
```

Rules:

- command recording may bind a sampled texture only after upload completion and shader-read transition;
- texture descriptor image layout must be `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` for first material path;
- mip generation is deferred;
- compressed/linear texture policy is deferred to texture format docs;
- missing transition fails texture smoke.

## Staging Buffer Policy

Staging buffers do not have image layouts.

Rules:

- staging buffer is host-visible upload memory;
- non-coherent mapped writes must be flushed before transfer reads;
- coherent mapped writes still require correct command submission ordering;
- staging buffer must remain alive until copy command execution completes;
- staging buffer destruction is guarded by upload fence/wait policy;
- buffer barriers are added only when GPU writes then GPU reads require them.

First texture upload uses:

```text
staging_buffer_usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT
image_usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
copy_command=vkCmdCopyBufferToImage
```

## Future Render Target Policy

Deferred render targets should use explicit names:

```text
render_target.color
render_target.depth
render_target.resolve
render_target.sampled_history
render_target.screenshot_source
```

Likely transitions:

```text
UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL
TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
```

Rules:

- future render target docs must define whether contents are discarded, preserved, sampled, copied, or presented;
- no future render target may rely on implicit render-pass transitions in the dynamic-rendering path;
- each render target must have a diagnostic name and current layout tracking policy.

## Queue Family Ownership Policy

First implementation:

```text
graphics_queue=present_queue preferred
transfer_queue=not_used_for_uploads_initially
queue_family_ownership_transfers=deferred
```

Rules:

- first texture uploads should use graphics queue commands unless a transfer-queue packet is accepted;
- if graphics and present queue families differ, swapchain sharing mode and ownership rules must be explicit;
- no queue-family ownership transfer is allowed without a dedicated sync/lifetime plan;
- diagnostics must print whether ownership transfer was used.

## Barrier Helper Policy

Expected helper concepts:

```text
transitionSwapchainImageForRendering
transitionSwapchainImageForPresent
transitionDepthImageForRendering
transitionTextureImageForCopy
transitionTextureImageForSampling
```

Rules:

- helpers must take explicit old/new layout;
- helpers must take explicit aspect mask;
- helpers must name stage/access masks;
- helpers must emit diagnostics in strict smoke when requested;
- helpers must reject unsupported transition pairs unless a file plan adds them.

Avoid one generic "do anything" transition helper with hidden rules until the explicit transition table is stable.

## Transition Table

Initial accepted transitions:

| Use | Old layout | New layout | Source stage/access | Destination stage/access |
| --- | --- | --- | --- | --- |
| swapchain color begin | `UNDEFINED` | `COLOR_ATTACHMENT_OPTIMAL` | `TOP_OF_PIPE` / none | `COLOR_ATTACHMENT_OUTPUT` / color attachment write |
| swapchain color present | `COLOR_ATTACHMENT_OPTIMAL` | `PRESENT_SRC_KHR` | `COLOR_ATTACHMENT_OUTPUT` / color attachment write | `BOTTOM_OF_PIPE` / none |
| depth begin | `UNDEFINED` | `DEPTH_ATTACHMENT_OPTIMAL` or `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` | `TOP_OF_PIPE` / none | `EARLY_FRAGMENT_TESTS` / depth-stencil attachment write |
| texture upload begin | `UNDEFINED` | `TRANSFER_DST_OPTIMAL` | `TOP_OF_PIPE` / none | `TRANSFER` / transfer write |
| texture upload finish | `TRANSFER_DST_OPTIMAL` | `SHADER_READ_ONLY_OPTIMAL` | `TRANSFER` / transfer write | `FRAGMENT_SHADER` / shader read |

Rules:

- transitions outside this table require a document update or explicit file-plan section;
- strict tests should fail unsupported transition pairs in helper code;
- stage/access names in diagnostics should be stable strings.

## Dynamic Rendering Attachment Layouts

First-room dynamic rendering must pass:

```text
colorAttachment.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
depthAttachment.imageLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL or VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
```

Rules:

- attachment image layout in dynamic rendering info must match the actual image layout after barrier;
- color/depth attachment formats must match pipeline dynamic rendering format metadata;
- render-pass implicit layout transitions are not part of the first path.

## Diagnostics Receipt

Image layout smoke should print:

```text
image_layout_policy=first_room_dynamic_rendering_v1
barrier_api=vkCmdPipelineBarrier
synchronization2_enabled=false
swapchain_color_before_render=UNDEFINED->COLOR_ATTACHMENT_OPTIMAL
swapchain_color_after_render=COLOR_ATTACHMENT_OPTIMAL->PRESENT_SRC_KHR
depth_before_render=UNDEFINED->DEPTH_ATTACHMENT_OPTIMAL
texture_upload_begin=UNDEFINED->TRANSFER_DST_OPTIMAL
texture_upload_finish=TRANSFER_DST_OPTIMAL->SHADER_READ_ONLY_OPTIMAL
texture_steady_state=SHADER_READ_ONLY_OPTIMAL
queue_family_ownership_transfer_used=false
unsupported_transition_count=
layout_validation_clean=true|false
runtime_state_touched=false
reason=
```

Rules:

- receipts should print layout pairs, not only success/failure;
- texture smoke must print the sampled texture final layout;
- `runtime_state_touched=false` is mandatory.

## Failure Reason Codes

Use stable reason codes:

```text
image_layout_scope_blocked
image_layout_unsupported_transition
image_layout_old_layout_mismatch
image_layout_missing_swapchain_to_color
image_layout_missing_color_to_present
image_layout_missing_depth_transition
image_layout_missing_texture_transfer_dst
image_layout_missing_texture_shader_read
image_layout_dynamic_rendering_mismatch
image_layout_descriptor_mismatch
image_layout_bad_aspect_mask
image_layout_queue_family_transfer_unplanned
image_layout_staging_buffer_lifetime_unsafe
image_layout_sync_validation_failed
image_layout_runtime_leak
```

Rules:

- reason codes should appear in smoke output;
- reason codes must not vary by platform;
- Vulkan validation messages may be appended after stable renderer reason codes.

## Validation Expectations

Validation and sync validation should catch:

- attachment used in a layout that does not match dynamic rendering info;
- present attempted without present layout;
- copy to image not in transfer destination layout;
- sampled image descriptor layout mismatch;
- missing transfer-write to shader-read dependency;
- invalid aspect mask for depth/stencil image;
- unsafe resource destruction while upload/render work is in flight.

Tests should catch:

- required first-room layout transition table;
- texture upload transition sequence;
- unsupported transition rejection;
- runtime/content/projection leakage of layout names;
- diagnostics receipt fields.

Suggested firewall scan:

```sh
rg -n "VK_IMAGE_LAYOUT|VK_PIPELINE_STAGE|VK_ACCESS_|vkCmdPipelineBarrier|VkImageMemoryBarrier" src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result: no production runtime/content/projection/save/public-render leakage.

## Platform Notes

The layout/barrier policy is shared across macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan.

Rules:

- MoltenVK portability notes may explain backend constraints but do not change cross-platform layout truth;
- Linux and Windows native Vulkan smoke should use the same transition receipt schema;
- platform differences should appear as validation/driver diagnostics, not separate renderer architecture.

## Tests

Expected future tests:

```text
tests/unit/render_image_layout_policy_tests.cpp
tests/unit/render_texture_upload_policy_tests.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Unit tests should cover:

- accepted transition table;
- rejected unsupported transition pairs;
- aspect mask selection;
- layout diagnostic string stability;
- failure reason stability.

Smoke tests should cover:

- first-room color/depth transitions validation-clean;
- color-to-present transition before present;
- texture upload into shader-read layout;
- sampled texture descriptor layout matches resource layout;
- sync validation clean for upload-to-sample path.

## Acceptance Criteria

Image layout/barrier work is acceptable only when:

- first-room color transitions from discard/clear state to color attachment layout before dynamic rendering;
- first-room color transitions to present layout before present;
- depth image is in depth attachment layout before dynamic rendering;
- texture upload transitions through transfer-dst to shader-read layout;
- sampled texture descriptors use shader-read layout;
- unsupported transition pairs are rejected or explicitly documented;
- sync validation is clean for first-room and texture smoke;
- diagnostics print transition pairs and barrier API;
- runtime/content/projection/save scans show no layout/barrier leakage.

## Open Detail Items

The next detailed pass should define:

- exact C++ helper signatures for each accepted transition;
- exact aspect-mask helper for depth-only vs depth-stencil formats;
- exact old-layout tracking storage for texture records;
- exact policy for using `UNDEFINED` vs tracked `PRESENT_SRC_KHR` on swapchain images;
- exact migration point for synchronization2 barriers;
- exact future screenshot transition to `TRANSFER_SRC_OPTIMAL`;
- exact render-target sampled-history layout policy.
