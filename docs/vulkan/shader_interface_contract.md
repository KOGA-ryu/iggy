# Vulkan Shader Interface Contract

This document defines the exact agreement between renderer-side C++ data and SPIR-V shader inputs for `iggy3d`.

Shader interfaces are renderer contracts. Runtime/content/projection may provide backend-neutral draw data, transforms, camera frame data, and material ids, but they do not own shader locations, descriptor set numbers, binding numbers, push constant byte ranges, SPIR-V decorations, or Vulkan pipeline layout compatibility.

## Purpose

Prevent shader and C++ drift:

```text
shader source declares a stable interface
generated SPIR-V preserves that interface
renderer pipeline layout matches that interface
command recording binds data that matches that interface
diagnostics prove the interface used for a smoke run
runtime/projection never learn Vulkan shader details
```

This document narrows [shader_pipeline.md](shader_pipeline.md), [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md), [descriptor_policy.md](descriptor_policy.md), [render_assets_and_materials.md](render_assets_and_materials.md), [depth_and_coordinates.md](depth_and_coordinates.md), and [frame_input_contract.md](frame_input_contract.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for shader stage interfaces, vertex input state, descriptor set/binding layout rules, push constants, pipeline layouts, SPIR-V environment, and pipeline validation rules.
- Vulkan Guide for shader, descriptor, dynamic rendering, and portability explanations.
- Khronos Vulkan Tutorial shader module, vertex input, descriptor, uniform buffer, texture, and pipeline layout chapters for first implementation shape.
- glslang documentation for GLSL-to-SPIR-V command flags and stage behavior while GLSL is the first-room shader language.
- Slang documentation only when the shader language decision gate is reopened.
- SPIRV-Tools reflection/disassembly docs if interface verification becomes automated.

Rules:

- Vulkan/GLSL/SPIR-V docs define shader interface behavior.
- Project docs define which renderer data is allowed to feed the interface.
- Runtime/content/projection docs do not define shader bindings.

## Scope

In scope:

- shader stage entry points;
- vertex formats and attribute locations;
- push constant layout;
- descriptor set and binding names;
- material shader inputs;
- matrix names and multiplication expectations;
- generated SPIR-V interface expectations;
- reflection or manual verification policy;
- pipeline layout compatibility;
- diagnostics and failure reason codes.

Out of scope:

- shader language final choice beyond current GLSL default;
- full material authoring format;
- PBR lighting;
- skeletal animation;
- compute shaders;
- shader hot reload;
- bindless descriptors;
- runtime gameplay logic;
- save/replay semantics.

## Local File Surface

Likely future files:

```text
shaders/vulkan/src/first_room.vert.glsl
shaders/vulkan/src/first_room.frag.glsl
shaders/vulkan/src/material_unlit_textured.vert.glsl
shaders/vulkan/src/material_unlit_textured.frag.glsl
cmake/iggy3d_shaders.cmake
src/render/FrameInput.hpp
src/render/RenderMesh.hpp
src/render/RenderMaterial.hpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/VulkanTypes.hpp
tests/unit/render_shader_interface_tests.cpp
tests/unit/render_vertex_format_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
tests/smoke/vulkan_material_smoke.cpp
```

`PipelinesShaders` should own shader module loading, pipeline layout creation, push constant ranges, descriptor set layout compatibility, and interface diagnostics. `CommandBuffers` should own binding the data that matches that interface.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| shader source interface declarations | shader source | locations, sets, bindings, push constant block |
| generated SPIR-V | build shader pipeline | build artifact, renderer input |
| vertex input state | Vulkan pipeline module | must match shader locations |
| push constant ranges | Vulkan pipeline module | must match shader stage use and byte size |
| descriptor set layouts | Vulkan descriptor/pipeline modules | must match shader set/binding declarations |
| material ids | content/runtime identity | no shader binding numbers |
| material GPU binding | renderer material/resource layer | maps id to descriptors/pipeline |
| camera/matrices | frame input/projection assembly | backend-neutral values consumed by renderer |
| shader compatibility diagnostics | renderer | printed during pipeline/smoke |

Rules:

- runtime/content/projection must not store shader locations, sets, bindings, or push constant offsets;
- shader files must not encode gameplay authority;
- renderer may reject an incompatible shader interface;
- renderer must not repair runtime data to hide a shader mismatch;
- shader interface changes are pipeline compatibility changes.

## Entry Gates

Shader interface implementation may begin after:

- [shader_pipeline.md](shader_pipeline.md) compiler discovery policy is accepted;
- first shader source folder and generated SPIR-V path are planned;
- [depth_and_coordinates.md](depth_and_coordinates.md) matrix/depth contract is accepted;
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md) first pipeline name and key are accepted;
- validation layers are enabled for pipeline smoke;
- descriptor interface work waits until [descriptor_policy.md](descriptor_policy.md) gates are satisfied.

First-room shader interface is allowed before texture/material descriptors because it uses vertex attributes and push constants only.

## Stage Entry Points

Default entry point:

```text
main
```

First-room shader stages:

```text
first_room.vert.glsl -> vertex stage
first_room.frag.glsl -> fragment stage
```

First material shader stages:

```text
material_unlit_textured.vert.glsl -> vertex stage
material_unlit_textured.frag.glsl -> fragment stage
```

Rules:

- entry point name must be stable and printed in diagnostics;
- stage must come from file extension or explicit build metadata;
- shader module creation failure must name source file, generated artifact, stage, and entry point;
- runtime code must not select shader entry points.

## First-Room Vertex Interface

First-room vertex input:

```text
location 0: in_position_model, vec3, float32
location 1: in_color, vec3 or vec4, float32
```

Recommended first C++ vertex format:

```text
struct FirstRoomVertex
position: float3
color: float3
```

Pipeline vertex input state:

```text
binding 0 stride = sizeof(FirstRoomVertex)
binding 0 inputRate = vertex
attribute location 0 format = R32G32B32_SFLOAT offset = position
attribute location 1 format = R32G32B32_SFLOAT offset = color
```

Rules:

- location numbers are shader/renderer contract only;
- vertex attribute order, format, and stride must be tested;
- vertex color is presentation fallback, not material truth;
- if `vec4` color is selected instead, the C++ format and diagnostics must change together;
- no texture coordinates are required for first-room proof.

## Material Vertex Interface

First unlit textured material vertex input:

```text
location 0: in_position_model, vec3, float32
location 1: in_normal_model, vec3, float32 optional_later
location 2: in_uv0, vec2, float32
location 3: in_color, vec3 or vec4, float32 optional_fallback
```

Recommended first material path:

```text
required: position, uv0
optional for diagnostics/fallback: color
deferred: normal, tangent, skin weights
```

Rules:

- do not require normals for the first unlit textured material;
- adding normals/tangents is a vertex format and pipeline variant change;
- UV origin and texture sampling convention must be documented before first texture smoke;
- mesh payload validation stays outside Vulkan, but renderer validates that the consumed vertex view matches its declared format.

## Fragment Outputs

First-room fragment output:

```text
location 0: out_color, vec4
```

First unlit textured fragment output:

```text
location 0: out_color, vec4
```

Rules:

- output location must match the color attachment at location 0;
- output color format compatibility is handled by pipeline/swapchain selection;
- alpha blending is disabled until a pipeline variant explicitly enables it;
- fragment output does not encode runtime visibility truth.

## Push Constant Contract

First-room push constant block:

```text
push_constant_name=DrawPushConstants
stage_flags=vertex
offset=0
size=64
field0=clipFromModel mat4
```

Rules:

- `clipFromModel` is the first required matrix;
- push constant byte size must be printed in pipeline diagnostics;
- push constant range must match pipeline layout;
- command recording must push constants after binding the compatible pipeline and before draw;
- push constants are not a runtime data model;
- if row-major/column-major conversion is required, the conversion point must be named in the file plan.

Deferred push constant fields:

```text
object_id_for_debug
material_index
flags
```

Adding fields requires:

- shader source update;
- C++ struct update;
- pipeline layout range review;
- diagnostics update;
- test update.

## Matrix Names And Semantics

Required renderer-side matrix names:

```text
viewFromWorld
clipFromView
clipFromWorld
worldFromModel
clipFromModel
```

Shader-visible first-room matrix:

```text
clipFromModel
```

Rules:

- shader receives `clipFromModel` and multiplies it by model-space position;
- shader does not own camera mode truth;
- `clipFromModel = clipFromView * viewFromWorld * worldFromModel`;
- Vulkan depth range is 0..1;
- all matrix values must be finite before command recording;
- renderer diagnostics should print matrix validity, not full matrices by default.

See [depth_and_coordinates.md](depth_and_coordinates.md).

## Descriptor Interface Contract

First-room baseline:

```text
descriptor_set_count=0
descriptor_bindings=none
```

First unlit textured material descriptor:

```text
set1_material.binding0 = combined_image_sampler_base_color
shader declaration = set 1, binding 0
stage = fragment
descriptor_type = combined image sampler
```

Potential future frame descriptor:

```text
set0_frame.binding0 = uniform_buffer_frame
shader declaration = set 0, binding 0
stage = vertex and/or fragment
descriptor_type = uniform buffer
```

Rules:

- set and binding numbers are renderer/shader contract only;
- descriptor set names from [descriptor_policy.md](descriptor_policy.md) must appear in diagnostics;
- descriptor layout order must match pipeline layout order;
- material shader must not assume a descriptor exists unless the selected pipeline layout includes it;
- adding/removing a descriptor binding is a shader interface version change.

## Descriptor Layout Names

Stable layout names:

```text
descriptor_layout.frame.v1
descriptor_layout.material_unlit_textured.v1
```

Rules:

- shader interface docs name the expected bindings;
- descriptor policy owns pool/set/update rules;
- pipeline policy owns layout compatibility with pipeline variants;
- diagnostics must print layout names and binding names when a descriptor pipeline is created.

## Shader Interface Versions

Every shader family should have an interface version:

```text
shader_interface.first_room.v1
shader_interface.material_unlit_textured.v1
```

Version increments are required when:

- vertex locations change;
- vertex formats change;
- push constant layout changes;
- descriptor sets or bindings change;
- fragment outputs change;
- matrix semantic names change.

Version increments are not required when:

- shader internal math changes without interface change;
- comments change;
- generated SPIR-V path changes while source/interface is identical.

## SPIR-V Interface Expectations

Generated SPIR-V must preserve:

- entry point name;
- stage;
- input locations;
- output locations;
- descriptor set numbers;
- descriptor binding numbers;
- push constant block and byte range;
- shader-visible names when debug info is enabled, if practical.

Rules:

- generated SPIR-V is renderer input only;
- pipeline smoke must name the SPIR-V artifacts loaded;
- stale SPIR-V must fail build or diagnostics rather than silently loading old shader code;
- committed SPIR-V, if later chosen, must carry provenance from shader source and generation command.

## Reflection Or Manual Verification

Initial acceptable path:

```text
manual interface table in docs
C++ constants or tests mirror the table
pipeline smoke prints interface diagnostics
validation layers catch Vulkan-level mismatches
```

Preferred later path:

```text
SPIR-V reflection or disassembly check verifies locations, sets, bindings, and push constants
```

Rules:

- manual verification is acceptable for first-room proof;
- descriptor/material growth should add automated checks if practical;
- reflection tooling must be build/dev tooling, not runtime gameplay dependency;
- if reflection is unavailable on a platform, tests may use checked metadata generated at build time.

Possible tools to evaluate later:

- SPIRV-Tools disassembly;
- SPIRV-Cross reflection;
- glslang reflection output if suitable;
- Slang reflection if Slang becomes the selected toolchain.

## Pipeline Compatibility

Pipeline creation must verify or diagnose:

```text
shader_interface_name
shader_interface_version
vertex_format_name
push_constant_bytes
descriptor_layout_names
color_output_location
pipeline_layout_name
pipeline_variant
```

Rules:

- vertex input state must match shader input locations;
- push constant range must cover shader push constant use;
- descriptor set layouts must match shader set/binding use;
- fragment output must match expected color attachment location;
- pipeline variant key must include shader interface version;
- incompatible shader interface is a renderer setup failure.

## Command Recording Requirements

Before each draw:

```text
bind compatible graphics pipeline
bind matching vertex buffer and optional index buffer
bind descriptor sets required by pipeline layout
push DrawPushConstants when required
issue draw or drawIndexed
```

Rules:

- command recording must not infer shader interface from runtime types;
- command recording uses renderer-private pipeline/material records;
- draw items missing required material descriptors use fallback or skip with diagnostics;
- command recording should not allocate shader interface resources during normal draw recording.

## Diagnostics Receipt

Pipeline/material smoke should print:

```text
shader_interface=shader_interface.first_room.v1
shader_stage_vertex=first_room.vert.spv
shader_stage_fragment=first_room.frag.spv
shader_entry_point=main
vertex_format=FirstRoomVertex.position_color
vertex_binding_count=
vertex_attribute_count=
attribute_location_0=in_position_model:R32G32B32_SFLOAT
attribute_location_1=in_color:R32G32B32_SFLOAT
push_constant_name=DrawPushConstants
push_constant_bytes=64
push_constant_stage_flags=vertex
descriptor_set_count=0
descriptor_layouts=
fragment_output_location_0=out_color
matrix_contract=clipFromModel
pipeline_layout=
pipeline_variant=
interface_validation=manual|reflected
runtime_shader_interface_leak=false
reason=
```

Material smoke should additionally print:

```text
shader_interface=shader_interface.material_unlit_textured.v1
attribute_location_2=in_uv0:R32G32_SFLOAT
descriptor_layouts=descriptor_layout.material_unlit_textured.v1
descriptor_binding=set1_material.binding0:combined_image_sampler_base_color
material_model=material.unlit_textured
```

## Failure Reason Codes

Use stable reason codes:

```text
shader_interface_scope_blocked
shader_entry_point_missing
shader_stage_mismatch
shader_spirv_stale
shader_spirv_missing
vertex_location_missing
vertex_format_mismatch
vertex_stride_mismatch
push_constant_range_missing
push_constant_size_mismatch
push_constant_stage_mismatch
descriptor_set_missing
descriptor_binding_missing
descriptor_type_mismatch
descriptor_stage_mismatch
fragment_output_mismatch
matrix_contract_invalid
pipeline_layout_shader_mismatch
shader_interface_version_mismatch
runtime_shader_interface_leak
```

Rules:

- reason codes should appear in diagnostics and failing smoke output;
- reason code names should not vary by platform;
- compiler errors may include tool-native messages, but renderer reason codes should wrap them.

## Validation Expectations

Validation should catch:

- descriptor set layout mismatch;
- pipeline layout mismatch;
- push constant range mismatch;
- descriptor type mismatch;
- missing descriptor set bind before draw;
- invalid vertex buffer bind;
- invalid shader module input to pipeline creation.

Tests should catch:

- stale generated SPIR-V;
- C++ vertex format table drift;
- shader interface version drift;
- forbidden shader binding leakage into runtime/content/projection;
- missing diagnostic fields.

Suggested leak scan:

```sh
rg -n "layout\\s*\\(|set1_material|binding0|shader_interface|gl_Position|glsl|SPIR-V|spirv" src/runtime src/content src/projection src/runtime/save
```

Expected result: no production runtime/content/projection/save ownership leak.

## Platform Notes

The shader interface contract must be identical across macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan.

Rules:

- generated SPIR-V must be accepted by all target platforms before the interface is considered stable;
- shader compiler discovery may differ by platform, but generated interface must not;
- macOS/MoltenVK may impose portability constraints, but it does not redefine descriptor or vertex locations;
- Windows multi-config generated shader paths must not change shader interface identity;
- Linux and Windows native Vulkan smoke should print the same shader interface names as macOS.

## Tests

Expected future tests:

```text
tests/unit/render_shader_interface_tests.cpp
tests/unit/render_vertex_format_tests.cpp
tests/unit/render_shader_artifact_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
tests/smoke/vulkan_material_smoke.cpp
```

Unit tests should cover:

- first-room vertex locations;
- first-room vertex stride and offsets;
- push constant byte size;
- descriptor layout names;
- shader interface version names;
- failure reason stability.

Smoke tests should cover:

- first-room shader interface receipt;
- material unlit textured interface receipt;
- missing SPIR-V failure path;
- stale SPIR-V detection if supported by build metadata;
- descriptor shader mismatch failure path where practical.

## Acceptance Criteria

Shader interface work is acceptable only when:

- first-room shader interface name and version are documented;
- C++ vertex format matches documented locations and formats;
- push constant range matches shader use;
- descriptor-free first-room pipeline remains validation-clean;
- unlit material descriptor interface is documented before implementation;
- pipeline diagnostics print interface version, vertex format, push constant size, descriptor layouts, and SPIR-V artifact names;
- runtime/content/projection/save scans show no shader binding or Vulkan leakage;
- macOS, Linux, and Windows smoke plans use the same interface names.

## Open Detail Items

The next detailed pass should define:

- exact C++ first-room vertex struct name and layout;
- exact material vertex struct name and layout;
- whether colors are `vec3` or `vec4`;
- UV origin convention;
- exact GLSL push constant block syntax;
- exact descriptor GLSL syntax for `set1_material.binding0`;
- whether SPIR-V reflection is required before material descriptors;
- exact stale-SPIR-V detection method;
- shader interface constants location in C++;
- Linux shader compiler install/discovery command;
- Windows shader compiler install/discovery command.
