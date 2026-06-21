# Vulkan Resource Model

This document defines GPU memory, buffers, images, descriptors, materials, and resource diagnostics for the Vulkan backend.

The resource model has two lanes:

1. First-room lane: enough GPU resources to draw a visible room with vertex colors, depth, and one matrix push constant.
2. Growth lane: VMA-backed resources, descriptors, textures, materials, asset upload, and diagnostics.

The first-room lane may start before VMA if the implementation needs a tiny bootstrap buffer path. The growth lane must adopt VMA or a documented equivalent before texture/material work begins.

## Decision Status

Decision: VMA adoption and acquisition

Status: `Deferred` until Phase 8

Recommended default: adopt Vulkan Memory Allocator for real buffer/image expansion.

Acquisition recommendation: vendor a pinned VMA release snapshot when Phase 8 begins unless a project dependency policy clearly replaces it.

Primary sources:

- VMA overview: https://gpuopen.com/vulkan-memory-allocator/
- VMA reference docs: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/
- Vulkan Guide memory topics: https://docs.vulkan.org/guide/latest/index.html
- Vulkan Specification/Registry: https://registry.khronos.org/vulkan/

## Local File Surface

Likely future paths:

```text
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
cmake/iggy3d_vulkan_deps.cmake
third_party/vma/ or external/vma/
tests/unit/render_memory_policy_tests.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

This document does not implement those files.

## Ownership

`BuffersImagesMemory` owns:

- buffer creation;
- image creation;
- allocation handles;
- staging resources;
- upload operations;
- depth resources;
- texture image resources after the first room;
- allocation names;
- memory diagnostics.

`PipelinesShaders` owns:

- descriptor set layouts;
- pipeline layouts;
- push constant ranges;
- shader-visible resource interface.

`VulkanBackend` owns:

- module lifetime orchestration;
- choosing when resources are created/destroyed;
- swapchain recreation coordination;
- renderer shutdown ordering.

Content/runtime/projection own:

- asset identity;
- gameplay meaning;
- scene item projection;
- no GPU handles;
- no allocation truth;
- no descriptor/pipeline state.

## Dependency Rules

Allowed VMA/Vulkan resource references:

- `src/render/vulkan/**`;
- Vulkan-specific smoke tests;
- Vulkan dependency CMake helpers.

Forbidden VMA/Vulkan resource references:

- `src/runtime/**`;
- `src/content/**`;
- `src/projection/**`;
- `src/runtime/save/**`;
- headless runtime apps;
- replay tools.

Firewall scan:

```sh
rg -n 'Vma|vma|VkBuffer|VkImage|VkDeviceMemory|VkDescriptor|VkSampler|VK_' src/runtime src/content src/projection src/runtime/save apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result: no production runtime/content/projection/save ownership leak.

## Resource Phases

### Phase A: First-Room Bootstrap

Goal: draw a first 3D room with the smallest resource surface that still respects ownership.

Allowed resources:

- vertex buffer;
- index buffer if useful;
- depth image;
- command-upload staging path;
- one first-room mesh/proxy;
- no textures;
- no materials beyond fallback vertex color;
- no descriptor sets required if push constants carry `clipFromModel`.

VMA requirement:

- optional in this phase;
- if VMA is not yet adopted, manual allocation must stay narrow and must be replaced or wrapped before texture/material work.

Exit criteria:

- first-room geometry is visible;
- depth testing works;
- steady-state render does not allocate per frame;
- diagnostics name every allocation/resource;
- no resource ownership leaks outside `src/render/vulkan/**`.

### Phase B: VMA Resource Baseline

Goal: make GPU allocation durable enough for growth.

Required resources:

- VMA allocator;
- named buffer allocations;
- named image allocations;
- staging upload helpers;
- memory budget/stat diagnostics;
- depth image allocation through resource model.

Exit criteria:

- diagnostics print `memory_allocator=vma`;
- allocation names appear in verbose diagnostics;
- memory smoke creates and destroys vertex/index/depth resources validation-clean;
- steady-state render reports zero per-frame allocation churn.

### Phase C: Descriptor/Material Growth

Goal: introduce descriptors, textures, samplers, materials, and asset-backed GPU residency.

Blocked until:

- Phase B is green;
- allocation naming exists;
- memory diagnostics exist;
- shader/pipeline descriptor interface is documented.

Exit criteria:

- descriptor set layout is named and diagnosed;
- texture upload path is validation-clean;
- missing texture/material fallback is visible and diagnosed;
- runtime/content still own asset identity only.

## VMA Adoption Contract

Version pin:

- pin a specific VMA release/tag when Phase 8 begins;
- record version in `docs/vulkan/decisions.md` or a dependency manifest;
- diagnostics should print the VMA version if accessible.

Acquisition:

- preferred: vendored pinned snapshot under `third_party/vma/` or `external/vma/`;
- acceptable: CMake `FetchContent` with pinned tag/hash only if network dependency is opt-in;
- acceptable override: system package for developers/CI;
- rejected: unpinned download at configure time.

Allocator owner:

- `BuffersImagesMemory` or a private `VulkanMemoryAllocator` helper owns allocator creation/destruction;
- allocator is created after instance, physical device, logical device, and Vulkan function loading are ready;
- allocator is destroyed before device destruction;
- allocator outlives every VMA-backed buffer/image allocation.

Naming:

- every allocation must receive a stable debug name;
- allocation names should include resource class and semantic use;
- example names:

```text
buffer.first_room.vertices
buffer.first_room.indices
image.depth.swapchain_extent
image.texture.<asset_id>
buffer.staging.upload.<sequence>
```

Diagnostics:

- allocation count;
- buffer allocation count;
- image allocation count;
- total allocated bytes if available;
- budget/usage if available;
- per-frame allocation count;
- verbose named allocation list when requested.

## Buffer Classes

### Vertex Buffer

Owns:

- GPU buffer for vertex data;
- allocation;
- vertex count/stride diagnostics;
- optional asset/proxy id used only for diagnostics.

Rules:

- first room uses `FirstRoomVertex` with position and color;
- buffer is created during resource setup or asset upload;
- buffer is not recreated every frame;
- renderer may map projected scene items to existing mesh buffers;
- runtime/projection never store `VkBuffer` or VMA handles.

### Index Buffer

Owns:

- GPU buffer for indices;
- allocation;
- index type;
- index count diagnostics.

Rules:

- use 16-bit indices if sufficient for first room;
- use 32-bit indices when geometry requires it;
- index type must be recorded in diagnostics.

### Staging Buffer

Owns:

- host-visible upload buffer;
- allocation;
- mapped range policy;
- upload byte counters.

Rules:

- staging is used for device-local vertex/index/image uploads;
- staging resources may be transient per upload batch, not per frame in steady state;
- upload completion must be synchronized before staging memory is reused/destroyed;
- per-frame staging allocations are forbidden after first-room initialization unless explicitly measured and diagnosed.

### Uniform Buffer

Status: deferred for first room.

Reason:

- first room uses a 64-byte push constant for `clipFromModel`;
- uniform buffers become useful once there are per-frame camera/material/light descriptors.

Rules when introduced:

- use per-frame uniform buffers or ring buffer;
- avoid CPU/GPU overwrite hazards;
- name every per-frame buffer;
- include mapped range and flush/invalidate rules if memory is not coherent.

### Per-Frame Scratch Buffers

Status: deferred.

Rules when introduced:

- allocation count must be fixed at initialization or resize;
- no unbounded per-frame allocation;
- diagnostics print max bytes used per frame;
- resize/recreate path owns capacity changes.

## Image Classes

### Depth Image

Owns:

- depth `VkImage`;
- image view;
- allocation;
- current extent;
- selected format;
- layout transitions;
- diagnostics.

Format fallback order:

```text
VK_FORMAT_D32_SFLOAT
VK_FORMAT_D24_UNORM_S8_UINT
VK_FORMAT_D32_SFLOAT_S8_UINT
```

Rules:

- selected format must support depth-stencil attachment usage;
- depth image extent follows swapchain extent;
- depth image is recreated with swapchain extent;
- depth resource is destroyed before allocator/device shutdown;
- depth format is printed in renderer smoke receipt.

Open detail:

- if stencil is unused, prefer pure depth format;
- exact fallback order can change if platform proof shows a better cross-platform set.

### Color Images

Status: swapchain-owned for first room.

Rules:

- swapchain owns presentable color images;
- resource model may own offscreen color images later;
- no offscreen color target until a feature requires it.

### Texture Images

Status: blocked until VMA baseline is green.

Rules when introduced:

- content/runtime own asset identity;
- renderer owns image allocation, image view, sampler binding, and residency;
- texture upload uses staging buffers;
- texture format policy must be explicit;
- missing texture uses renderer fallback texture and diagnostic;
- texture handles do not enter runtime/projection/save.

Initial texture format recommendation:

```text
VK_FORMAT_R8G8B8A8_SRGB for color/albedo
VK_FORMAT_R8G8B8A8_UNORM for non-color data if introduced later
```

Mipmap policy:

- no mipmaps required for first textured proof;
- when enabled, mip generation ownership and format feature checks must be documented.

### Samplers

Status: blocked until texture images.

Initial recommendation when introduced:

```text
filter=linear
address_mode=repeat for materials
address_mode=clamp_to_edge for fallback/debug textures
anisotropy=off until feature proof
```

Sampler diagnostics should print filter/address/anisotropy policy.

## Descriptor Model

First-room baseline:

- no descriptor sets required;
- one push constant matrix;
- vertex/index/depth only.

First descriptor phase:

- one global/frame descriptor set for camera or frame data if push constants are insufficient;
- one material/texture descriptor set only after texture/material growth begins;
- descriptor pool owned by Vulkan resource/pipeline layer;
- descriptor set layout owned by `PipelinesShaders`;
- descriptor allocation/update policy documented before use.

Proposed descriptor set families:

```text
set 0: FrameData
set 1: MaterialData
```

Do not introduce both sets until the shader/resource need exists.

Descriptor rules:

- descriptor layout names must be stable;
- descriptor pool sizes must be explicit;
- per-frame descriptor updates must be bounded;
- descriptor writes cannot happen from runtime/projection;
- missing descriptor resource falls back in renderer or fails renderer validation, never mutates runtime.

Push constant rule:

- first room uses 64-byte `clipFromModel`;
- keep push constant usage under guaranteed minimum unless device diagnostics prove larger safe use;
- large per-frame/per-material data belongs in buffers/descriptors, not push constants.

## Material Model

Status: deferred until after first room.

Runtime/content responsibilities:

- define asset ids;
- define accepted package references;
- define gameplay meaning, if any.

Projection responsibilities:

- emit inert asset/material reference ids when available;
- keep stable item ordering.

Renderer responsibilities:

- map asset/material ids to GPU resources;
- provide fallback material;
- diagnose missing/unloaded material;
- bind material resources for draw.

Initial fallback material:

```text
name=fallback.vertex_color
color_source=vertex_color
texture=none
lighting=none
```

First textured material recommendation:

```text
name=unlit_textured
base_color_texture=<asset_id>
fallback_color=magenta_or_named_debug_color
lighting=none
```

Lighting is not part of the first resource model. Add it only after unlit asset/material flow is proven.

## Upload And Synchronization

Upload owner: `BuffersImagesMemory`.

Rules:

- upload command buffer ownership must be explicit;
- transfer queue use is optional, graphics queue upload is acceptable for first room;
- upload completion must be synchronized before draw uses uploaded data;
- staging resources cannot be destroyed before upload completion;
- upload byte count is recorded in diagnostics;
- upload failures do not mutate runtime state.

First-room acceptable path:

- create staging buffer;
- copy vertex/index data to staging;
- copy staging to device-local buffer;
- wait for upload completion during initialization;
- destroy transient staging after completion.

Growth path:

- batch uploads;
- reuse staging buffers;
- avoid device wait-idle for every upload;
- add transfer timeline/fence policy only after first-room proof.

## Lifetime And Destruction Order

Creation order:

1. Vulkan instance/device/surface.
2. Swapchain.
3. VMA allocator or bootstrap allocation helpers.
4. Depth resources.
5. First-room buffers.
6. Shader modules/pipeline layout/pipeline.
7. Descriptor pools/sets when introduced.

Destruction order:

1. Wait for device idle or safe frame-resource completion.
2. Descriptor resources.
3. Pipeline and pipeline layout.
4. Shader modules if still alive.
5. First-room buffers.
6. Texture/material resources.
7. Depth resources.
8. VMA allocator/bootstrap allocation helpers.
9. Swapchain.
10. Device/surface/instance.

Swapchain recreation:

- depth resources are recreated with swapchain extent;
- pipelines may be recreated if color/depth format changes;
- vertex/index buffers survive swapchain recreation;
- material/texture resources survive swapchain recreation unless format/descriptor policy says otherwise.

## Resource Diagnostics Receipt

Minimum fields:

```text
memory_allocator=none|manual_bootstrap|vma
vma_version=
allocation_count=
buffer_allocation_count=
image_allocation_count=
allocated_bytes=
budget_bytes=
used_budget_bytes=
per_frame_allocation_count=
upload_bytes_this_frame=
upload_bytes_total=
depth_format=
depth_extent=
vertex_buffer_count=
index_buffer_count=
texture_count=
sampler_count=
descriptor_set_layout_count=
descriptor_pool_count=
material_count=
missing_resource_count=
```

Verbose named resources:

```text
resource.name=buffer.first_room.vertices
resource.type=buffer
resource.bytes=
resource.owner=BuffersImagesMemory
resource.source=first_room
```

Rules:

- receipt is renderer diagnostic data;
- receipt does not affect replay hash;
- missing budget fields should print `unavailable`, not disappear;
- strict smoke tests may parse required fields.

## Failure Policy

Resource failures:

- allocation failure;
- upload failure;
- unsupported depth format;
- descriptor pool exhaustion;
- missing texture/material;
- image layout transition validation failure;
- per-frame allocation detected in steady state.

Rules:

- all failures produce stable renderer diagnostics;
- strict Vulkan smoke fails on unexpected resource failure;
- missing optional texture/material may use fallback if diagnosed;
- unsupported depth format is fatal for first-room Vulkan rendering;
- resource failure cannot mutate runtime truth.

## Tests

Unit tests:

```text
tests/unit/render_memory_policy_tests.cpp
```

Required coverage:

- resource name formatting;
- no runtime/content/projection/save VMA/Vulkan leaks;
- depth format fallback order is stable;
- per-frame allocation policy is represented;
- diagnostics field names are stable.

Smoke tests:

```text
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Required coverage:

- allocator/bootstrap path initializes;
- vertex/index/depth resources create and destroy;
- upload path succeeds;
- first room draws without per-frame allocation churn;
- diagnostics receipt prints memory/resource fields;
- validation is clean.

Commands:

```sh
ctest --test-dir build --output-on-failure -R 'render_memory'
ctest --test-dir build --output-on-failure -R 'vulkan_memory|vulkan_first_room'
```

Strict Vulkan command:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -R 'render_memory|vulkan_memory|vulkan_first_room'
```

## Acceptance Criteria

The resource model is ready for file plans when:

- first-room bootstrap resource scope is understood;
- VMA adoption gate is documented;
- buffer classes are named;
- depth image format fallback is named;
- texture/material work is blocked until memory diagnostics exist;
- descriptor work is deferred but shaped;
- upload synchronization rules are explicit;
- resource lifetime/destruction order is explicit;
- diagnostics fields are named;
- tests and smoke gates are named.

## Open Detail Items

These belong in future file plans or later docs:

- exact VMA version pin;
- exact dependency folder name;
- exact CMake target for VMA;
- exact `FirstRoomVertex` owner;
- exact first-room geometry source;
- exact upload command helper API;
- exact descriptor pool sizing once descriptors are introduced;
- exact fallback texture bytes;
- exact verbose diagnostics format.
