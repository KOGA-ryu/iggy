# cmake/iggy3d_shaders.cmake

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `cmake/iggy3d_shaders.cmake`

Purpose: Compile Vulkan GLSL shader sources into generated SPIR-V artifacts with stable metadata.

## Build Position

Packet order: 6 - Shaders, pipeline, resources, descriptors
Owner module: `CMake shader build`
File kind: `cmake`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/vulkan_shader_build_pipeline.md`

## Ownership

This file owns:
- Compile Vulkan GLSL shader sources into generated SPIR-V artifacts with stable metadata.
- the public or private names listed in the file shape section;
- diagnostics fields directly tied to its responsibility.

This file must never own:
- gameplay truth;
- command legality;
- save/load truth;
- replay or deterministic state-hash truth;
- content package validation truth;
- renderer fallback without diagnostics;
- legacy renderer linkage.

Runtime firewall boundaries:
- runtime, content, projection, and save code do not depend on this file unless it is a backend-neutral render contract explicitly consumed by an app layer;
- this file cannot mutate runtime state directly;
- renderer output cannot affect replay results.

## Required Include Policy

Allowed inputs: CMake built-ins, project options, imported dependency targets, shader source paths, and install path variables.

Forbidden inputs: old repo paths, runtime state assumptions, source-tree generated output writes, and hard-coded developer machine paths.

Vulkan headers are referenced only through dependency targets and include directories, not through runtime code.

SDL is referenced only as a build dependency for visual/platform targets.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `iggy3d_add_vulkan_shader`
- `iggy3d_vulkan_shaders target`
- `IGGY3D_GLSLANG_VALIDATOR`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

CMake files own build graph, dependency targets, generated artifact paths, and install rules. They do not own runtime state. Generated files belong under build or package output roots, not under runtime truth.

## Semantics

Normal path: configure feature-gated graphics build edges without affecting headless targets. Skip/fail: optional graphics targets may be disabled; strict visual/shader lanes fail when required tools or runtime dependencies are absent.

Platform behavior:
macOS/MoltenVK: report `platform=macos` and `platform_lane=moltenvk` when Vulkan is attempted; MoltenVK portability details are diagnostics, not cross-platform law.
Linux: report `platform=linux` and `platform_lane=native_vulkan` for hardware/native validation; software Vulkan uses a separate lane.
Windows: report `platform=windows` and `platform_lane=native_vulkan`; multi-config shader/package paths must include the active config where relevant.
Software Vulkan: allowed for optional development evidence only; it cannot replace native macOS/Linux/Windows proof.
Strict lane: required gates fail with `result=fail`.
Optional lane: unsupported environment or missing optional Vulkan prerequisites may skip with `result=skip` before unsafe renderer work begins.

## Diagnostics And Result Policy

Stable reason names must use lowercase snake-case text. Receipts use deterministic key-value lines.

Required receipt fields:
```text
receipt_version=1
repo=iggy3d
file_plan=cmake/iggy3d_shaders.cmake
packet_order=6
allowed_to_implement_code_now=false
cmake_target=
dependency_mode=
shader_root=
package_mode=
result=pass|fail|skip
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: dependencies are feature-gated. Headless targets build without Vulkan/SDL/shader tools; strict visual targets fail when required dependencies are absent.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=CMake_shader_build
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: configure-time dependency discovery or build-time shader commands.
Per-frame cost: none.
Resize cost: none.
GPU memory cost: none.

## Tests And Verification

Unit tests:
- `tests/unit/render_shader_build_policy_tests.cpp`

Smoke tests:
- `tests/smoke/vulkan_pipeline_smoke.cpp`

CTest labels:
```text
iggy3d;build;vulkan
```

Expected pass behavior: required receipt fields are present and the file owns only the declared responsibility.
Expected skip behavior: optional Vulkan lanes may skip only before required Vulkan work begins and must print `result=skip` plus `reason_code`.
Expected fail behavior: strict lanes fail on missing required dependency, validation error, runtime mutation, or boundary leak.

Firewall scan:
```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected firewall scan result:
```text
no matches
```

## Packet 6 Detailed Contract

Current-build CMake API:
```cmake
option(IGGY3D_ENABLE_VULKAN_SHADERS "Build Vulkan shader artifacts" OFF)
option(IGGY3D_REQUIRE_GLSLANG "Fail configure/build when glslangValidator is unavailable" OFF)
set(IGGY3D_SHADER_SOURCE_ROOT "${PROJECT_SOURCE_DIR}/shaders/vulkan/src" CACHE PATH "Vulkan shader source root")
set(IGGY3D_SHADER_BINARY_ROOT "${PROJECT_BINARY_DIR}/generated/shaders/vulkan/$<CONFIG>" CACHE PATH "Generated Vulkan SPIR-V root")
```

Function surface:
```cmake
iggy3d_add_vulkan_shader(
  TARGET <target>
  STAGE <vert|frag>
  SOURCE <source-path>
  OUTPUT_NAME <artifact-name>
)

iggy3d_add_vulkan_shader_set(
  TARGET <target>
  SOURCES <source-list>
)
```

Build behavior:
- Discover `glslangValidator` through `find_program`.
- Generate SPIR-V under `build/generated/shaders/vulkan/<config>/`.
- Use generator-expression-aware paths for Visual Studio, Xcode, Ninja, and Ninja Multi-Config.
- Source files remain under `shaders/vulkan/src/`.
- Generated SPIR-V is build output, not committed source truth.
- Headless runtime targets do not depend on shader targets.

Required first artifacts:
```text
first_room.vert.glsl -> first_room.vert.spv
first_room.frag.glsl -> first_room.frag.spv
material_unlit_textured.vert.glsl -> material_unlit_textured.vert.spv
material_unlit_textured.frag.glsl -> material_unlit_textured.frag.spv
```

Stale artifact policy:
- Custom commands depend on source shader files.
- Shader compile command line includes target environment and stage.
- A source timestamp or compiler command change rebuilds the matching SPIR-V.
- Build output path includes config for multi-config generators.

Diagnostics:
```text
shader_language=glsl
shader_compiler=glslangValidator
shader_target_env=vulkan1.3
shader_source_root=<absolute-path>
shader_binary_root=<absolute-path>
shader_artifacts_found=true|false
shader_compile_target=<target-name>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Failure policy:
- Missing compiler fails when `IGGY3D_REQUIRE_GLSLANG=ON`.
- Missing compiler leaves Vulkan shader target unavailable when shader build is not required.
- Runtime/content/projection/save never branch on shader compiler availability.

Verification:
- `render_shader_build_policy_tests` validates path policy, generated output names, and headless isolation.
- Pipeline smoke consumes generated SPIR-V from configured shader root.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not make headless runtime targets require Vulkan, SDL, display access, or shader tools.
- Do not write generated SPIR-V beside committed shader source by default.

## Completion Criteria

- File `cmake/iggy3d_shaders.cmake` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
