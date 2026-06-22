# tests/smoke/vulkan_device_smoke.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/smoke/vulkan_device_smoke.cpp`

Purpose: Prove Vulkan instance/device/surface/queue/feature selection and unsupported-device receipts.

## Build Position

Packet order: 4 - Vulkan bootstrap, device, validation, diagnostics
Owner module: `device smoke`
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
- `docs/vulkan/device_selection.md`
- `docs/vulkan/vulkan_unsupported_device_policy.md`

## Ownership

This file owns:
- Prove Vulkan instance/device/surface/queue/feature selection and unsupported-device receipts.
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
- `vulkan_device_smoke executable`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: expose or test the backend-neutral renderer contract. Skip/fail: invalid frame/config inputs fail locally; Vulkan availability is not evaluated by this file.

Platform behavior:
macOS/MoltenVK: report `platform=macos` and `platform_lane=moltenvk` when Vulkan is attempted; MoltenVK portability details are diagnostics, not cross-platform law.

The macOS dependency readiness probe precedes this device smoke. It reports
system SDL3, Vulkan loader, SDK root, ICD path, MoltenVK, `glslc`, validation
layers, sync validation, and portability enumeration without creating a
swapchain or submitting a Vulkan frame.
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
file_plan=tests/smoke/vulkan_device_smoke.cpp
packet_order=4
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
fallback_area=device_smoke
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
- `tests/smoke/vulkan_device_smoke.cpp`

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

## Packet 4 Detailed Contract

Smoke purpose:
- Prove the host can create the Packet 4 Vulkan bootstrap surface: instance, surface, physical-device selection, logical device, queue retrieval, validation plan, function loading, receipt, and clean teardown.
- This smoke does not create swapchain images, command buffers, pipelines, buffers, textures, or first-room draw calls.

Inputs:
```text
IGGY3D_REQUIRE_VULKAN_SMOKE=0|1
IGGY3D_ALLOW_SOFTWARE_VULKAN=0|1
IGGY3D_REQUIRE_VALIDATION_LAYERS=0|1
IGGY3D_REQUIRE_SYNC_VALIDATION=0|1
IGGY3D_DIAGNOSTICS_DIR=<path>
```

Skip/fail policy:
- Non-strict lane may skip before logical device creation when loader, display, WSI, ICD, required extension, or supported device is unavailable.
- Strict lane fails on the same missing requirements.
- Once logical device creation succeeds, teardown failures are failures in both lanes.
- Validation-required lanes fail when validation cannot be enabled.

Required receipt fields:
```text
smoke=vulkan_device
strict_lane=true|false
platform=<macos|linux|windows>
backend=vulkan
loader_strategy=<linked_loader|manual_loader>
physical_device_count=<integer>
selected_physical_device_index=<integer-or-none>
device_name=<name-or-none>
device_type=<discrete|integrated|cpu|virtual|other|none>
api_version_selected=<major.minor.patch-or-none>
driver_version=<integer-or-none>
graphics_queue_family=<integer-or-none>
present_queue_family=<integer-or-none>
enabled_instance_extensions=<comma-separated-names>
enabled_device_extensions=<comma-separated-names>
dynamic_rendering_source=<core_1_3|khr_extension|unavailable>
validation=<enabled|disabled|unavailable>
sync_validation=<enabled|disabled|unavailable>
function_loading_clean=true|false
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Exit behavior:
- Pass exits `0`.
- Non-strict unsupported environment exits `77`.
- Strict missing requirement exits nonzero.
- Any receipt-schema failure exits nonzero.

Verification:
- The smoke asserts no runtime save/replay/hash files are touched.
- The smoke writes diagnostics receipt under configured diagnostics directory.
- The smoke tears down all Packet 4 Vulkan objects before process exit.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `tests/smoke/vulkan_device_smoke.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
