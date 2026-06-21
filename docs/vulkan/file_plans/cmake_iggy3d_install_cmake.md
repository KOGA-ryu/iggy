# cmake/iggy3d_install.cmake

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `cmake/iggy3d_install.cmake`

Purpose: Install visual package resources, shaders, SDL3/MoltenVK runtime dependencies, and diagnostics-safe layout.

## Build Position

Packet order: 7 - First visible room proof and packaging smoke
Owner module: `CMake packaging`
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
- `docs/vulkan/vulkan_package_runtime_lookup.md`

## Ownership

This file owns:
- Install visual package resources, shaders, SDL3/MoltenVK runtime dependencies, and diagnostics-safe layout.
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
- `install rules for visual package`
- `shader install dir`
- `runtime dependency copy rules`

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
file_plan=cmake/iggy3d_install.cmake
packet_order=7
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
fallback_area=CMake_packaging
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
- `tests/smoke/package_visual_startup_smoke.cpp`
- `tests/smoke/package_vulkan_dependency_smoke.cpp`

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

## Packet 7 Detailed Contract

Current-build CMake options:
```cmake
option(IGGY3D_INSTALL_HEADLESS_DEMO "Install headless runtime demo" ON)
option(IGGY3D_INSTALL_VISUAL_DEMO "Install visual Vulkan demo" OFF)
option(IGGY3D_INSTALL_VULKAN_SHADERS "Install generated Vulkan SPIR-V artifacts" OFF)
option(IGGY3D_INSTALL_RUNTIME_DEPS "Install bundled visual runtime dependencies where supported" OFF)
option(IGGY3D_INSTALL_VALIDATION_TOOLS "Install validation tooling for dev packages" OFF)
```

Install layout policy:
- Headless install contains no Vulkan runtime, SDL runtime, shader artifacts, or display dependency.
- Visual install contains the visual demo executable, shader artifacts, resource root, and package metadata.
- Generated SPIR-V is installed from `build/generated/shaders/vulkan/<config>/`.
- Shader source may be installed only in dev packages.
- Diagnostics artifacts are runtime output and are not installed as source truth.

Platform layouts:
```text
macos_visual_shaders=<bundle>/Contents/Resources/shaders/vulkan
linux_visual_shaders=<prefix>/share/iggy3d/shaders/vulkan
windows_visual_shaders=<package_root>/resources/shaders/vulkan
```

Runtime dependency policy:
- macOS visual packages must account for SDL3 and MoltenVK runtime lookup.
- Linux visual packages depend on system Vulkan loader, GPU driver/ICD, and SDL3 unless a later packaging packet bundles SDL3.
- Windows visual packages must account for SDL3 DLL placement and system Vulkan runtime lookup.
- Packages must not bundle GPU drivers.

Receipt fields:
```text
install_mode=<headless|visual>
install_prefix=<absolute-path>
shader_install_root=<absolute-path-or-empty>
resource_install_root=<absolute-path-or-empty>
runtime_deps_installed=true|false
validation_tools_installed=true|false
headless_requires_graphics=false
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Verification:
- `package_headless_smoke` proves headless install has no graphics dependency.
- `package_visual_startup_smoke` proves visual install can locate resources and shader root.
- `package_shader_lookup_smoke` proves installed shader artifacts resolve.
- `package_vulkan_dependency_smoke` proves platform runtime dependency diagnostics.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not make headless runtime targets require Vulkan, SDL, display access, or shader tools.
- Do not write generated SPIR-V beside committed shader source by default.

## Completion Criteria

- File `cmake/iggy3d_install.cmake` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
