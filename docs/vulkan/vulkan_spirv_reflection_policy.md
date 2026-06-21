# Vulkan SPIR-V Reflection Policy

This document defines how `iggy3d` verifies shader interface drift between GLSL source, generated SPIR-V, and renderer-side C++ pipeline/descriptor metadata.

Shader reflection is renderer build/test infrastructure. It must not become runtime truth, gameplay command legality, camera truth, save/load state, replay determinism, package validation, or projection semantics.

## Purpose

Define the first reflection policy:

```text
first_room_reflection=manual_metadata_allowed
descriptor_material_reflection=required_before_growth
preferred_reflection_tool=SPIRV-Reflect_initially
spirv_tools_role=validate_disassemble_not_primary_reflection
spirv_cross_role=secondary_if_cross_compile_or_text_reflection_needed
glslang_reflection_role=not_authoritative_for_final_spirv
manual_metadata_role=small_first_room_and_expected_schema
runtime_shader_interface_knowledge=false
```

The first room may use explicit manual metadata because it has no descriptors and a tiny interface. Before descriptor/material growth, reflection or an equivalent generated manifest must verify that SPIR-V inputs, push constants, descriptor sets, bindings, and shader stages match C++ expectations.

This document narrows:

- [shader_interface_contract.md](shader_interface_contract.md)
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md)
- [descriptor_policy.md](descriptor_policy.md)
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md)
- [render_assets_and_materials.md](render_assets_and_materials.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| SPIRV-Reflect: https://github.com/KhronosGroup/SPIRV-Reflect | lightweight C/C++ reflection API for SPIR-V shader bytecode in Vulkan applications |
| SPIRV-Cross: https://github.com/KhronosGroup/SPIRV-Cross | reflection plus cross-compilation/disassembly when broader shader-tooling needs appear |
| SPIRV-Cross reflection guide: https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide | practical reflection API behavior and resource querying |
| SPIRV-Tools: https://github.com/KhronosGroup/SPIRV-Tools | SPIR-V validation, disassembly, optimization, and diagnostics support; not primary reflection |
| glslang: https://github.com/KhronosGroup/glslang | GLSL-to-SPIR-V compiler and source/AST-side reflection context; not final SPIR-V interface authority |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html | final shader interface, descriptor, push constant, pipeline layout, and SPIR-V environment rules |
| Project shader/descriptor docs | local pipeline names, expected layouts, diagnostics, and runtime firewall |

Priority rule:

```text
shader_interface_truth=Vulkan Specification and generated SPIR-V
reflection_source_of_record=post_compile_SPIR-V_bytecode
compiler_truth=glslang_or_future_shader_compiler
validation_truth=SPIRV-Tools_and_Vulkan_validation_layers
project_policy=iggy3d Vulkan shader docs
runtime_truth=iggy3d runtime docs, never reflection metadata
```

## Decision

Recommended path:

```text
first_room=manual_expected_interface_metadata
before_descriptors=adopt_SPIRV-Reflect_or_equivalent_bytecode_reflection
SPIRV-Cross=defer_until_cross_compile_or_text_reflection_need
SPIRV-Tools=use_for_spirv-val_and_disassembly_not_reflection
glslang_reflection=do_not_use_as_final_authority
```

Rationale:

- first-room proof should not be blocked by adding another dependency;
- first-room interface is small enough to test manually: vertex locations, push constant size, fragment output;
- descriptor/material growth adds set/binding/layout drift risk that manual metadata will not scale to safely;
- reflection should inspect generated SPIR-V, not only source AST, because the renderer consumes SPIR-V;
- SPIRV-Reflect is narrower and Vulkan-oriented for reflection;
- SPIRV-Cross is valuable if the project needs text reflection, MSL/HLSL/GLSL cross-compilation, or broader shader inspection later;
- SPIRV-Tools remains mandatory-quality tooling for validation/disassembly but is not the reflection owner.

Decision gate:

```text
reflection_tool_final_choice_before_descriptor_material_growth=true
```

If SPIRV-Reflect acquisition fails on macOS, Linux, or Windows, the fallback is SPIRV-Cross reflection or a generated manifest built from a vetted tool. Do not proceed to descriptor/material implementation with hand-maintained binding metadata only.

## Scope

In scope:

- first-room manual metadata;
- reflection tool choice;
- generated reflection manifest;
- shader interface drift detection;
- vertex input verification;
- push constant verification;
- descriptor set/binding verification;
- fragment output verification;
- pipeline layout compatibility checks;
- CMake/build integration shape;
- diagnostics and strict smoke behavior.

Out of scope:

- implementing reflection now;
- shader hot reload;
- runtime shader compilation;
- material authoring format;
- bindless descriptors;
- GPU-assisted validation requirement;
- cross-compiling shaders to MSL/HLSL now;
- using reflection metadata as package validation truth.

## Local File Surface

Likely future files:

```text
cmake/iggy3d_shaders.cmake
cmake/iggy3d_spirv_reflection.cmake
shaders/vulkan/src/first_room.vert.glsl
shaders/vulkan/src/first_room.frag.glsl
shaders/vulkan/metadata/first_room.interface.kv
shaders/vulkan/metadata/material_unlit_textured.interface.kv
build/generated/shaders/vulkan/<config>/first_room.vert.spv
build/generated/shaders/vulkan/<config>/first_room.vert.reflect.kv
build/generated/shaders/vulkan/<config>/first_room.frag.spv
build/generated/shaders/vulkan/<config>/first_room.frag.reflect.kv
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/VulkanShaderReflection.hpp
src/render/vulkan/VulkanShaderReflection.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
tests/unit/render_shader_reflection_policy_tests.cpp
tests/unit/render_shader_interface_tests.cpp
tests/unit/render_descriptor_reflection_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_descriptor_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| generated SPIR-V | shader build pipeline | runtime truth |
| reflection tool discovery | CMake shader/reflection helper | gameplay behavior |
| reflection manifest | shader build/test artifacts | save/replay state |
| expected interface metadata | renderer shader policy files | content package validation |
| pipeline layout validation | `PipelinesShaders` / reflection helper | runtime command legality |
| descriptor set/binding validation | descriptor/pipeline modules | projection semantics |
| reflection diagnostics | render diagnostics | shader compiler authority |

Rules:

- runtime/content/projection/save must not depend on reflection libraries;
- reflection output is renderer build/test data;
- reflection manifests may be packaged for diagnostics only, not gameplay authority;
- renderer may fail startup or smoke when shader interface drift is detected;
- renderer must not patch runtime/projection data to match a shader drift.

## Tool Roles

| Tool | Role | Use now? | Notes |
| --- | --- | --- | --- |
| manual metadata | expected schema for tiny first-room interface | yes, first room only | must be tested and replaced/augmented before descriptors |
| SPIRV-Reflect | preferred first reflection library | before descriptor/material growth | narrow C/C++ SPIR-V reflection API for Vulkan |
| SPIRV-Cross | secondary reflection/cross-compile tool | defer | useful if text reflection or MSL/HLSL cross-compilation becomes needed |
| SPIRV-Tools | validator/disassembler/optimizer | yes for validation/disassembly | use `spirv-val`/`spirv-dis`, not as primary reflection |
| glslang reflection | source/AST-side inspection | not final authority | compiler/source reflection may differ from emitted SPIR-V details |

Hard rule:

```text
final_interface_check_must_use_generated_spirv_or_manifest_derived_from_generated_spirv=true
```

## First-Room Manual Metadata

Manual metadata is allowed only for first-room baseline:

```text
pipeline_family=first_room
descriptor_sets=0
push_constant_ranges=1
vertex_locations=0,1
fragment_outputs=0
```

Expected metadata:

```text
pipeline_family=first_room
entry_point.vertex=main
entry_point.fragment=main
vertex_input.0.location=0
vertex_input.0.name=in_position_model
vertex_input.0.format=R32G32B32_SFLOAT
vertex_input.1.location=1
vertex_input.1.name=in_color
vertex_input.1.format=R32G32B32_SFLOAT
push_constant.0.name=DrawPushConstants
push_constant.0.stage=vertex
push_constant.0.offset=0
push_constant.0.size=64
descriptor_sets=
fragment_output.0.location=0
fragment_output.0.name=out_color
```

Rules:

- manual metadata must live under `shaders/vulkan/metadata/**` or the renderer shader policy path named in the shader file plan;
- manual metadata is compared against C++ expected values in unit tests;
- first-room strict smoke must print metadata version and interface hash;
- manual metadata must not expand to material descriptors without a reflection decision.

## Descriptor/Material Reflection Gate

Before descriptor/material growth, reflection must verify:

```text
descriptor_set_numbers
descriptor_binding_numbers
descriptor_types
descriptor_counts
shader_stage_visibility
push_constant_ranges
vertex_input_locations
fragment_output_locations
entry_point_names
pipeline_layout_family
```

Required for first material descriptor path:

```text
set1_material.binding0=combined_image_sampler_base_color
stage=fragment
descriptor_count=1
fallback_texture_binding_verified=true
```

Rules:

- descriptor set layouts must be generated from or checked against reflected SPIR-V;
- C++ hardcoded descriptor layout is allowed only if reflection verifies it;
- mismatch is a strict failure;
- no material descriptor smoke passes without reflection or an approved equivalent manifest;
- adding a binding requires updating shader, expected metadata, tests, and diagnostics together.

## Reflection Manifest

Generated reflection output should be normalized into a stable project manifest.

Suggested manifest:

```text
schema=1
source=shaders/vulkan/src/material_unlit_textured.frag.glsl
spv=build/generated/shaders/vulkan/Debug/material_unlit_textured.frag.spv
compiler=glslangValidator
reflection_tool=spirv-reflect
entry_point=main
stage=fragment
source_hash=
spv_hash=
interface_hash=
descriptor_sets=
push_constants=
inputs=
outputs=
```

Rules:

- normalize tool output so tests are not coupled to one tool-specific shape;
- include source and SPIR-V hashes;
- include reflection tool name/version;
- include target environment;
- store generated manifests in build tree by default;
- package manifests only if needed for visual package diagnostics.

## Interface Hash Policy

Every pipeline family should have an interface hash.

Hash inputs:

```text
entry_points
stages
vertex_inputs
fragment_outputs
descriptor_sets
bindings
descriptor_types
descriptor_counts
push_constant_ranges
shader_stage_visibility
```

Do not include:

```text
absolute_build_paths
timestamps
human comments
full SPIR-V binary bytes
compiler warning prose
```

Rules:

- interface hash changes mean pipeline compatibility review;
- shader source hash changes do not always mean interface hash changes;
- interface hash must be printed in pipeline smoke receipts;
- descriptor/material cache keys may use interface hash later, but runtime must not.

## CMake Integration Policy

First reflection CMake option shape:

```cmake
option(IGGY3D_ENABLE_SPIRV_REFLECTION "Generate/check Vulkan SPIR-V reflection manifests" ON)
option(IGGY3D_REQUIRE_SPIRV_REFLECTION "Fail if SPIR-V reflection tool is unavailable" OFF)
set(IGGY3D_SPIRV_REFLECTION_TOOL "spirv-reflect" CACHE STRING "SPIR-V reflection tool")
set(IGGY3D_SPIRV_REFLECT_EXE "" CACHE FILEPATH "spirv-reflect executable")
set(IGGY3D_SPIRV_CROSS_EXE "" CACHE FILEPATH "spirv-cross executable")
```

Rules:

- reflection runs after SPIR-V generation;
- strict descriptor/material smoke sets `IGGY3D_REQUIRE_SPIRV_REFLECTION=ON`;
- headless runtime builds must not require reflection tools;
- optional first-room pipeline smoke may run with manual metadata only;
- generated reflection files are build artifacts with declared dependencies.

## Runtime Firewall

Forbidden:

```text
src/runtime/** includes reflection headers
src/content/** includes reflection headers
src/projection/** includes reflection headers
src/runtime/save/** includes reflection headers
runtime code reads reflection manifests
save/replay serializes reflection data
content package validation depends on SPIRV reflection
projection emits descriptor set or binding numbers
```

Allowed:

```text
cmake/iggy3d_spirv_reflection.cmake discovers tools
src/render/vulkan/** consumes reflection manifests or library output
tests/unit/render_* verifies expected shader interface
tests/smoke/vulkan_* prints reflection diagnostics
shaders/vulkan/metadata/** stores expected renderer shader interface
```

Firewall scan:

```sh
rg -n "SPIRV-Reflect|spirv-reflect|SPIRV-Cross|spirv-cross|reflection|descriptor_set|binding" \
  src/runtime src/content src/projection src/runtime/save \
  apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save reflection ownership leak
```

## Failure Policy

Shader reflection failures map through renderer diagnostics:

| Failure | Lane behavior | Reason code |
| --- | --- | --- |
| reflection tool missing for first-room manual path | optional continue if manual metadata exists | `spirv_reflection_unavailable` |
| reflection tool missing for strict descriptor/material path | fail | `spirv_reflection_required_unavailable` |
| generated manifest malformed | fail | `spirv_reflection_manifest_malformed` |
| vertex location mismatch | fail | `shader_vertex_interface_mismatch` |
| push constant mismatch | fail | `shader_push_constant_mismatch` |
| descriptor set/binding mismatch | fail | `shader_descriptor_interface_mismatch` |
| fragment output mismatch | fail | `shader_fragment_output_mismatch` |
| interface hash mismatch | fail unless expected metadata updated intentionally | `shader_interface_hash_mismatch` |
| SPIR-V validation failure | fail | `spirv_validation_failed` |

Rules:

- strict descriptor/material smoke cannot skip reflection mismatch;
- optional lane may skip only missing tool/environment before renderer work;
- reflection mismatch is a renderer/shader integration failure, not runtime failure;
- runtime hash must remain unchanged.

## Diagnostics Receipt Fields

Shader/pipeline receipts should include:

```text
shader_reflection=manual|spirv_reflect|spirv_cross|unavailable
shader_reflection_required=true|false
shader_reflection_tool=
shader_reflection_tool_version=
shader_reflection_manifest=
shader_interface_hash=
shader_expected_interface_hash=
shader_interface_match=true|false
spirv_validation=pass|fail|unavailable
vertex_input_count=
descriptor_set_count=
descriptor_binding_count=
push_constant_range_count=
fragment_output_count=
first_interface_mismatch=
reason_code=
```

Rules:

- first-room smoke may print `shader_reflection=manual`;
- descriptor/material smoke must print a bytecode-reflection tool or approved equivalent;
- mismatch receipts must name the shader stage and pipeline family;
- absolute paths should be avoided or normalized in public receipts.

## Test Gates

Unit tests should prove:

- manual first-room metadata matches expected C++ vertex/push constant contract;
- missing descriptor reflection tool fails strict descriptor/material policy;
- reflected descriptor sets match C++ descriptor layout metadata;
- reflected push constant ranges match pipeline layout metadata;
- interface hash is stable across path/timestamp changes;
- interface hash changes when binding/location/range changes;
- malformed reflection manifest fails;
- runtime firewall scan is clean.

Smoke tests should prove:

- first-room pipeline receipt reports manual or reflected interface status;
- descriptor smoke refuses to run without required reflection;
- material smoke prints descriptor binding reflection data;
- shader interface mismatch fails before draw;
- SPIR-V validation runs before reflection-dependent pipeline acceptance when enabled.

## Tool Choice Revisit Gate

Revisit SPIRV-Reflect versus SPIRV-Cross when any of these become true:

- Slang becomes the selected shader authoring path;
- MoltenVK/MSL cross-compile diagnostics require SPIRV-Cross output;
- generated text reflection from a command-line tool is preferred over a linked C++ dependency;
- SPIRV-Reflect package acquisition fails on one required platform;
- shader interface needs exceed what SPIRV-Reflect exposes cleanly;
- packaging wants a single external tool already provided by the Vulkan SDK.

Until that gate, do not add both reflection libraries. More shader tools increase build and packaging surface.

## Review Checklist

Reviewer should reject a shader/descriptor packet if:

- descriptor bindings are hand-maintained with no reflection or generated manifest;
- glslang source reflection is treated as final SPIR-V truth;
- SPIRV-Tools validation is called reflection;
- runtime/projection stores descriptor set or binding numbers;
- shader interface mismatch is downgraded to warning;
- reflection manifests include unstable absolute paths in interface hashes;
- material descriptor growth starts before reflection decision is implemented;
- Linux and Windows tool acquisition are ignored.

## Acceptance Criteria

This policy is ready for implementation when:

- [ ] first-room manual metadata is defined and tested;
- [ ] descriptor/material growth is blocked behind SPIR-V-bytecode reflection or an approved generated manifest;
- [ ] preferred reflection tool is SPIRV-Reflect unless evidence changes the decision;
- [ ] SPIRV-Tools role is validation/disassembly, not reflection;
- [ ] glslang reflection is not used as final interface authority;
- [ ] interface hashes are defined;
- [ ] diagnostics print reflection status and mismatch reasons;
- [ ] runtime firewall scan is clean.

## Non-Goals For First Renderer Pass

Do not add:

- descriptor/material reflection before first-room baseline needs it;
- multiple reflection libraries at once;
- runtime reflection loading;
- shader hot reload;
- generated C++ code from reflection;
- package validation based on shader bindings;
- bindless descriptor reflection;
- cross-compilation to MSL/HLSL;
- Slang reflection policy before the shader-language decision reopens.
