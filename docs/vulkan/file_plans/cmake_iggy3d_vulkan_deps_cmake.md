# cmake/iggy3d_vulkan_deps.cmake

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `cmake/iggy3d_vulkan_deps.cmake`

Purpose: Discover and gate Vulkan, SDL3, validation/runtime dependencies, and renderer build options.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `CMake Vulkan deps`
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
- `docs/vulkan/packaging.md`
- `docs/vulkan/vulkan_ci_and_smoke_lanes.md`

## Ownership

This file owns:
- Discover and gate Vulkan, SDL3, validation/runtime dependencies, and renderer build options.
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
- `IGGY3D_ENABLE_VULKAN`
- `IGGY3D_ENABLE_VISUAL_DEMO`
- `Vulkan::Vulkan`
- `SDL3 target`

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
file_plan=cmake/iggy3d_vulkan_deps.cmake
packet_order=3
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
fallback_area=CMake_Vulkan_deps
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
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/vulkan_platform_smoke.cpp`

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

## Packet 3 Detailed Contract

Current-build CMake options:
```cmake
option(IGGY3D_ENABLE_VULKAN "Enable Vulkan renderer build surface" OFF)
option(IGGY3D_ENABLE_VISUAL_DEMO "Build the visual demo app" OFF)
option(IGGY3D_USE_SYSTEM_SDL3 "Use a system SDL3 package" ON)
option(IGGY3D_ENABLE_VULKAN_SMOKE "Build Vulkan smoke tests" OFF)
option(IGGY3D_REQUIRE_VULKAN_SMOKE "Fail smoke lanes instead of skipping unsupported hosts" OFF)
option(IGGY3D_REQUIRE_VALIDATION_LAYERS "Require Vulkan validation layers in strict Vulkan lanes" OFF)
option(IGGY3D_REQUIRE_SYNC_VALIDATION "Require synchronization validation in strict Vulkan lanes" OFF)
```

Target policy:
- Headless runtime targets must configure and build with all Vulkan options off.
- `iggy3d_render` may exist without Vulkan enabled when it contains only backend-neutral and null renderer code.
- `iggy3d_visual_demo` links SDL3 only when `IGGY3D_ENABLE_VISUAL_DEMO=ON`.
- Vulkan backend targets link Vulkan loader/headers only when `IGGY3D_ENABLE_VULKAN=ON`.
- Smoke tests are registered only when the corresponding build option is on.

Dependency discovery:
- Use `find_package(Vulkan)` only when Vulkan is enabled.
- Use `find_package(SDL3 CONFIG)` when system SDL3 is requested.
- Emit deterministic configure messages for found/missing Vulkan, SDL3, glslang, and shader toolchain inputs.
- No configure-time network access.
- No dependency on any legacy repository path.

Configure messages:
```text
iggy3d.vulkan.enabled=<ON|OFF>
iggy3d.visual_demo.enabled=<ON|OFF>
iggy3d.sdl3.source=<system|disabled>
iggy3d.vulkan.loader=<found|missing>
iggy3d.vulkan.smoke=<enabled|disabled>
iggy3d.vulkan.strict_smoke=<ON|OFF>
```

Platform behavior:
- macOS: surface support is expected through SDL plus MoltenVK when Vulkan is enabled.
- Linux: SDL3 must expose the active WSI path, commonly X11 or Wayland.
- Windows: Visual Studio multi-config output paths must be supported by shader/resource lookup.
- Software Vulkan lanes remain runtime smoke configuration, not CMake global truth.

Verification:
- Configure-only tests prove headless builds do not require Vulkan or SDL3.
- Visual-demo configure tests prove SDL3 discovery is isolated to visual targets.
- Vulkan smoke configure tests prove missing Vulkan is a skip in non-strict smoke lanes and a configure/test fail in strict lanes according to build option.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not make headless runtime targets require Vulkan, SDL, display access, or shader tools.
- Do not write generated SPIR-V beside committed shader source by default.

## Completion Criteria

- File `cmake/iggy3d_vulkan_deps.cmake` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
