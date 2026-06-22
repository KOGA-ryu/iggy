# tests/smoke/package_vulkan_dependency_smoke.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/smoke/package_vulkan_dependency_smoke.cpp`

Purpose: Prove SDL3, Vulkan loader/ICD, MoltenVK, validation, and platform runtime dependency diagnostics.

## Build Position

Packet order: 7 - First visible room proof and packaging smoke
Owner module: `package smoke`
File kind: `smoke test`
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
- `docs/vulkan/vulkan_package_runtime_lookup.md`
- `docs/vulkan/packaging.md`

## Ownership

This file owns:
- Prove SDL3, Vulkan loader/ICD, MoltenVK, validation, and platform runtime dependency diagnostics.
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

Allowed includes: standard library, `src/render/**`, `src/render/vulkan/**`, Vulkan SDK headers, and test harness headers for Vulkan smoke tests.

Forbidden includes: runtime mutation internals, content package validators, save/load internals, old repo headers, and public exposure of raw `Vk*` handles outside Vulkan-owned declarations.

Vulkan headers are allowed because this file is under `src/render/vulkan/**` or is an explicitly named Vulkan smoke test.

SDL headers are allowed only when the smoke test exercises the platform shell; otherwise keep SDL behind app/platform files.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `package_vulkan_dependency_smoke executable`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: expose or test the backend-neutral renderer contract. Skip/fail: invalid frame/config inputs fail locally; Vulkan availability is not evaluated by this file.

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
file_plan=tests/smoke/package_vulkan_dependency_smoke.cpp
packet_order=7
allowed_to_implement_code_now=false
test_name=
platform_lane=
strict_vulkan=true|false
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

Fallback policy: optional lanes may skip with `result=skip` and a reason code before required Vulkan work begins. Strict lanes fail with `result=fail`. Render-pass fallback and software Vulkan shipping substitution are not allowed by this file plan.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=package_smoke
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: test harness setup plus any feature-gated renderer objects under test.
Per-frame cost: bounded by the smoke frame count and receipt collection.
Resize cost: exercised only in resize smoke files.
GPU memory cost: limited to named resources under the tested module.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/package_vulkan_dependency_smoke.cpp`

CTest labels:
```text
iggy3d;vulkan;smoke
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

Smoke purpose:
- Prove visual package dependency diagnostics distinguish Vulkan loader, physical device, WSI/display, SDL runtime, MoltenVK on macOS, validation layers, sync validation, and shader artifacts.
- This smoke does not certify GPU driver installation; it reports what the package can observe.

Platform dependency checks:
```text
macos: SDL3 runtime, Vulkan loader path, MoltenVK availability, portability path, shader root
linux: SDL3 runtime, system Vulkan loader, ICD/device visibility, X11 or Wayland WSI, shader root
windows: SDL3 runtime or DLL placement, Vulkan runtime loader, physical device visibility, Win32 WSI, shader root
```

macOS readiness probe fields:
```text
platform=macos
platform_lane=moltenvk
sdl3_target_available=true|false
sdl3_source=system|missing|disabled
vulkan_loader_found=true|false
vulkan_sdk_root=<absolute-path-or-empty>
vulkan_sdk_source=environment|default_path|cmake_discovery|not_found
vulkan_icd_path=<absolute-path-or-empty>
vulkan_icd_found=true|false
moltenvk_available=true|false|unavailable
glslc_path=<absolute-path-or-empty>
glslc_found=true|false
validation_layer_found=true|false|unavailable
sync_validation_available=true|false|unavailable
portability_enumeration_available=true|false|unavailable
portability_enumeration_required=true|false|unavailable
portability_subset_exposed=true|false|unavailable
strict_lane=true|false
result=pass|skip|fail
reason_code=<stable-lower-snake-case>
```

Optional dependency probes may exit `77` with `result=skip`. Strict probes fail
nonzero and must not convert missing SDL3, Vulkan loader, MoltenVK/ICD, or
required portability support into skip.

Required receipt fields:
```text
smoke=package_vulkan_dependency
platform=<macos|linux|windows>
package_mode=<build_tree_visual|installed_visual>
sdl_runtime_found=true|false|unavailable
vulkan_loader_found=true|false
physical_device_count=<integer-or-zero>
wsi_available=true|false|unavailable
moltenvk_available=true|false|not_applicable
validation_layer_found=true|false|unavailable
sync_validation_available=true|false|unavailable
shader_root=<absolute-path-or-empty>
shader_artifacts_found=true|false
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Skip/fail policy:
- Non-strict dependency smoke may skip missing graphics stack with receipt.
- Strict visual package dependency smoke fails on missing required SDL runtime, Vulkan loader, WSI/display, supported device, or shader artifacts.
- Missing validation layers fail only when validation is required by lane config.

Verification:
- Receipt distinguishes missing loader from missing device.
- macOS receipts distinguish MoltenVK availability from Vulkan source-of-truth behavior.
- Linux/Windows receipts identify native Vulkan proof lane.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `tests/smoke/package_vulkan_dependency_smoke.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
