# Vulkan GPU Resource Upload

This document defines how CPU-side renderer data becomes GPU-resident Vulkan buffers and images for `iggy3d`.

Upload is renderer-owned. Runtime/content/projection may produce or reference backend-neutral scene data, asset ids, and projection items, but they do not own Vulkan buffers, images, memory allocations, staging resources, upload command buffers, fences, or GPU residency.

## Purpose

Define the first upload contract:

```text
create device-local destination resource
create host-visible staging resource
map and copy CPU bytes into staging
flush if memory is not coherent
record copy command
submit upload command
wait for upload completion
destroy or recycle staging resource
mark destination ready for draw
print upload diagnostics
```

This document narrows [resource_model.md](resource_model.md), [sync_contract.md](sync_contract.md), [lifetime.md](lifetime.md), and [command_recording.md](command_recording.md) into an implementation-ready upload lane.

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for buffer/image usage flags, memory requirements, mapping, flushing, barriers, and copy commands.
- Vulkan Guide memory and synchronization topics.
- Vulkan Memory Allocator docs for allocation, mapping, naming, budgets, stats, and debugging.
- Khronos Vulkan Tutorial buffer, image, and staging chapters for first implementation shape.
- Vulkan Samples only after first-room upload is validation-clean.

## Scope

In scope:

- first-room vertex/index buffer upload;
- staging buffer ownership;
- host-visible memory mapping policy;
- device-local destination buffers;
- upload command buffer ownership;
- graphics queue upload baseline;
- upload fence/wait policy;
- staging destruction/reuse safety;
- image/texture upload growth contract;
- VMA adoption boundary;
- diagnostics and failure reason codes.

Out of scope:

- asset file parsing;
- material descriptor binding;
- streaming residency;
- asynchronous background loading;
- transfer queue ownership transfers;
- timeline semaphore adoption;
- mip generation beyond naming the future gate;
- runtime save/replay behavior.

## Local File Surface

Likely future files:

```text
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_memory_policy_tests.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`BuffersImagesMemory` owns resource creation, allocation, upload helpers, staging resources, resource names, and upload diagnostics.

## Ownership

| Item | Owner | Lifetime |
| --- | --- | --- |
| first-room vertex buffer | Vulkan resource module | device/resource lifetime |
| first-room index buffer | Vulkan resource module | device/resource lifetime |
| staging buffer | upload helper/resource module | transient upload or reusable staging lifetime |
| upload command buffer | upload helper or command module | upload lifetime |
| upload fence/wait path | upload helper/sync module | upload lifetime |
| texture image | Vulkan resource module | post-first-room growth |
| resource diagnostic name | resource module | resource lifetime |

Rules:

- runtime/projection never store Vulkan handles;
- command recording binds resources only after upload completion;
- staging is not destroyed or reused before upload completion;
- destination resource is not marked drawable until upload completion;
- upload failure is renderer failure, not runtime failure.

## Upload Phases

Phase A: first-room bootstrap.

Goal: upload enough geometry to draw a visible first room.

Allowed:

- manual narrow allocation or VMA if already adopted;
- transient staging buffer per initialization batch;
- graphics queue upload;
- blocking fence wait during initialization;
- vertex/index buffer upload only.

Blocked:

- textures;
- materials;
- descriptors;
- per-frame uploads;
- background streaming;
- transfer queue optimization.

Phase B: VMA-backed growth.

Goal: durable resource allocation and diagnostics.

Required before texture/material growth:

- VMA or documented equivalent;
- named allocations;
- memory budget/stat diagnostics;
- reusable or batched staging policy;
- no steady-state per-frame allocation churn.

Phase C: image/texture upload.

Goal: upload texture images validation-clean.

Required:

- image layout transition policy;
- texture format policy;
- sampler/descriptor policy;
- fallback texture diagnostics;
- mip policy, even if "no mipmaps yet".

## First-Room Buffer Upload

Required first resources:

```text
buffer.first_room.vertices
buffer.first_room.indices optional
buffer.staging.upload.<sequence>
```

Vertex buffer rules:

- usage includes transfer destination and vertex buffer;
- memory is device-local when possible;
- vertex stride and count are diagnosed;
- buffer survives swapchain recreation;
- buffer is destroyed before allocator/device shutdown.

Index buffer rules:

- usage includes transfer destination and index buffer;
- index type is diagnosed;
- index count is diagnosed;
- index buffer may be omitted only if first-room draw uses non-indexed geometry.

## Staging Buffer Policy

First implementation:

```text
staging_policy=transient_initialization_batch
staging_memory=host_visible
staging_use=transfer_src
```

Rules:

- staging buffer is host-visible;
- staging buffer is used as transfer source;
- staging allocation size must be at least upload byte count;
- mapped range and flush behavior must be correct for memory coherency;
- staging is destroyed only after upload completion;
- transient staging is allowed during initialization but not per frame in steady state.

Growth policy:

```text
staging_policy=reusable_or_batched
```

The growth policy should avoid repeated allocation churn and should diagnose high-water byte usage.

## Mapping And Coherency

Rules:

- map only host-visible memory;
- copy exactly the intended byte range;
- if memory is not host-coherent, flush mapped range before GPU copy reads it;
- if CPU reads GPU-written staging later, invalidate mapped range first;
- unmap policy must be explicit: unmap after copy for transient staging, or persistent mapping for reusable staging if chosen;
- never expose mapped GPU memory to runtime/projection.

Diagnostics:

```text
staging_host_visible=true|false
staging_host_coherent=true|false|unavailable
mapped_bytes=
flush_required=true|false
flush_performed=true|false
```

## Upload Command Policy

First implementation:

```text
upload_queue=graphics
upload_command_buffer=one_time_primary
upload_submit_policy=blocking_initialization
```

Rules:

- graphics queue upload is acceptable for first-room proof;
- upload command buffer is distinct from normal frame command buffers or clearly isolated;
- command buffer uses one-time submit usage when appropriate;
- copy command records buffer-to-buffer for vertex/index resources;
- command buffer is not reset/destroyed until upload work completes;
- upload command pool ownership must be explicit.

Transfer queue use is deferred until queue-family ownership transfer policy exists.

## Buffer Copy Rules

For each buffer upload:

1. Validate source byte span is nonempty.
2. Validate destination buffer size is sufficient.
3. Copy CPU bytes to staging.
4. Flush if needed.
5. Record `staging -> device-local` copy.
6. Submit upload.
7. Wait for completion.
8. Destroy or recycle staging.
9. Mark destination ready.

Rules:

- destination buffer usage includes transfer destination;
- staging buffer usage includes transfer source;
- copy ranges are diagnosed in verbose mode;
- destination resource cannot be bound for drawing before ready.

## Image Upload Growth

Texture upload is blocked until VMA/resource diagnostics are green.

When enabled, image upload must define:

- image format;
- image extent;
- mip levels;
- array layers;
- tiling;
- usage flags;
- initial layout;
- staging row pitch or tightly packed policy;
- buffer-to-image copy regions;
- layout transitions: undefined -> transfer destination -> shader read;
- sampler/descriptor binding policy;
- fallback texture bytes.

First texture recommendation:

```text
format=VK_FORMAT_R8G8B8A8_SRGB
mips=1
filter=linear
address_mode=repeat for real textures
fallback_address_mode=clamp_to_edge
```

Texture upload does not enter first-room baseline unless a later packet explicitly opens that scope.

## Queue Policy

First-room upload queue:

```text
upload_queue=graphics
```

Rationale:

- graphics queue already exists;
- avoids queue-family ownership transfers;
- simpler validation path for first-room proof.

Deferred transfer queue gate:

- transfer queue selected and diagnosed;
- queue-family ownership transfer helper exists;
- sync validation remains clean;
- upload volume justifies complexity;
- fallback to graphics queue is diagnosed.

Timeline semaphore gate:

- binary WSI path passes;
- upload batching needs internal GPU scheduling;
- timeline support queried and diagnosed;
- sync validation remains clean.

## Upload Wait Policy

First implementation:

```text
upload_wait_policy=fence_wait
upload_wait_stage=initialization
```

Allowed bootstrap fallback:

```text
upload_wait_policy=device_wait_idle
```

Rules:

- fence wait is preferred over device idle;
- device idle during initialization is allowed only if diagnosed;
- device idle per upload in steady-state is not acceptable;
- timeout policy should match [sync_contract.md](sync_contract.md);
- upload wait failure fails renderer initialization or resource upload.

Diagnostics:

```text
upload_wait_policy=fence_wait|device_wait_idle
upload_wait_count=
upload_wait_timeout_ns=
upload_wait_result=
```

## Resource Ready State

Each upload destination should have an internal ready state:

```text
created
uploading
ready
failed
destroyed
```

Rules:

- command recording may bind only `ready` resources;
- failed resources produce diagnostics;
- missing first-room required buffer fails strict smoke;
- optional future texture may use fallback texture if diagnosed;
- ready state is renderer-private.

## Per-Frame Allocation Policy

First-room steady-state rule:

```text
per_frame_allocation_count=0
upload_bytes_this_frame=0 after initialization
```

Allowed per-frame work:

- bind existing buffers;
- push constants;
- use existing descriptors later;
- update bounded persistent/ring buffers only after descriptor policy exists.

Forbidden after initialization:

- creating/destroying staging buffers every frame;
- creating/destroying vertex/index buffers every frame;
- allocating memory every frame;
- uploading static first-room geometry every frame.

Any exception must be explicitly diagnosed and justified in a file plan.

## VMA Boundary

Before texture/material growth:

- pin the VMA version or documented local allocator policy;
- allocator lifetime is defined;
- every allocation has a stable debug name;
- memory budget/stat diagnostics exist;
- staging and destination allocations use the same naming policy;
- defragmentation is not required for first growth but diagnostics should not block it later.

Manual bootstrap allocation may exist only for first-room buffers if narrow and clearly replaceable.

## Diagnostics Receipt Fields

Upload diagnostics should include:

```text
memory_allocator=none|manual_bootstrap|vma
vma_version=
upload_queue=graphics|transfer
upload_command_buffer_policy=one_time_primary|reusable
staging_policy=transient_initialization_batch|reusable|batched
staging_buffer_count=
staging_bytes_allocated=
staging_host_visible=true|false
staging_host_coherent=true|false|unavailable
mapped_bytes=
flush_required=true|false
flush_performed=true|false
upload_bytes_this_frame=
upload_bytes_total=
upload_batch_count=
upload_wait_policy=fence_wait|device_wait_idle|none
upload_wait_count=
upload_wait_timeout_ns=
upload_wait_result=
per_frame_allocation_count=
vertex_buffer_count=
index_buffer_count=
texture_upload_enabled=true|false
resource_ready_count=
resource_failed_count=
reason=
```

Use `unavailable` only when a field truly does not apply.

## Failure Reason Codes

Recommended reason codes:

```text
allocation_failed
staging_buffer_create_failed
staging_memory_map_failed
staging_memory_flush_failed
upload_command_buffer_create_failed
upload_command_record_failed
upload_submit_failed
upload_wait_failed
upload_wait_timeout
buffer_copy_failed
image_copy_failed
layout_transition_failed
resource_not_ready
per_frame_allocation_detected
vma_required_missing
texture_upload_blocked
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Validation Expectations

Validation blockers:

- buffer usage missing transfer source/destination flags;
- copying beyond buffer bounds;
- drawing from buffer before upload completion;
- destroying staging before copy completes;
- freeing destination resource while in use;
- missing image layout transitions for image upload;
- wrong queue-family ownership when transfer queue is introduced;
- per-frame allocation churn hidden from diagnostics.

Strict smoke fails on validation or sync validation errors.

## Platform Notes

macOS/MoltenVK:

- upload behavior should be standard Vulkan through MoltenVK;
- keep first path simple: graphics queue, blocking initialization upload;
- print memory allocator and upload wait diagnostics because Metal translation can obscure mistakes.

Linux:

- native Vulkan lane should validate upload path on hardware when possible;
- optional software Vulkan can help CI but cannot replace hardware shipping proof;
- memory type availability may differ and must be diagnosed through allocator/resource fields.

Windows:

- native Vulkan lane should validate upload and destruction on driver-backed hardware;
- package smoke should distinguish missing Vulkan runtime from upload/resource failures;
- validation logs should capture upload hazards.

## Tests

Future tests:

```text
tests/unit/render_memory_policy_tests.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Unit expectations:

- upload policy reports no steady-state per-frame allocation;
- resource names follow stable format;
- texture upload remains blocked until VMA/diagnostics gate;
- runtime/projection types do not contain Vulkan/VMA handles.

Smoke expectations:

- staging buffer creates/maps/copies/destroys validation-clean;
- vertex buffer upload succeeds;
- optional index buffer upload succeeds when used;
- first-room draw binds only ready resources;
- upload diagnostics receipt prints byte counts and wait policy;
- steady-state first-room frames report zero upload bytes/per-frame allocations.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'render_memory|vulkan_memory|vulkan_first_room|vulkan_sync'
```

## Acceptance Criteria

This upload contract is ready for file plans when:

- first-room buffer upload sequence is explicit;
- staging ownership/lifetime is defined;
- mapping/coherency rules are defined;
- upload command buffer ownership is defined;
- graphics queue upload baseline is defined;
- transfer queue/timeline gates are deferred explicitly;
- upload wait/fence policy is defined;
- resource ready state is defined;
- no steady-state per-frame allocation rule is defined;
- VMA boundary before texture/material growth is defined;
- diagnostics and failure reason codes are defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- upload failures cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact VMA version pin;
- exact manual bootstrap allocator wrapper if used before VMA;
- exact upload helper C++ type shape;
- exact command pool for upload commands;
- exact fence timeout constants reused from sync contract;
- exact buffer usage flag helpers;
- exact memory property selection helper;
- exact image upload helper after texture scope opens;
- exact fallback texture bytes;
- exact Linux and Windows upload smoke commands.
