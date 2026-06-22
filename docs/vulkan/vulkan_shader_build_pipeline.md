# Vulkan Shader Build Pipeline

This document defines the exact CMake and glslang build pipeline for compiling `iggy3d` Vulkan GLSL shaders into SPIR-V. It covers compiler discovery, generated artifact paths, stale artifact detection, build metadata, install/package handoff, and Windows multi-config behavior.

Shader compilation is build infrastructure for the renderer. Runtime, content, projection, save, replay, and gameplay code must not depend on shader compilers, generated SPIR-V paths, CMake shader targets, shader stage names, or Vulkan shader toolchain state.

Packet 1 must not depend on this shader pipeline. The first Vulkan shader path remains GLSL plus `glslangValidator`; Slang remains a later secondary investigation unless a future packet explicitly re-scopes shader language policy.

## Purpose

Define a reproducible first shader build path:

```text
shader_language=GLSL
shader_compiler=glslangValidator
shader_target_env=vulkan1.3 unless API baseline selects vulkan1.2
shader_source_root=shaders/vulkan/src/
shader_generated_root=<binary_dir>/generated/shaders/vulkan/$<CONFIG>/
shader_metadata_required=true
stale_artifact_detection=source_hash_plus_command_metadata
multi_config_safe=true
runtime_shader_compiler_dependency=false
```

This document narrows:

- [shader_pipeline.md](shader_pipeline.md)
- [shader_interface_contract.md](shader_interface_contract.md)
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md)
- [packaging.md](packaging.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [vulkan_feature_baseline.md](vulkan_feature_baseline.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| glslang project: https://github.com/KhronosGroup/glslang | Khronos GLSL-to-SPIR-V compiler behavior, stage extension rules, command-line tool role |
| Khronos reference compiler page: https://www.khronos.org/opengles/sdk/Reference-Compiler/ | glslang reference compiler role and command-line usage expectations |
| CMake `find_program`: https://cmake.org/cmake/help/latest/command/find_program.html | compiler discovery and cache-variable behavior |
| CMake `add_custom_command`: https://cmake.org/cmake/help/latest/command/add_custom_command.html | source-to-generated-file build edges, `DEPENDS`, `BYPRODUCTS`, `VERBATIM`, `USES_TERMINAL` |
| CMake `add_custom_target`: https://cmake.org/cmake/help/latest/command/add_custom_target.html | aggregate shader target that depends on generated outputs |
| CMake generator expressions: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html | `$<CONFIG>` and build-configuration-specific paths |
| CMake Ninja Multi-Config: https://cmake.org/cmake/help/latest/generator/Ninja%20Multi-Config.html | multi-config build model and per-config generated outputs |
| CMake `file(GENERATE)`: https://cmake.org/cmake/help/latest/command/file.html | config-specific generated helper/manifest files if needed |
| How to Vulkan in 2026: https://howtovulkan.com/ | secondary practical reference for the modern Slang direction and shader-to-SPIR-V workflow comparison; does not override the current GLSL/glslang first path |

Priority rule:

```text
shader_compiler_truth=glslang docs and executable behavior
build_graph_truth=CMake official docs
shader_interface_truth=Vulkan and SPIR-V interface docs
practical_shader_comparison=How to Vulkan only after official toolchain policy is satisfied
runtime_truth=project runtime docs, never shader build files
```

## Scope

In scope:

- first-room GLSL shader build;
- `glslangValidator` discovery;
- CMake cache variables;
- shader source naming;
- generated SPIR-V path naming;
- CMake custom command shape;
- aggregate shader target;
- multi-config output behavior;
- stale artifact detection;
- metadata sidecar policy;
- install/package handoff;
- diagnostics and tests.

Out of scope:

- implementing the CMake files now;
- Slang adoption;
- shader hot reload;
- runtime shader compilation;
- checked-in SPIR-V as the default;
- shader reflection implementation;
- material shader authoring beyond naming first likely files;
- platform package installers.

## Local File Surface

Likely future files:

```text
CMakeLists.txt
cmake/iggy3d_shaders.cmake
cmake/iggy3d_shader_manifest.cmake
shaders/vulkan/src/first_room.vert.glsl
shaders/vulkan/src/first_room.frag.glsl
shaders/vulkan/src/material_unlit_textured.vert.glsl
shaders/vulkan/src/material_unlit_textured.frag.glsl
build/generated/shaders/vulkan/<config>/first_room.vert.spv
build/generated/shaders/vulkan/<config>/first_room.vert.spv.meta.kv
build/generated/shaders/vulkan/<config>/first_room.frag.spv
build/generated/shaders/vulkan/<config>/first_room.frag.spv.meta.kv
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
tests/unit/render_shader_build_policy_tests.cpp
tests/unit/render_shader_artifact_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
```

This document does not create those build files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| shader source files | shader source folder | runtime authority, command legality |
| compiler discovery | CMake shader helper | runtime startup |
| generated SPIR-V | build tree or package resource tree | content/package validation truth |
| shader metadata sidecar | build shader helper | gameplay state |
| shader target | CMake renderer build | headless runtime tools |
| shader root path | renderer config/visual app | runtime/content/projection/save |
| SPIR-V loading | Vulkan pipeline module | CMake compiler discovery |

Rules:

- headless runtime builds do not require glslang;
- visual/Vulkan builds require glslang only when shader compilation is enabled;
- packaged visual builds may use prebuilt SPIR-V and must not require glslang at runtime;
- runtime/content/projection/save must not mention glslang, `.spv`, shader output paths, or CMake shader targets.

## Entry Gates

Shader build implementation may begin after:

- headless runtime acceptance is green;
- renderer boundary exists;
- [shader_pipeline.md](shader_pipeline.md) still selects GLSL/glslang for the first room;
- [shader_interface_contract.md](shader_interface_contract.md) defines first-room shader inputs;
- Vulkan API baseline has selected `vulkan1.3` or a documented fallback target;
- visual/Vulkan CMake options exist or are being introduced in the same renderer build packet.

Reviewer should reject shader build work if it makes the headless runtime path require Vulkan SDK, glslang, generated SPIR-V, or display access.

## CMake Version Policy

Recommended shader-helper CMake minimum:

```text
cmake_minimum_required_for_shader_helper=3.20
```

Reason:

- generator expressions in generated output paths are required;
- `$<CONFIG>` is required for multi-config generators;
- CMake 3.20 added broader generator-expression support in custom command outputs/byproducts;
- Ninja Multi-Config support is mature enough for the shader path.

If the project sets a lower root CMake minimum, the shader helper must either:

- raise the minimum only when Vulkan shader compilation is enabled; or
- use a less elegant generated path strategy that is explicitly tested on Visual Studio, Xcode, Ninja, and Ninja Multi-Config.

## CMake Options

Proposed options:

```cmake
option(IGGY3D_ENABLE_VULKAN "Build Vulkan renderer components" OFF)
option(IGGY3D_ENABLE_VISUAL_DEMO "Build visual demo app" OFF)
option(IGGY3D_ENABLE_SHADER_COMPILE "Compile Vulkan GLSL shaders to SPIR-V" ON)
option(IGGY3D_REQUIRE_SHADER_COMPILE "Fail configure/build if shader compiler is unavailable" OFF)
set(IGGY3D_GLSLANG_VALIDATOR "" CACHE FILEPATH "Path to glslangValidator executable")
set(IGGY3D_SHADER_TARGET_ENV "vulkan1.3" CACHE STRING "glslang --target-env value for Vulkan shaders")
set(IGGY3D_SHADER_OUTPUT_DIR "" CACHE PATH "Optional shader output root override")
```

Rules:

- `IGGY3D_ENABLE_SHADER_COMPILE` has no effect unless Vulkan/visual shader targets are requested or shader-only tests are requested;
- strict platform lanes set `IGGY3D_REQUIRE_SHADER_COMPILE=ON`;
- optional developer lanes may skip shader compile if prebuilt artifacts exist;
- target env must match the renderer Vulkan baseline decision.

## Compiler Discovery

Discovery order:

```text
1. IGGY3D_GLSLANG_VALIDATOR cache variable if non-empty
2. find_program names: glslangValidator, glslang
3. Vulkan SDK hints from VULKAN_SDK when present
4. normal PATH and CMake program search behavior
```

Policy:

- prefer `glslangValidator`;
- accept `glslang` only if the command supports the required options in the selected SDK/toolchain;
- print compiler path during configure when shader compile is enabled;
- print compiler version during configure or shader policy test;
- missing compiler is a configure or build failure only when shader compile is required;
- missing compiler must not affect headless runtime targets.

Proposed CMake shape:

```cmake
set(_iggy3d_glslang_hints)
if(DEFINED ENV{VULKAN_SDK})
  list(APPEND _iggy3d_glslang_hints "$ENV{VULKAN_SDK}/bin")
endif()

if(IGGY3D_GLSLANG_VALIDATOR)
  set(IGGY3D_GLSLANG_VALIDATOR_EXE "${IGGY3D_GLSLANG_VALIDATOR}" CACHE FILEPATH "glslangValidator executable" FORCE)
else()
  find_program(
    IGGY3D_GLSLANG_VALIDATOR_EXE
    NAMES glslangValidator glslang
    HINTS ${_iggy3d_glslang_hints}
  )
endif()
```

Validation command:

```sh
glslangValidator --version
```

If `glslang` rather than `glslangValidator` is found, the file plan must verify the exact invocation still emits Vulkan SPIR-V with the selected target environment.

## Shader Source Contract

Source root:

```text
shaders/vulkan/src/
```

First files:

```text
first_room.vert.glsl
first_room.frag.glsl
```

Material growth files:

```text
material_unlit_textured.vert.glsl
material_unlit_textured.frag.glsl
```

Rules:

- shader stage is explicit in filename;
- GLSL language is explicit in filename;
- generated artifacts are not written beside source files;
- shader source is committed;
- generated SPIR-V is build output by default;
- generated SPIR-V is committed only if a packaging decision explicitly changes that rule.

Stage mapping:

| Source suffix | Stage | glslang stage expectation |
| --- | --- | --- |
| `.vert.glsl` | vertex | file copied or invoked with explicit stage as needed |
| `.frag.glsl` | fragment | file copied or invoked with explicit stage as needed |
| `.comp.glsl` | compute later | deferred |

Because glslang commonly infers stage from extensions such as `.vert` and `.frag`, the CMake helper must either:

- pass a stage option if using `.vert.glsl`; or
- generate/copy a temporary stage-suffixed input name; or
- use source names like `first_room.vert` and document that `.glsl` is omitted.

Preferred policy:

```text
source_name=first_room.vert.glsl
cmake_helper_passes_explicit_stage=true
```

## Generated Path Contract

Default generated root:

```text
${CMAKE_BINARY_DIR}/generated/shaders/vulkan/$<CONFIG>/
```

If `$<CONFIG>` evaluates to an empty value for a single-config generator, the helper must use a stable fallback directory:

```text
single_config_fallback=default
```

Acceptable generated roots:

```text
multi_config: <build>/generated/shaders/vulkan/$<CONFIG>/
single_config: <build>/generated/shaders/vulkan/default/
```

First generated files:

```text
first_room.vert.spv
first_room.vert.spv.meta.kv
first_room.frag.spv
first_room.frag.spv.meta.kv
```

Rules:

- Debug and Release artifacts must not overwrite each other;
- Visual Studio and Ninja Multi-Config builds must put artifacts under the active config;
- renderer startup diagnostics must print the shader root it is using;
- package install rules may flatten config-specific output only at install/package time;
- build-tree visual demo should consume the config-specific generated root.

## Compile Command Contract

Baseline command:

```sh
glslangValidator -V --target-env vulkan1.3 -S <stage> -o <output.spv> <input.glsl>
```

Rules:

- `-V` is required for Vulkan SPIR-V output;
- `--target-env` must match `IGGY3D_SHADER_TARGET_ENV`;
- `-S` must be used if source filenames keep `.vert.glsl` / `.frag.glsl`;
- output directory is created before compile;
- compiler stdout/stderr appears in build logs;
- command uses `VERBATIM`;
- source file appears in `DEPENDS`;
- helper CMake script appears in `DEPENDS` if it affects command behavior;
- generated metadata is created in the same command or a dependent command;
- shader compilation happens at build time, not runtime.

Proposed CMake command shape:

```cmake
add_custom_command(
  OUTPUT "${_spv_output}"
  BYPRODUCTS "${_spv_meta_output}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${_spv_output_dir}"
  COMMAND "${IGGY3D_GLSLANG_VALIDATOR_EXE}"
          -V
          --target-env "${IGGY3D_SHADER_TARGET_ENV}"
          -S "${_stage}"
          -o "${_spv_output}"
          "${_shader_source}"
  COMMAND "${CMAKE_COMMAND}"
          -DINPUT="${_shader_source}"
          -DOUTPUT="${_spv_output}"
          -DMETA="${_spv_meta_output}"
          -DCOMPILER="${IGGY3D_GLSLANG_VALIDATOR_EXE}"
          -DTARGET_ENV="${IGGY3D_SHADER_TARGET_ENV}"
          -P "${CMAKE_SOURCE_DIR}/cmake/iggy3d_shader_manifest.cmake"
  DEPENDS "${_shader_source}" "${CMAKE_SOURCE_DIR}/cmake/iggy3d_shader_manifest.cmake"
  COMMENT "Compiling Vulkan shader ${_shader_source}"
  VERBATIM
)
```

`USES_TERMINAL` is optional. Use it only if shader errors are unreadable without direct terminal output.

## Aggregate Target

Target name:

```text
iggy3d_vulkan_shaders
```

Policy:

```cmake
add_custom_target(iggy3d_vulkan_shaders DEPENDS ${IGGY3D_VULKAN_SHADER_OUTPUTS})
```

Dependency rules:

- visual demo depends on `iggy3d_vulkan_shaders` when shader compile is enabled;
- shader policy tests depend on `iggy3d_vulkan_shaders`;
- Vulkan pipeline smoke depends on `iggy3d_vulkan_shaders` when it needs build-generated shaders;
- headless runtime targets never depend on `iggy3d_vulkan_shaders`;
- package targets depend on shader artifacts only when packaging visual/Vulkan outputs.

## Multi-Config Handling

Problem:

```text
Visual Studio, Xcode, and Ninja Multi-Config can build Debug and Release from one build tree.
If shader outputs omit config, one config can silently overwrite another.
```

Required rule:

```text
all build-generated shader outputs include config directory
```

Command examples:

```sh
cmake --build build --config Debug --target iggy3d_vulkan_shaders
cmake --build build --config Release --target iggy3d_vulkan_shaders
```

Expected outputs:

```text
build/generated/shaders/vulkan/Debug/first_room.vert.spv
build/generated/shaders/vulkan/Debug/first_room.frag.spv
build/generated/shaders/vulkan/Release/first_room.vert.spv
build/generated/shaders/vulkan/Release/first_room.frag.spv
```

Rules:

- Visual Studio builds must not load Release shaders when running Debug visual demo from the build tree;
- Debug and Release metadata files must be separate;
- shader root passed to the visual demo must include the active config;
- CTest invocations for multi-config generators must pass `-C <Config>` or the equivalent preset;
- Windows path handling must quote every path through CMake variables and `VERBATIM`.

## Stale Artifact Detection

Stale SPIR-V must fail build/test diagnostics rather than silently loading old shader code.

Build graph stale detection:

- `.spv` output depends on shader source;
- `.spv` output depends on shader manifest helper script;
- `.spv` output depends on any explicitly listed include files once shader includes are introduced;
- CMake reconfigure is required when compiler path or target environment changes;
- clean removes generated SPIR-V and metadata.

Metadata stale detection:

Each `.spv.meta.kv` must record:

```text
schema_version=
source_path=
source_sha256=
output_path=
output_sha256=
compiler_path=
compiler_version=
target_env=
stage=
entry_point=main
command_line=
generated_utc=
config=
```

Shader policy tests must verify:

- metadata exists for every required `.spv`;
- source hash in metadata matches current source;
- output hash in metadata matches current `.spv`;
- target env matches configured target env;
- stage matches expected stage;
- compiler path is non-empty when artifacts are build-generated;
- `.spv` is newer than or equal to source when using timestamp checks as a secondary signal.

If shader includes are introduced:

- include files must appear in build dependencies;
- either glslang depfile output must be proven reliable for the selected generators, or includes must be listed explicitly in CMake;
- include hash coverage must be added to metadata.

Do not rely only on file timestamps. Timestamps catch obvious local mistakes, but hashes and command metadata catch copied stale artifacts, wrong config outputs, and compiler-target drift.

## Generated Metadata Helper

Preferred helper:

```text
cmake/iggy3d_shader_manifest.cmake
```

Responsibilities:

- compute source hash;
- compute output hash after compile;
- run compiler version command if not already captured;
- write sidecar metadata atomically enough for build use;
- avoid non-portable shell syntax;
- work on macOS, Linux, and Windows.

Rules:

- use CMake script mode for portability;
- do not use shell redirection in custom commands unless there is a proven cross-platform wrapper;
- do not require Python for shader metadata unless the project has already adopted Python as a build dependency;
- metadata helper failures fail the shader target.

## Install And Package Handoff

Build-tree root:

```text
<build>/generated/shaders/vulkan/<config>/
```

Installed visual package root:

```text
<install>/share/iggy3d/shaders/vulkan/
```

Windows package alternative:

```text
<package>/shaders/vulkan/
```

Rules:

- install/package step may copy only `.spv` files;
- metadata files may be installed in developer packages but are not required for runtime packages;
- package smoke must verify required `.spv` files exist;
- packaged runtime does not require glslang;
- missing packaged shader produces `reason_code=shader_missing`.

## Diagnostics Receipt

Shader build or pipeline smoke should print:

```text
shader_build_policy_version=
shader_language=glsl
shader_compiler_path=
shader_compiler_name=glslangValidator|glslang
shader_compiler_version=
shader_target_env=
shader_source_root=
shader_generated_root=
shader_runtime_root=
shader_config=Debug|Release|RelWithDebInfo|MinSizeRel|default
shader_compile_enabled=true|false
shader_compile_required=true|false
shader_metadata_required=true
shader_artifact_count=
shader_artifact_0=
shader_artifact_0_source_sha256=
shader_artifact_0_output_sha256=
shader_artifact_0_stale=false
multi_config_generator=true|false
runtime_shader_compiler_dependency=false
reason=
```

## Validation And Tests

Firewall scan:

```sh
rg -n "glslang|glslangValidator|SPIR-V|spirv|\\.spv|shader_generated|IGGY3D_GLSLANG|IGGY3D_SHADER" src/runtime src/content src/projection src/runtime/save apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save/headless shader build dependency leak
```

Unit tests should cover:

- shader source list includes first-room vertex and fragment shaders;
- generated path includes config or stable single-config fallback;
- metadata exists for each generated artifact;
- metadata source hash matches current source;
- metadata output hash matches artifact;
- stale metadata fails policy test;
- missing compiler behavior respects required/optional mode;
- target env mismatch fails policy test;
- headless build/test path does not require shader target.

Smoke tests should cover:

- build `iggy3d_vulkan_shaders`;
- verify first-room `.spv` files exist;
- load first-room `.spv` files into Vulkan shader modules;
- print shader receipt;
- run on macOS/MoltenVK first, then Linux native Vulkan, then Windows native Vulkan;
- for Windows, build at least Debug and Release with a multi-config generator and verify distinct output roots.

Suggested commands:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON
cmake --build build --target iggy3d_vulkan_shaders
ctest --test-dir build --output-on-failure -R "shader_build|shader_artifact|vulkan_pipeline"
```

Windows multi-config lane:

```sh
cmake -S . -B build-vs -G "Visual Studio 17 2022" -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON
cmake --build build-vs --config Debug --target iggy3d_vulkan_shaders
cmake --build build-vs --config Release --target iggy3d_vulkan_shaders
ctest --test-dir build-vs -C Debug --output-on-failure -R "shader_build|shader_artifact"
ctest --test-dir build-vs -C Release --output-on-failure -R "shader_build|shader_artifact"
```

Ninja Multi-Config lane:

```sh
cmake -S . -B build-nmc -G "Ninja Multi-Config" -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON
cmake --build build-nmc --config Debug --target iggy3d_vulkan_shaders
cmake --build build-nmc --config Release --target iggy3d_vulkan_shaders
```

## Failure Reason Codes

Use stable reason codes in diagnostics:

```text
shader_build_scope_blocked
shader_compiler_missing
shader_compiler_invalid
shader_compiler_version_unknown
shader_target_env_invalid
shader_source_missing
shader_stage_unknown
shader_output_dir_create_failed
shader_compile_failed
shader_spirv_missing
shader_spirv_unreadable
shader_metadata_missing
shader_metadata_invalid
shader_spirv_stale
shader_source_hash_mismatch
shader_output_hash_mismatch
shader_command_mismatch
shader_config_path_collision
shader_wrong_config_loaded
shader_runtime_compiler_dependency
shader_runtime_leak
```

## Acceptance Gate

This policy is ready for implementation planning when:

- GLSL/glslang remains accepted for the first room;
- CMake minimum/version strategy is accepted;
- compiler discovery order is accepted;
- generated path policy handles single-config and multi-config generators;
- stale detection metadata fields are accepted;
- first shader source filenames are accepted;
- target env is tied to the Vulkan baseline decision;
- Windows Debug/Release output collision is impossible by construction;
- package handoff rules are compatible with [packaging.md](packaging.md);
- firewall scan has no shader build leaks outside renderer/build-owned files.
