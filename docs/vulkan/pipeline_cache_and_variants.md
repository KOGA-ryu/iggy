# Vulkan Pipeline Cache And Variants

This document defines how `iggy3d` names, creates, reuses, recreates, and diagnoses Vulkan graphics pipelines as the renderer grows.

Pipelines are renderer-owned GPU state. Runtime/projection may emit backend-neutral draw items and material ids, but they do not own pipeline objects, pipeline layouts, shader modules, render-pass/dynamic-rendering choices, culling variants, descriptor layouts, or pipeline cache files.

## Purpose

Define controlled pipeline growth:

```text
first-room uses one simple pipeline
pipeline variants are named and keyed
dynamic rendering formats are part of compatibility
render-pass fallback is explicit
descriptor layouts are part of pipeline layout compatibility
culling/depth/debug/material variants are gated
pipeline creation happens outside normal per-frame work
pipeline recreate is diagnosed
pipeline cache is optional and renderer-owned
```

This document extends [shader_pipeline.md](shader_pipeline.md), [command_recording.md](command_recording.md), [descriptor_policy.md](descriptor_policy.md), [depth_and_coordinates.md](depth_and_coordinates.md), [swapchain_contract.md](swapchain_contract.md), and [fallbacks.md](fallbacks.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for graphics pipeline, pipeline layout, render-pass compatibility, dynamic rendering, and pipeline cache behavior.
- Vulkan Guide pipeline, shader, and dynamic rendering topics.
- Khronos Vulkan Tutorial graphics pipeline chapters for first implementation shape.
- Vulkan Samples for production-ish pipeline/cache/variant patterns after first-room proof.

## Scope

In scope:

- first-room pipeline baseline;
- pipeline names and keys;
- pipeline layout compatibility;
- shader module relationship;
- dynamic rendering format compatibility;
- render-pass fallback compatibility;
- depth/culling variants;
- debug variants;
- material/texture variants;
- creation timing;
- recreate triggers;
- optional pipeline cache policy;
- diagnostics and failure reason codes.

Out of scope:

- shader source language decision beyond compiled SPIR-V interface;
- descriptor implementation details;
- material authoring format;
- runtime/projection material truth;
- compute pipelines;
- ray tracing/mesh/task pipelines;
- hot reload implementation beyond future gate.

## Local File Surface

Likely future files:

```text
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/VulkanTypes.hpp
tests/unit/render_pipeline_policy_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
```

`PipelinesShaders` owns shader module loading, pipeline layout creation, pipeline creation, pipeline registry, variant keys, pipeline cache use, and pipeline diagnostics.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| shader modules | `PipelinesShaders` | created from generated SPIR-V |
| pipeline layouts | `PipelinesShaders` | push constants + descriptor set layouts |
| graphics pipelines | `PipelinesShaders` | named variants |
| pipeline cache | `PipelinesShaders` or package/cache helper | optional |
| variant keys | renderer pipeline module | diagnostic data |
| selected pipeline for draw | command recording | chosen from renderer-owned registry |
| material id | content/runtime/projection identity | no Vulkan object |

Rules:

- runtime/projection never store pipeline names as executable policy;
- renderer may map backend-neutral item/material kinds to pipeline variants;
- pipeline creation failure is renderer failure;
- pipeline changes cannot mutate runtime truth;
- pipeline diagnostics must use stable names.

## First-Room Baseline

First-room baseline:

```text
pipeline_family=first_room
pipeline_variant=first_room.vertex_color.opaque.depth.backface
shader_pair=first_room.vert.spv + first_room.frag.spv
layout=push_constants_only
descriptor_sets=0
rendering_path=dynamic_rendering
depth_test=enabled
depth_write=enabled
depth_compare=less
cull_mode=back unless winding fallback is active
front_face=counter_clockwise
```

Rules:

- one pipeline is enough for first visible room proof;
- debug draw may reuse this pipeline if geometry is compatible;
- no material/texture pipeline before descriptor/resource gates;
- no pipeline creation during normal per-frame rendering;
- any culling fallback must be diagnosed.

## Pipeline Key

Every pipeline variant should have a key.

Suggested key fields:

```text
pipeline_family
shader_vertex
shader_fragment
pipeline_layout_name
rendering_path
color_format
depth_format
primitive_topology
polygon_mode
cull_mode
front_face
depth_test
depth_write
depth_compare
blend_mode
vertex_format
descriptor_layout_names
debug_variant
material_model
```

Rules:

- key fields must be deterministic;
- key must include all state that changes compatibility or draw behavior;
- key should not include runtime entity ids;
- key should not include frame index;
- key should not include raw Vulkan handles;
- diagnostics should print a stable key string or structured fields.

## Pipeline Names

Recommended naming:

```text
pipeline.first_room.vertex_color.opaque.depth.backface
pipeline.debug.vertex_color.lines.depth_off
pipeline.material.unlit_textured.opaque.depth.backface
```

Rules:

- names are renderer diagnostics only;
- names are stable across runs when inputs are equivalent;
- names should correspond to shader/material/debug purpose;
- changing a pipeline name without changing behavior is docs/test churn and should be avoided.

## Pipeline Layout Compatibility

Pipeline layout includes:

- push constant ranges;
- descriptor set layout order;
- descriptor set layout compatibility.

First-room layout:

```text
pipeline_layout=push_constants_only
push_constant_bytes=64
descriptor_set_layout_count=0
```

Descriptor growth layouts:

```text
pipeline_layout=frame_material
set0=descriptor_layout.frame.v1
set1=descriptor_layout.material_unlit_textured.v1
```

Rules:

- command recording must bind descriptor sets compatible with the bound pipeline layout;
- push constant range must match shader expectations;
- layout change can require pipeline recreation;
- layout names must print in diagnostics;
- runtime/projection never know descriptor layout names.

## Shader Module Relationship

Rules:

- shader modules are created from generated SPIR-V;
- shader module lifetime can be temporary during pipeline creation;
- pipeline retains compiled state after shader modules are destroyed;
- shader interface must match pipeline vertex input, push constants, and descriptor layouts;
- shader artifact hash/version should be part of verbose diagnostics;
- shader module creation failure blocks pipeline creation.

Hot reload is deferred. If introduced later, shader artifact changes must trigger explicit pipeline recreation and diagnostics.

## Dynamic Rendering Compatibility

Dynamic rendering pipeline variants must include:

```text
color_format=selected_swapchain_format
depth_format=selected_depth_format
rendering_path=dynamic_rendering
```

Rules:

- pipeline creation requires selected color/depth formats unless pipeline creation is deferred until after swapchain setup;
- swapchain color format change can require pipeline recreation;
- depth format change can require pipeline recreation;
- command recording attachment formats must match pipeline metadata;
- dynamic rendering unsupported after selection is a renderer failure or explicit fallback gate.

## Render-Pass Fallback Compatibility

Render-pass fallback is allowed only through [fallbacks.md](fallbacks.md).

If selected:

```text
rendering_path=render_pass
render_pass_name=first_room.compat
framebuffer_lifetime=swapchain_extent
```

Rules:

- render-pass compatibility becomes part of pipeline key;
- framebuffer lifetime must be documented;
- fallback must be visible in diagnostics;
- dynamic and render-pass variants must not both silently compete;
- render-pass concepts must not enter runtime/projection.

## Depth And Culling Variants

Allowed first variants:

```text
depth.enabled.backface
depth.enabled.cull_none
debug.depth_off.cull_none
```

Rules:

- default first-room target is depth enabled, back-face culling, counter-clockwise front face;
- `cull_none` is allowed only when winding is unverified or for debug geometry;
- disabled culling must print fallback reason;
- depth-off debug variant may be introduced only if debug overlay needs it;
- variant count must remain small and diagnosed.

Forbidden early variants:

- every object kind gets a separate pipeline;
- every color gets a separate pipeline;
- per-entity pipeline variants;
- material variants before material/descriptor gate.

## Debug Pipeline Variants

Debug rendering may reuse first-room vertex-color pipeline if compatible.

Create debug-specific pipeline only when needed for:

- lines;
- depth-off overlays;
- different topology;
- different blend mode;
- special debug visualization.

Rules:

- debug pipeline names start with `pipeline.debug`;
- debug pipeline variants cannot affect runtime hash/replay;
- debug rendering can be disabled without changing projection output;
- unsupported debug primitive uses diagnosed skip/fallback.

## Material Pipeline Variants

Material pipeline variants are deferred until descriptors, textures, and material GPU records exist.

First material variant recommendation:

```text
pipeline.material.unlit_textured.opaque.depth.backface
descriptor_sets=set0_frame,set1_material
texture=base_color
lighting=none
```

Rules:

- material id maps to renderer-owned material GPU record;
- material GPU record selects pipeline/material descriptors;
- runtime/content own identity/package validation only;
- renderer owns fallback material pipeline and descriptor binding;
- material variants must be bounded and diagnosed.

## Blending Variants

Opaque first:

```text
blend_mode=opaque
```

Deferred:

- alpha blend;
- alpha test/cutout;
- additive debug;
- order-independent transparency.

Rules:

- transparent sorting is not first-room scope;
- alpha variants require draw ordering policy;
- blend mode belongs in pipeline key;
- material authoring does not force a new pipeline until material contract exists.

## Creation Timing

Allowed creation points:

- renderer initialization after device/swapchain/depth format selection;
- swapchain recreate if format/depth compatibility changes;
- material/texture growth initialization after descriptor layouts exist;
- explicit development hot reload later.

Forbidden:

- creating pipelines every frame;
- creating pipelines during command recording for normal draw;
- creating per-entity pipelines;
- silently creating fallback variants without diagnostics.

First implementation should create the first-room pipeline once after swapchain/depth format selection.

## Recreate Triggers

Pipeline recreate may be required when:

- swapchain color format changes;
- depth format changes;
- rendering path changes;
- shader artifacts change;
- pipeline layout changes;
- descriptor set layouts change;
- culling/depth fallback policy changes;
- device is recreated.

Pipeline recreate is not required for:

- swapchain extent-only resize;
- camera movement;
- runtime tick;
- projected draw item count change;
- material id change if existing material pipeline covers it.

Recreate must be diagnosed.

## Pipeline Cache Policy

Pipeline cache is optional for first implementation.

First policy:

```text
pipeline_cache=disabled_or_memory_only
pipeline_cache_file=none
```

Allowed later:

```text
pipeline_cache=persistent
pipeline_cache_file=build/artifacts/pipeline_cache/<platform>/<device>.bin
```

Rules:

- cache is performance infrastructure, not correctness;
- cache failure must not mutate runtime;
- corrupt/incompatible cache should be discarded with diagnostics;
- cache key must include device/driver/API details when persisted;
- release package cache policy must be documented before shipping.

Do not block first-room proof on persistent pipeline cache.

## Variant Count Policy

Keep variant count visible and bounded.

First-room target:

```text
pipeline_count=1
pipeline_variant_count=1
```

Early growth target:

```text
pipeline_variant_count <= small_explicit_limit
```

Rules:

- every variant must have a named reason;
- diagnostics print count and names;
- reviewer should reject "just in case" variants;
- variant growth follows actual render features.

## Diagnostics Receipt Fields

Pipeline diagnostics should include:

```text
pipeline_count=
pipeline_variant_count=
pipeline_family=
pipeline_variant=
pipeline_key=
pipeline_layout=
descriptor_set_layout_count=
descriptor_layout_names=
push_constant_bytes=
shader_vertex=
shader_fragment=
shader_artifact_hashes=
rendering_path=dynamic_rendering|render_pass
color_format=
depth_format=
primitive_topology=
polygon_mode=
cull_mode=
front_face=
depth_test=enabled|disabled
depth_write=enabled|disabled
depth_compare=
blend_mode=
pipeline_cache=disabled|memory_only|persistent
pipeline_cache_hit_count=
pipeline_cache_miss_count=
pipeline_recreate_count=
pipeline_recreate_reason=
reason=
```

Use `unavailable` only for fields that do not apply yet.

## Failure Reason Codes

Recommended reason codes:

```text
shader_module_create_failed
pipeline_layout_create_failed
pipeline_create_failed
pipeline_cache_create_failed
pipeline_cache_load_failed
pipeline_cache_store_failed
pipeline_format_mismatch
pipeline_layout_descriptor_mismatch
pipeline_push_constant_mismatch
pipeline_variant_missing
pipeline_variant_explosion
rendering_path_unsupported
render_pass_fallback_required
culling_fallback_active
material_pipeline_blocked
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Validation Expectations

Validation blockers:

- pipeline created with attachment formats that do not match dynamic rendering use;
- render-pass pipeline used with incompatible framebuffer/render pass;
- descriptor sets bound with incompatible pipeline layout;
- push constants outside declared range;
- vertex input mismatch with shader;
- binding wrong pipeline for command buffer render scope;
- destroying pipeline/layout while command buffers may still use it.

Strict pipeline smoke fails on validation errors.

## Platform Notes

macOS/MoltenVK:

- pipeline creation can expose shader/format incompatibilities through MoltenVK translation;
- dynamic rendering support must be proven before relying on dynamic variants;
- pipeline cache persistence should be deferred until first-room path is stable.

Linux:

- native Vulkan pipeline smoke should validate dynamic rendering, depth, culling, and descriptor growth when enabled;
- software Vulkan may help CI but does not replace hardware/native validation.

Windows:

- native Vulkan pipeline smoke should validate pipeline creation after swapchain/depth format selection;
- persistent cache path must be package-safe before shipping if enabled;
- validation logs should capture pipeline layout/descriptor mismatch.

## Tests

Future tests:

```text
tests/unit/render_pipeline_policy_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
```

Unit expectations:

- first-room pipeline key is deterministic;
- first-room variant count is one;
- extent-only resize does not require pipeline recreate;
- color/depth format change marks pipeline recreate required;
- descriptor layout change marks pipeline recreate required;
- runtime/projection types contain no pipeline identifiers as Vulkan policy.

Smoke expectations:

- first-room pipeline creates validation-clean;
- first-room draw binds expected pipeline variant;
- culling fallback prints reason if active;
- pipeline diagnostics include key fields;
- descriptor smoke creates compatible pipeline layout when descriptors open.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'render_pipeline|vulkan_pipeline|vulkan_first_room|vulkan_descriptor'
```

## Acceptance Criteria

This pipeline policy is ready for file plans when:

- first-room single-pipeline baseline is explicit;
- pipeline key fields are defined;
- pipeline names are defined;
- pipeline layout compatibility is defined;
- dynamic rendering compatibility is defined;
- render-pass fallback compatibility is defined;
- depth/culling/debug/material variant gates are defined;
- creation timing and recreate triggers are defined;
- pipeline cache policy is optional and defined;
- diagnostics and failure reason codes are defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- pipeline ownership cannot leak into runtime/projection/save truth.

## Open Detail Items

These belong in future file plans:

- exact C++ pipeline key type;
- exact pipeline registry/container;
- exact shader artifact hash calculation;
- exact pipeline cache persistence path if enabled;
- exact culling fallback expiration gate;
- exact debug primitive pipeline needs;
- exact material pipeline key after material contract exists;
- exact Linux and Windows pipeline smoke commands.
