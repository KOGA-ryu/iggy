# Vulkan Render Assets And Materials

This document defines how real renderable assets and materials enter the Vulkan renderer after the first-room proof.

Render assets are renderer-owned GPU residency records derived from backend-neutral content/runtime identity. They are not package validation, gameplay truth, save truth, replay truth, or asset authoring policy.

## Purpose

Define the growth path from proxy geometry to real renderer assets:

```text
content/runtime owns stable asset and material identity
projection emits backend-neutral draw references
renderer resolves draw references to GPU records
renderer owns buffers, images, image views, samplers, descriptors, and pipeline choice
missing or failed assets use diagnosed renderer fallbacks
runtime state is never mutated by renderer residency results
```

This document connects [resource_model.md](resource_model.md), [gpu_resource_upload.md](gpu_resource_upload.md), [descriptor_policy.md](descriptor_policy.md), [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md), [first_room_render_contract.md](first_room_render_contract.md), [depth_and_coordinates.md](depth_and_coordinates.md), and [boundaries.md](boundaries.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for buffer, image, image view, sampler, descriptor, pipeline-layout, and image-layout rules.
- VMA docs for allocation strategy, naming, memory budgets, pools, mapping, statistics, and debugging.
- Vulkan Guide for memory, descriptors, synchronization, validation, and portability explanations.
- Khronos Vulkan Tutorial for first texture, vertex buffer, index buffer, descriptor, and sampler implementation shape.
- Vulkan Samples for production-ish resource binding, texture, descriptor, and material examples after the first-room proof.
- Project runtime/content/projection docs for backend-neutral identity only.

Rules:

- Vulkan docs define GPU behavior.
- Project runtime/content docs define semantic identity and validation ownership.
- MoltenVK and platform notes define portability constraints, not cross-platform Vulkan truth.

## Scope

In scope:

- backend-neutral render asset identity;
- mesh GPU records;
- texture GPU records;
- sampler ownership;
- material GPU records;
- material-to-pipeline selection;
- descriptor binding for materials;
- fallback mesh, texture, and material policy;
- resource ready states;
- cache lookup behavior;
- resize/lifetime interaction;
- diagnostics, failure reasons, and smoke gates.

Out of scope:

- asset authoring tools;
- package validation;
- content import format design;
- save/load truth;
- replay determinism truth;
- skeletal animation;
- streaming residency;
- PBR lighting;
- shadows;
- bindless descriptor indexing;
- editor hot reload;
- renderer-driven runtime mutation.

## Local File Surface

Likely future files:

```text
src/render/RenderMesh.hpp
src/render/RenderMaterial.hpp
src/render/FrameInput.hpp
src/render/RenderDiagnostics.hpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanRenderAssets.hpp
src/render/vulkan/VulkanRenderAssets.cpp
tests/unit/render_asset_policy_tests.cpp
tests/unit/render_material_policy_tests.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_material_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`VulkanRenderAssets` should exist only if asset/material ownership becomes too large for `BuffersImagesMemory`. Until then, a builder may keep records near the upload/resource module, but the ownership rules in this document still apply.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| asset ids | content/runtime | validated identity, no Vulkan handles |
| material ids | content/runtime | semantic material identity, no binding numbers |
| draw references | projection | backend-neutral references only |
| mesh GPU records | renderer | vertex/index buffers and draw counts |
| texture GPU records | renderer | images, image views, layouts, allocation names |
| samplers | renderer | immutable or cached Vulkan sampler objects |
| material GPU records | renderer | fallback flags, descriptor binding, pipeline variant |
| descriptor sets | renderer | see descriptor policy |
| pipeline variants | renderer | see pipeline variant policy |
| fallback resources | renderer | visible diagnostics, not content truth |

Rules:

- runtime/content/projection must not include Vulkan headers for asset/material identity;
- runtime/content/projection must not store `Vk*` handles or `VK_*` constants;
- renderer cache misses must not mutate runtime state;
- renderer fallback use must not become save/load truth;
- renderer asset readiness must not affect deterministic state hashes or replay results;
- picking/input that depends on rendered assets must route through runtime commands later.

## Entry Gates

Render asset/material work may begin only after:

- headless runtime acceptance is green;
- renderer boundary and null renderer are established;
- first-room proxy geometry renders or a file plan explicitly justifies moving asset work earlier;
- upload diagnostics from [gpu_resource_upload.md](gpu_resource_upload.md) exist;
- descriptor policy has an accepted first material set layout;
- pipeline variant policy has an accepted unlit material pipeline name;
- fallback mesh, texture, and material behavior are documented;
- validation and sync validation are enabled in development smoke runs;
- resource lifetime rules cover asset destruction before allocator/device shutdown.

Reviewer should reject asset/material implementation if it arrives before these gates without an explicit dependency reason.

## First-Room Relationship

The first visible Vulkan room should not need real asset or material loading.

First-room baseline:

```text
geometry_source=projection_proxy_room
material_model=fallback.vertex_color
texture_count=0
descriptor_sets=0
asset_cache=disabled
material_cache=disabled
```

Asset/material growth begins after that proof:

```text
geometry_source=backend_neutral_mesh_ref
material_model=unlit_textured
texture_count>=1
descriptor_sets>=1
asset_cache=enabled
material_cache=enabled
```

The first room proves renderer wiring. Asset/material work proves residency and binding. Do not merge the two scopes unless a packet explicitly changes the acceptance target.

## Backend-Neutral Identity

Asset and material identity passed into rendering must be inert data.

Allowed examples:

```text
asset_id="room_wall_basic"
mesh_id="crate_a"
material_id="stone_floor_unlit"
debug_name="first_room_floor"
```

Forbidden examples:

```text
VkBuffer
VkImageView
VkSampler
VkDescriptorSet
VK_FORMAT_R8G8B8A8_SRGB
binding=1
set=1
pipeline.material.unlit_textured.opaque.depth.backface
```

Rules:

- content/runtime may validate that an asset id exists;
- projection may reference a validated backend-neutral asset/material id;
- renderer maps that id to a GPU record;
- renderer may print both backend-neutral id and GPU diagnostic name;
- renderer must not require runtime code to know descriptor sets, bindings, pipeline names, or Vulkan formats.

## GPU Record Model

Conceptual renderer-private records:

```text
MeshGpuRecord
TextureGpuRecord
SamplerGpuRecord
MaterialGpuRecord
```

`MeshGpuRecord` should carry:

- backend-neutral mesh id for diagnostics;
- renderer diagnostic name;
- vertex buffer handle;
- index buffer handle when indexed;
- vertex count;
- index count;
- vertex format name;
- allocation names;
- ready state;
- fallback flag.

`TextureGpuRecord` should carry:

- backend-neutral texture id for diagnostics;
- renderer diagnostic name;
- image handle;
- image view handle;
- image format;
- extent;
- mip count;
- current steady-state layout;
- allocation name;
- ready state;
- fallback flag.

`SamplerGpuRecord` should carry:

- sampler handle;
- filter mode;
- address mode;
- anisotropy flag;
- diagnostic name.

`MaterialGpuRecord` should carry:

- backend-neutral material id for diagnostics;
- material model name;
- fallback color;
- texture reference if any;
- sampler reference if any;
- descriptor set reference if descriptors are enabled;
- pipeline variant name;
- ready state;
- fallback flag.

These names describe ownership. They do not require exact class names.

## Ready States

Every renderer asset/material lookup should resolve to one of these states:

```text
missing
requested
loading
ready
fallback
failed
evicted future
```

Rules:

- command recording may bind only `ready` or `fallback` records;
- `missing` becomes fallback or render-skip with diagnostics;
- `failed` becomes fallback or render-skip with diagnostics;
- `loading` may use fallback for the current frame;
- `evicted` is future streaming scope;
- no state transition mutates runtime truth.

## Mesh Policy

First real mesh policy:

```text
mesh_payload=approved_cpu_mesh_boundary
vertex_buffer=device_local
index_buffer=device_local if indexed
upload=staging_buffer
draw=indexed preferred when index data exists
```

Rules:

- renderer consumes an approved CPU mesh payload or backend-neutral mesh view;
- renderer does not parse arbitrary package files unless a later file plan assigns that ownership;
- winding, handedness, and coordinate conversion must be diagnosed against [depth_and_coordinates.md](depth_and_coordinates.md);
- renderer must name vertex/index allocations;
- fallback mesh may be a simple cube/proxy shape, but it must be visibly diagnosed;
- skeletal mesh and animation are deferred.

## Texture Policy

First texture policy:

```text
format=VK_FORMAT_R8G8B8A8_SRGB unless rejected by device capability
mip_count=1
usage=transfer_dst|sampled
steady_state_layout=shader_read_only
descriptor_type=combined_image_sampler
fallback_texture=required
```

Rules:

- texture upload follows [gpu_resource_upload.md](gpu_resource_upload.md);
- image layout transitions must be explicit and validation-clean;
- texture records own image/image view/allocation state;
- runtime/content do not know Vulkan texture formats;
- first texture smoke should use one small known texture before broad asset loading;
- mip generation, compression, arrays, cubemaps, and streaming are later gates.

If `VK_FORMAT_R8G8B8A8_SRGB` is unavailable or inappropriate on a target, the renderer must print the selected fallback format and reason.

## Sampler Policy

Initial sampler policy:

```text
filter=linear
address_mode=repeat
anisotropy=disabled until feature gate
fallback_sampler=linear_clamp or nearest_clamp
```

Rules:

- samplers are renderer-owned;
- sampler count should be small and diagnosed;
- identical sampler descriptions may share a sampler object;
- enabling anisotropy requires device feature selection and diagnostics;
- sampler policy does not live in runtime gameplay code.

## Material Policy

Initial material models:

```text
fallback.vertex_color
material.unlit_textured
```

`fallback.vertex_color`:

- no texture;
- no material descriptor required;
- used by first-room baseline and missing material fallback.

`material.unlit_textured`:

- base color texture;
- sampler;
- optional fallback color;
- material descriptor set from `set1_material`;
- pipeline variant `pipeline.material.unlit_textured.opaque.depth.backface`.

Rules:

- content/runtime may own material id and semantic fields;
- renderer owns the GPU binding form;
- missing material id must not crash normal development smoke runs;
- missing material must produce a diagnostic reason and visible fallback;
- lighting/PBR material expansion is blocked until unlit textured material is stable.

## Descriptor And Pipeline Coupling

Material descriptors must follow [descriptor_policy.md](descriptor_policy.md).

Expected first material descriptor:

```text
descriptor_layout.material_unlit_textured.v1
set1_material.binding0=combined_image_sampler_base_color
```

Expected first material pipeline:

```text
pipeline.material.unlit_textured.opaque.depth.backface
layout=frame_material or material_only if frame descriptors are not used
```

Rules:

- descriptor layout name must match pipeline layout compatibility;
- command recording must bind descriptor sets compatible with the selected pipeline;
- material pipeline creation must happen outside normal per-frame recording;
- missing descriptor binding is a renderer failure, not runtime failure;
- descriptor set/binding numbers must not appear in runtime/content/projection.

## Fallback Resources

Required fallback resources:

```text
fallback_mesh
fallback_texture
fallback_material
```

Recommended first definitions:

- `fallback_mesh`: simple cube/proxy mesh with known winding;
- `fallback_texture`: high-contrast checker or solid diagnostic color;
- `fallback_material`: vertex-color or unlit textured fallback using fallback texture.

Rules:

- fallback use must be visible in diagnostics;
- fallback use must not silently become content truth;
- fallback resources are renderer-owned and created during renderer resource initialization;
- if fallback resources fail to create, renderer startup or smoke must fail;
- fallback resource ids should be stable in diagnostics.

## Residency And Lifetime

Render assets should survive swapchain resize.

Rules:

- swapchain images and image views are resize-owned;
- mesh buffers, texture images, samplers, and material records are device/resource-owned;
- assets must be destroyed before allocator and logical device shutdown;
- descriptor sets referencing textures must be invalidated before destroying referenced image views/samplers;
- no asset GPU resource may be destroyed while an in-flight frame can still use it;
- resize should not reload all static assets unless a device-loss path requires it;
- device loss invalidates the renderer asset cache.

See [lifetime.md](lifetime.md) and [swapchain_contract.md](swapchain_contract.md).

## Cache And Lookup Policy

Renderer cache lookup should be explicit:

```text
draw_item.mesh_id -> MeshGpuRecord
draw_item.material_id -> MaterialGpuRecord
MaterialGpuRecord.texture_id -> TextureGpuRecord
MaterialGpuRecord.sampler_key -> SamplerGpuRecord
MaterialGpuRecord.pipeline_key -> pipeline variant
```

Rules:

- lookup keys are backend-neutral ids or renderer-private keys;
- cache misses resolve to fallback or render-skip with diagnostics;
- cache entries carry ready state;
- cache size should be printed in smoke receipts;
- eviction/reload/hot-swap are deferred until first static asset path is stable.

## Draw Binding Flow

Expected material draw flow:

```text
projection emits draw item with mesh/material ids
renderer resolves mesh record
renderer resolves material record
renderer resolves material texture/sampler/descriptors
renderer selects compatible pipeline variant
command recording binds pipeline
command recording binds vertex/index buffers
command recording binds material descriptors when enabled
command recording pushes per-draw matrices/constants
command recording draws
diagnostics records ready/fallback/missing counts
```

Rules:

- draw item resolution happens inside renderer/backend-owned code;
- command recording must not perform package/content validation;
- command recording should not allocate new persistent assets during normal draw recording;
- missing resources should be accounted once per frame or once per asset to avoid log spam.

## Diagnostics Receipt

Asset/material smoke should print:

```text
render_asset_system=enabled|disabled
mesh_gpu_record_count=
texture_gpu_record_count=
material_gpu_record_count=
sampler_count=
asset_lookup_count=
asset_missing_count=
asset_fallback_count=
material_lookup_count=
material_missing_count=
material_fallback_count=
fallback_mesh_used=true|false
fallback_texture_used=true|false
fallback_material_used=true|false
texture_format=
texture_extent=
texture_layout=
material_model=
material_pipeline=
descriptor_sets_enabled=true|false
descriptor_layouts=
pipeline_variant=
resource_ready_count=
resource_failed_count=
runtime_state_touched=false
reason=
```

`runtime_state_touched=false` is mandatory for renderer smoke that exercises missing assets or fallbacks.

## Failure Reason Codes

Use stable reason codes:

```text
render_asset_scope_blocked
mesh_asset_missing
mesh_payload_invalid
mesh_upload_failed
mesh_winding_invalid
texture_asset_missing
texture_format_unsupported
texture_upload_failed
texture_layout_invalid
sampler_create_failed
material_missing
material_descriptor_missing
material_pipeline_missing
fallback_mesh_missing
fallback_texture_missing
fallback_material_missing
asset_gpu_record_not_ready
descriptor_pipeline_layout_mismatch
renderer_asset_leak_to_runtime
```

Rules:

- reason codes should appear in diagnostics and smoke failure output;
- reason codes should not be localized or rewritten per platform;
- runtime/content failure codes should remain separate from renderer residency failure codes.

## Validation Expectations

Validation should catch:

- descriptor set layout and pipeline layout mismatch;
- descriptor set bound with destroyed image view or sampler;
- sampled image in wrong layout;
- buffer used after destroy;
- memory used without correct allocation/binding;
- upload command using invalid barriers;
- drawing with missing vertex/index buffers;
- destroying resource while frame is in flight.

Renderer tests should also scan for forbidden Vulkan leakage into runtime/content/projection paths.

## Platform Notes

Behavior target is shared across macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan.

Rules:

- material fallback behavior must be identical across platforms;
- renderer diagnostics should print the selected texture format and pipeline variant on all platforms;
- macOS/MoltenVK portability notes may explain constraints but must not redefine the cross-platform asset contract;
- Linux and Windows smoke should run on native Vulkan drivers when available;
- software Vulkan is acceptable only as a clearly labeled development fallback.

## Tests

Expected future tests:

```text
tests/unit/render_asset_policy_tests.cpp
tests/unit/render_material_policy_tests.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_material_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Unit tests should cover:

- backend-neutral id policy;
- forbidden Vulkan type leakage;
- fallback resolution;
- material model selection;
- descriptor/pipeline compatibility names;
- reason code stability.

Smoke tests should cover:

- fallback-only material draw;
- one uploaded texture draw;
- missing texture fallback;
- missing material fallback;
- descriptor set enabled path;
- diagnostics receipt fields.

Suggested leak scan:

```sh
rg -n "vulkan/vulkan.h|Vk[A-Z]|VK_" src/runtime src/content src/projection src/runtime/save
```

Expected result: no production ownership leak.

## Acceptance Criteria

Asset/material renderer work is acceptable only when:

- first-room baseline remains green;
- at least one real texture can be uploaded and sampled;
- at least one unlit textured material can draw;
- missing texture uses diagnosed fallback;
- missing material uses diagnosed fallback;
- descriptor/pipeline layout compatibility is named and validation-clean;
- resize does not destroy static mesh/texture/material residency;
- shutdown destroys asset resources before allocator/device;
- diagnostics receipt includes asset/material counts and fallback counts;
- runtime state hash/replay result is unaffected by renderer fallback behavior;
- Linux and Windows validation plans are present beside macOS/MoltenVK validation.

## Open Detail Items

The next detailed pass should define:

- exact CPU mesh payload boundary accepted by renderer;
- exact public backend-neutral render asset id type;
- exact material id and material model record shape;
- fallback mesh vertices and winding;
- fallback texture byte pattern and dimensions;
- texture format fallback order;
- sampler key structure;
- first material descriptor set allocation count;
- first material pipeline layout name;
- cache capacity and eviction policy;
- Linux smoke command;
- Windows smoke command;
- whether render asset loading is synchronous for the first visual demo.
