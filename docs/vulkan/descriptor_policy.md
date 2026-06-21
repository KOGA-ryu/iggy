# Vulkan Descriptor Policy

This document defines when descriptors enter the renderer, what descriptor layouts are allowed, who owns descriptor pools/sets/updates, and how descriptor concepts stay out of runtime/projection.

Descriptors are renderer-owned GPU binding infrastructure. Runtime/content/projection may produce backend-neutral ids and frame data, but they do not own `VkDescriptorSetLayout`, `VkDescriptorPool`, `VkDescriptorSet`, descriptor writes, sampler handles, image views, uniform buffers, or binding indices.

## Purpose

Define the descriptor growth contract:

```text
first-room baseline uses push constants only
descriptor work is blocked until resource/upload diagnostics exist
frame/camera descriptors may enter only when push constants are insufficient
texture/material descriptors enter after VMA + texture upload
descriptor layouts are named and stable
descriptor pool sizes are explicit
descriptor writes are renderer-owned and bounded
runtime/projection never mutate descriptor state
```

This document narrows descriptor sections from [resource_model.md](resource_model.md), [shader_pipeline.md](shader_pipeline.md), [gpu_resource_upload.md](gpu_resource_upload.md), [command_recording.md](command_recording.md), and [boundaries.md](boundaries.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for descriptor set layout, descriptor pool, descriptor update, binding, and pipeline layout rules.
- Vulkan Guide descriptor topics for explanation.
- Khronos Vulkan Tutorial descriptors/uniform buffer/texture chapters for first implementation shape.
- Vulkan Samples for production-ish descriptor patterns after first-room proof.
- VMA docs for descriptor-backed buffer/image allocation naming and diagnostics.

## Scope

In scope:

- descriptor entry gates;
- first-room no-descriptor baseline;
- descriptor set family names;
- descriptor layout ownership;
- descriptor pool sizing;
- per-frame descriptor strategy;
- camera/frame uniform buffer gate;
- material/texture descriptor gate;
- sampler ownership;
- descriptor update rules;
- fallback texture/material behavior;
- diagnostics and reason codes.

Out of scope:

- first-room push constant matrix baseline;
- texture upload implementation details;
- material authoring format;
- bindless/descriptor indexing;
- update-after-bind;
- streaming residency;
- lighting/shadow descriptors;
- runtime asset/package truth.

## Local File Surface

Likely future files:

```text
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/VulkanTypes.hpp
tests/unit/render_descriptor_policy_tests.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`PipelinesShaders` should own descriptor set layout compatibility with pipeline layout. `BuffersImagesMemory` should own descriptor-backed resources, descriptor pools, descriptor sets, and descriptor writes unless a later file plan creates a dedicated descriptor module.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| descriptor set layouts | pipeline/shader module | pipeline layout compatibility |
| descriptor pools | resource/descriptor module | explicit size policy |
| descriptor sets | resource/descriptor module | allocated from pools |
| descriptor writes | resource/descriptor module | renderer-owned updates |
| uniform buffers | resource module | per-frame or ring-buffer policy |
| texture image views | resource module | after upload gate |
| samplers | resource module | renderer-owned |
| material id mapping | renderer material layer | maps backend-neutral ids to GPU resources |

Rules:

- runtime/projection never hold descriptor handles;
- descriptor binding numbers do not appear in runtime/content/projection/save;
- descriptor update failures are renderer failures;
- missing optional material/texture may use renderer fallback with diagnostics;
- descriptor layout changes require shader/pipeline compatibility review.

## Entry Gates

Descriptors are not part of the first-room baseline.

Descriptor work may begin only after:

- headless runtime gate is green;
- first-room push-constant render path works or a file plan justifies why descriptors are needed first;
- VMA or documented equivalent is adopted for resource growth;
- upload diagnostics exist;
- no steady-state per-frame allocation churn is proven;
- shader interface for descriptor use is documented;
- descriptor diagnostics fields are added to smoke receipt.

Texture/material descriptors additionally require:

- texture upload path validation-clean;
- fallback texture bytes defined;
- sampler policy defined;
- missing material/texture fallback behavior defined.

## First-Room Baseline

First-room baseline:

```text
descriptor_sets=0
pipeline_layout=push_constants_only
push_constant=clipFromModel
texture_count=0
material_model=fallback.vertex_color
```

Rules:

- no descriptors are required to draw the first room;
- camera matrix is pushed as `clipFromModel`;
- vertex color is fallback presentation data;
- descriptor smoke comes after first-room baseline unless a builder packet explicitly changes scope;
- if descriptors appear before first-room proof, reviewer must reject unless the packet explains the dependency.

## Descriptor Set Families

Proposed future set families:

```text
set0_frame
set1_material
set2_object optional_later
```

`set0_frame`:

- camera/frame uniform buffer if push constants become insufficient;
- lighting globals later;
- per-frame data only;
- bounded per-frame update policy.

`set1_material`:

- base color texture/sampler;
- material constants later;
- fallback material binding;
- asset/material id mapping stays renderer-owned.

`set2_object`:

- deferred;
- only if per-object data outgrows push constants or instancing requires it.

Rules:

- set names must be stable in diagnostics;
- set order must match pipeline layout;
- adding a new set family is a shader/pipeline compatibility change.

## Layout Naming And Binding Policy

Every descriptor layout must have a stable name.

Example:

```text
descriptor_layout.frame.v1
descriptor_layout.material_unlit_textured.v1
```

Binding policy:

```text
set0_frame.binding0 = uniform_buffer_frame
set1_material.binding0 = combined_image_sampler_base_color
```

Rules:

- binding numbers are renderer/shader contract only;
- binding names are printed in diagnostics;
- binding type, shader stages, and descriptor count are explicit;
- no runtime or projection type stores binding numbers;
- layout version increments when bindings change incompatibly.

## Descriptor Pool Policy

Descriptor pools must be sized explicitly.

First descriptor smoke policy:

```text
descriptor_pool_strategy=fixed_smoke_pool
max_sets=small_explicit_count
free_individual_sets=false unless needed
```

Rules:

- pool sizes are derived from named descriptor set families;
- pool exhaustion is a renderer failure with diagnostics;
- do not allocate descriptor pools every frame;
- do not hide dynamic growth without counters;
- pool lifetime is device/resource lifetime unless a specific material cache owns a narrower lifetime.

Diagnostics must print pool counts and max sets.

## Per-Frame Descriptor Strategy

If frame/camera uniform buffers are introduced:

Preferred first policy:

```text
per_frame_descriptor_sets=frames_in_flight
per_frame_uniform_buffers=frames_in_flight
update_policy=write_once_per_frame_slot_after_fence
```

Rules:

- update only the current frame slot after its fence is safe;
- do not update descriptor resources while GPU may read them;
- use ring buffer only after alignment/offset policy is documented;
- per-frame updates must be bounded and diagnosed;
- renderer may consume `FrameInput`, but cannot mutate runtime camera truth.

For first-room, push constants remain simpler and preferred.

## Uniform Buffer Gate

Uniform buffers become acceptable when:

- push constant data exceeds the safe first-room size;
- there is shared frame data used by many draws;
- descriptor policy and per-frame resource lifetime are implemented;
- alignment requirements are documented;
- update synchronization is clean under sync validation.

Required before use:

- min uniform buffer offset alignment queried if dynamic offsets are used;
- mapped memory coherency policy documented;
- per-frame buffer count named;
- descriptor binding and shader interface documented.

Uniform buffers must not become runtime state.

## Texture And Sampler Descriptor Gate

Texture descriptors become acceptable only after texture upload is validation-clean.

Required first textured material:

```text
material_pipeline=unlit_textured
texture_format=VK_FORMAT_R8G8B8A8_SRGB
descriptor_type=combined_image_sampler
fallback_texture=defined
sampler_filter=linear
sampler_address_mode=repeat
fallback_sampler_address_mode=clamp_to_edge
```

Rules:

- texture image view and sampler are renderer-owned;
- content/runtime may identify a texture asset id, but not a GPU texture handle;
- missing texture binds fallback texture with diagnostics;
- texture descriptor write happens after image upload and layout transition to shader-read;
- descriptor remains valid while command buffers that reference it may be in flight.

## Material Mapping

Backend-neutral input:

```text
material_id
asset_id
render_item_kind
fallback_color
```

Renderer-owned mapping:

```text
material_id -> material_gpu_record
asset_id -> texture_gpu_record
material_gpu_record -> descriptor_set
```

Rules:

- runtime/content own material identity and package validation;
- renderer owns GPU residency and fallback binding;
- missing material does not mutate runtime/content;
- renderer diagnostics must say whether fallback material was used;
- material sorting/batching is deferred until first textured path works.

## Descriptor Update Rules

Descriptor update may happen:

- during initialization;
- after upload completion;
- after frame-slot fence for per-frame descriptors;
- during material/resource loading when no in-flight command can read the old descriptor or when lifetime policy protects it.

Descriptor update must not happen:

- from runtime/projection code;
- while GPU may read a descriptor that is being invalidated;
- every frame for static material descriptors;
- without bounded counters;
- without diagnostic names.

First policy avoids update-after-bind. If update-after-bind is ever needed, it requires a separate feature/extension gate.

## Pipeline Layout Coupling

Pipeline layout owns:

- push constant ranges;
- descriptor set layout order;
- descriptor set layout compatibility.

Rules:

- shader interface and descriptor layouts must be planned together;
- changing descriptor layout can require pipeline recreation;
- descriptor set layouts must outlive pipelines/layouts that use them according to Vulkan lifetime rules;
- pipeline layout diagnostics must print descriptor layout names.

## Lifetime And Destruction

Recommended destruction order after descriptors exist:

1. Wait for in-flight work that may use descriptor sets/resources.
2. Destroy material/texture resources that are not needed.
3. Destroy descriptor pools/sets.
4. Destroy samplers/image views/images.
5. Destroy descriptor set layouts after pipelines/pipeline layouts no longer need them, following final file-plan ownership.
6. Destroy allocator/device after all descriptor-backed resources are gone.

Exact layout/pipeline destruction order must be reconciled with [lifetime.md](lifetime.md) when implementation begins.

## Diagnostics Receipt Fields

Descriptor diagnostics should include:

```text
descriptor_sets_enabled=true|false
descriptor_set_layout_count=
descriptor_pool_count=
descriptor_set_count=
descriptor_pool_strategy=
descriptor_pool_max_sets=
descriptor_layout_names=
descriptor_binding_summary=
per_frame_descriptor_sets=
per_frame_uniform_buffers=
descriptor_update_count_this_frame=
descriptor_update_count_total=
material_descriptor_count=
texture_descriptor_count=
sampler_count=
fallback_material_used=true|false
fallback_texture_used=true|false
descriptor_update_after_bind=false
reason=
```

For first-room baseline:

```text
descriptor_sets_enabled=false
descriptor_set_layout_count=0
descriptor_pool_count=0
descriptor_set_count=0
```

## Failure Reason Codes

Recommended reason codes:

```text
descriptor_scope_blocked
descriptor_layout_create_failed
descriptor_pool_create_failed
descriptor_pool_exhausted
descriptor_set_allocate_failed
descriptor_update_failed
descriptor_binding_mismatch
descriptor_resource_not_ready
uniform_buffer_alignment_invalid
texture_descriptor_missing
sampler_create_failed
fallback_texture_missing
pipeline_layout_descriptor_mismatch
descriptor_leak_to_runtime
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Validation Expectations

Validation blockers:

- binding descriptor set with incompatible pipeline layout;
- updating descriptor with invalid buffer/image/sampler;
- binding descriptor whose resource was destroyed;
- missing image layout transition before shader read;
- descriptor pool exhaustion hidden by fallback;
- descriptor set updated while in use without protected lifetime;
- descriptor types/stage flags not matching shader.

Strict descriptor smoke fails on validation/sync validation errors.

## Platform Notes

macOS/MoltenVK:

- keep descriptor path conservative at first;
- avoid descriptor indexing/update-after-bind until required and validated;
- MoltenVK quirks must be diagnostics, not runtime rules.

Linux:

- native Vulkan descriptor smoke should validate same layout/pool/update policy;
- hardware lane is required for shippable descriptor/material confidence;
- software Vulkan lane may help CI but is optional.

Windows:

- native Vulkan descriptor smoke should validate descriptor pool sizing and texture binding;
- package smoke must distinguish missing runtime/device from descriptor failures;
- validation logs should capture descriptor binding errors.

## Tests

Future tests:

```text
tests/unit/render_descriptor_policy_tests.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Unit expectations:

- first-room descriptor count is zero;
- descriptor scope is blocked until resource/upload gate is satisfied;
- descriptor layout names are stable;
- runtime/projection types contain no Vulkan descriptor names/types/handles;
- pool sizing math is deterministic.

Smoke expectations:

- descriptor layouts create/destroy validation-clean;
- descriptor pool allocates expected set count;
- fallback texture/material descriptor binds cleanly;
- first textured draw reports descriptor diagnostics;
- first-room push-constant path remains descriptor-free until descriptor scope opens.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'render_descriptor|vulkan_descriptor|vulkan_texture|vulkan_first_room'
```

## Acceptance Criteria

This descriptor policy is ready for file plans when:

- first-room no-descriptor baseline is explicit;
- descriptor entry gates are defined;
- descriptor set families are named;
- layout/binding naming policy is defined;
- descriptor pool sizing policy is defined;
- per-frame descriptor strategy is defined;
- uniform buffer gate is defined;
- texture/sampler descriptor gate is defined;
- descriptor update rules are defined;
- pipeline layout coupling is defined;
- diagnostics and reason codes are defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- descriptors cannot leak into runtime/projection/save truth.

## Open Detail Items

These belong in future file plans:

- exact descriptor layout C++ wrappers;
- exact pool size constants for first descriptor smoke;
- exact frame uniform buffer struct;
- exact alignment helper if dynamic offsets are used;
- exact fallback texture bytes;
- exact sampler creation helper;
- exact material GPU record shape;
- exact descriptor diagnostics formatting;
- exact Linux and Windows descriptor smoke commands.
