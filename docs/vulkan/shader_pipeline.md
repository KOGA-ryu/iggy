# Shader Pipeline

This document defines shader authoring, compilation, generated SPIR-V handling, and the first graphics pipeline for `iggy3d`.

The first shader path is intentionally small. It should prove the renderer can compile shaders, load SPIR-V, create a pipeline, and draw a first 3D room without pulling materials, textures, descriptors, or asset streaming forward too early.

## Decision Status

Decision: shader language

Status: `Proposed`

Recommended default: GLSL with glslang for the first room.

Deferred option: Slang after cross-platform toolchain proof.

Rejected for first implementation: runtime-generated shaders, hand-authored checked-in SPIR-V with no source/provenance, and shader language references in runtime/content/projection/save.

Primary sources:

- glslang: https://github.com/KhronosGroup/glslang
- Vulkan Guide Slang page: https://docs.vulkan.org/guide/latest/slang.html
- Slang compiler docs, if evaluated later: https://shader-slang.org/slang/user-guide/compiling.html
- Khronos Vulkan Tutorial shader/pipeline chapters: https://docs.vulkan.org/tutorial/latest/00_Introduction.html

## Local File Surface

Likely future paths:

```text
shaders/vulkan/src/first_room.vert.glsl
shaders/vulkan/src/first_room.frag.glsl
build/generated/shaders/vulkan/<config>/first_room.vert.spv
build/generated/shaders/vulkan/<config>/first_room.frag.spv
cmake/iggy3d_shaders.cmake
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_shader_policy_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
```

The `<config>` component is required for multi-config generators on Windows. Single-config generators may use `default` or the active build type.

## Ownership

Shader source owns:

- human-authored shader logic;
- stage-specific entry points;
- shader interface declarations;
- no gameplay authority.

CMake shader integration owns:

- compiler discovery;
- compile commands;
- generated SPIR-V output path;
- dependencies from source to generated artifacts;
- build failure output.

`PipelinesShaders` owns:

- shader module loading;
- shader module lifetime;
- pipeline layout;
- push constant ranges;
- first graphics pipeline;
- pipeline diagnostics.

Renderer backend owns:

- swapchain color format;
- depth format;
- dynamic rendering or fallback render-pass choice;
- command-time pipeline binding;
- command-time push constants.

Runtime/projection owns:

- scene meaning;
- camera truth;
- projected draw items;
- no shader language or pipeline state.

## Dependency Rules

Allowed shader-tool references:

- `cmake/iggy3d_shaders.cmake`;
- `shaders/vulkan/src/**`;
- `src/render/vulkan/PipelinesShaders.*`;
- shader policy tests;
- Vulkan pipeline smoke tests.

Forbidden shader-tool references:

- `src/runtime/**`;
- `src/content/**`;
- `src/projection/**`;
- `src/runtime/save/**`;
- headless runtime apps;
- replay tools.

Firewall scan:

```sh
rg -n 'glsl|glslang|slang|SPIR-V|spirv|shader' src/runtime src/content src/projection src/runtime/save apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result: no production runtime/content/projection/save ownership leak. Matches in docs or Vulkan renderer files are allowed.

## Language Choice

First-room default: GLSL.

Why GLSL first:

- lower novelty for a first Vulkan room;
- mature Khronos glslang toolchain;
- easy stage mapping from `.vert.glsl` and `.frag.glsl`;
- enough for vertex-color first-room proof;
- keeps the renderer SPIR-V-facing so Slang can be adopted later without runtime churn.

Slang remains open because:

- current Khronos tutorial material uses Slang;
- Slang may be better for future multi-backend shader authoring;
- it should be selected only after compiler acquisition, packaging, and reproducible SPIR-V output are proven on macOS, Linux, and Windows.

Decision gate to move from GLSL to Slang:

- `slangc` discovery works on macOS, Linux, and Windows;
- generated SPIR-V is deterministic enough for tests;
- CMake integration is no more fragile than glslang;
- first-room pipeline loads Slang-generated SPIR-V without interface changes to runtime/projection;
- packaging can place compiled artifacts consistently.

## Source Folder Contract

Shader source root:

```text
shaders/vulkan/src/
```

First files:

```text
first_room.vert.glsl
first_room.frag.glsl
```

Naming rules:

- filename stem identifies the pipeline family: `first_room`;
- stage is encoded as `.vert` or `.frag`;
- language is encoded as `.glsl`;
- generated SPIR-V uses `.spv`;
- do not place generated files beside sources by default.

Allowed first-room shader scope:

- position input;
- vertex color input;
- push constant matrix;
- depth-compatible position output;
- fragment color output.

Forbidden first-room shader scope:

- texture sampling;
- material files;
- lighting model;
- skeletal animation;
- runtime command logic;
- package validation logic;
- save/replay logic.

## Generated SPIR-V Contract

Default generated root:

```text
build/generated/shaders/vulkan/<config>/
```

Generated files:

```text
first_room.vert.spv
first_room.frag.spv
```

Rules:

- shader source is committed;
- generated SPIR-V is a build artifact by default;
- generated SPIR-V is committed only if packaging later requires it;
- if generated SPIR-V is committed, source hash and generation command must be reproducible in tests;
- renderer loads SPIR-V from configured renderer shader path;
- runtime/content/projection/save never load shader files;
- missing shader artifacts produce machine-readable renderer diagnostics.

Install/package rule:

- packaging docs will define the final install location;
- renderer should consume a configured shader root rather than hardcoded source/build paths;
- visual demo may default to the build-generated shader root during development.

## Compiler Discovery

Proposed first compiler executable names:

```text
glslangValidator
glslang
```

The first pass should prefer `glslangValidator` if available because it is common in Vulkan SDK installations.

Compiler discovery rules:

- search explicit CMake cache variable first, such as `IGGY3D_GLSLANG_VALIDATOR`;
- then search `PATH`;
- then search Vulkan SDK hint paths if available;
- print compiler path and version when found;
- fail shader build if shader compilation is enabled and compiler is missing;
- do not require shader compiler when Vulkan/visual demo is disabled.

Proposed CMake cache variables:

```text
IGGY3D_ENABLE_VULKAN
IGGY3D_ENABLE_VISUAL_DEMO
IGGY3D_ENABLE_SHADER_COMPILE
IGGY3D_GLSLANG_VALIDATOR
IGGY3D_SHADER_OUTPUT_DIR
```

Foundation note: current foundation CMake options intentionally avoid renderer options. These options belong only to the later renderer-enabled build pass after headless acceptance is green.

## Compile Command Contract

Proposed GLSL command shape:

```sh
glslangValidator -V --target-env vulkan1.3 -o <output.spv> <input.glsl>
```

Rules:

- command must be emitted through CMake custom commands;
- input source file must be a dependency of its output SPIR-V;
- output directory must be created before compile;
- compiler stdout/stderr must appear in build logs on failure;
- command path and target environment must be recorded in diagnostics or build metadata;
- shader compile happens at build time, not during normal runtime.

Open detail:

- if a required platform only validates Vulkan 1.2 plus extensions, the shader target environment may need to become `vulkan1.2`; that decision belongs with the API baseline gate.

## CMake Integration Shape

Proposed helper file:

```text
cmake/iggy3d_shaders.cmake
```

Proposed function:

```cmake
iggy3d_compile_vulkan_shader(
  TARGET iggy3d_vulkan_shaders
  SOURCE shaders/vulkan/src/first_room.vert.glsl
  STAGE vert
  OUTPUT_NAME first_room.vert.spv
)
```

Proposed aggregate target:

```text
iggy3d_vulkan_shaders
```

Rules:

- Vulkan backend depends on `iggy3d_vulkan_shaders` only when shader compilation is enabled;
- visual demo depends on shader target when Vulkan backend is enabled;
- shader tests can depend on shader target;
- CMake must support multi-config output paths on Windows;
- no shader target is added when Vulkan is disabled unless explicitly requested for shader-only tests.

Needs file-plan detail:

- exact function arguments;
- exact imported/executable target for glslang;
- `BYPRODUCTS` handling;
- generator expression handling for `<config>`;
- install rules.

## First Room Shader Interface

The first room should use vertex color and one matrix push constant. This avoids descriptor layout work until the resource model phase.

Vertex input:

```cpp
struct FirstRoomVertex {
  Vec3 position;
  Vec3 color;
};
```

Shader locations:

```text
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 outColor;
```

Push constant:

```glsl
layout(push_constant) uniform PushConstants {
  mat4 clipFromModel;
} pc;
```

Fragment output:

```text
layout(location = 0) out vec4 outColor;
```

Rules:

- `clipFromModel` is computed by renderer CPU code from `FrameInput.camera.clipFromWorld` and the draw item's model transform;
- `clipFromModel` is presentation data and never written back to runtime;
- vertex color is fallback presentation data, not material truth;
- no descriptor sets are required for first-room baseline;
- descriptor work starts in `resource_model.md`.

Push constant size:

- `mat4` is expected to be 64 bytes;
- Vulkan guarantees at least 128 bytes of push constants;
- keep first-room push constant data at or below 64 bytes unless a later device diagnostics gate proves otherwise.

## First Room Pipeline State

Pipeline family: `first_room`.

Recommended first state:

```text
topology=triangle_list
polygon_mode=fill
cull_mode=back
front_face=counter_clockwise
depth_test=enabled
depth_write=enabled
depth_compare=less
msaa_samples=1
blend=disabled
viewport=dynamic
scissor=dynamic
color_format=swapchain_format
depth_format=selected_depth_format
rendering_path=dynamic
```

Rules:

- dynamic rendering is the proposed baseline;
- render-pass pipeline creation is fallback only if the dynamic rendering gate fails;
- color format comes from swapchain;
- depth format comes from renderer-owned depth selection;
- viewport/scissor come from swapchain extent, not runtime state;
- winding/culling must be validated against the first-room geometry before acceptance.

Open detail:

- exact depth format fallback order belongs in `resource_model.md` or `PipelinesShaders` file plan;
- if loaded/procedural geometry has inconsistent winding, first acceptance may temporarily disable culling, but that must be recorded as a fallback diagnostic.

## Shader Module And Pipeline Lifetime

Rules:

- load SPIR-V bytes during pipeline creation;
- create shader modules;
- create pipeline layout;
- create graphics pipeline;
- destroy shader modules after pipeline creation if not needed;
- keep pipeline and pipeline layout alive until backend shutdown or pipeline recreate;
- destroy pipeline before device destruction;
- rebuild pipeline if swapchain color format or depth format changes.

Diagnostics should include:

```text
shader_language=glsl
shader_compiler=
shader_target_env=vulkan1.3
shader_root=
vertex_shader=first_room.vert.spv
fragment_shader=first_room.frag.spv
pipeline_family=first_room
pipeline_layout=push_constants_only
rendering_path=dynamic
depth_test=enabled
cull_mode=back
front_face=counter_clockwise
```

## Error Handling

Build-time failures:

- missing compiler when shader compile is enabled;
- GLSL compile error;
- output path not writable;
- generated SPIR-V missing after custom command.

Runtime/pipeline failures:

- shader file missing;
- shader file unreadable;
- SPIR-V module creation failure;
- push constant range unsupported;
- pipeline layout creation failure;
- graphics pipeline creation failure;
- color/depth format mismatch;
- dynamic rendering unsupported after being selected.

Rules:

- all failures return renderer diagnostics or stable error codes;
- no shader/pipeline failure mutates runtime;
- visual demo exits nonzero in strict Vulkan mode;
- normal headless tools do not require shader files.

## Tests

Unit tests:

```text
tests/unit/render_shader_policy_tests.cpp
```

Required coverage:

- shader source paths are known;
- generated output paths are deterministic;
- runtime/content/projection/save have no shader-language references;
- missing compiler behavior is explicit;
- missing generated file behavior is explicit;
- shader diagnostics field names are stable.

Smoke tests:

```text
tests/smoke/vulkan_pipeline_smoke.cpp
```

Required coverage:

- generated SPIR-V exists;
- shader modules can be created;
- pipeline layout can be created;
- first pipeline can be created;
- pipeline diagnostics receipt is printed;
- validation is clean.

Commands:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy'
ctest --test-dir build --output-on-failure -R 'vulkan_pipeline'
```

Strict Vulkan command:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -R 'shader_policy|vulkan_pipeline'
```

## Acceptance Criteria

The shader pipeline design is ready for file plans when:

- GLSL/glslang remains the proposed first path or a recorded decision replaces it;
- shader source and generated paths are fixed;
- CMake helper responsibilities are defined;
- first-room vertex input is defined;
- push constant baseline is accepted;
- descriptor work remains deferred;
- dynamic rendering pipeline state is defined;
- fallback render-pass path is explicitly gated;
- diagnostics fields are named;
- shader tests and smoke tests are named.

## Open Detail Items

These belong in future file plans or later docs:

- exact glslang executable target/discovery code;
- exact shader compiler version parsing;
- exact CMake generator-expression handling for multi-config output;
- exact first-room GLSL source content;
- exact `FirstRoomVertex` owner file;
- exact depth format fallback order;
- exact culling decision after first geometry exists;
- exact install/package shader location;
- Slang migration proof if/when reopened.
