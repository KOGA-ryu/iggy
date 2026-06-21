# src/render/vulkan/VulkanResult.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/vulkan/VulkanResult.cpp`

Purpose: Implement VkResult mapping for setup, WSI acquire/present, memory, shader, validation, and device-loss contexts.

## Build Position

Packet order: 4 - Vulkan bootstrap, device, validation, diagnostics
Owner module: `Vulkan result mapping`
File kind: `source`
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
- `docs/vulkan/vulkan_result_and_error_policy.md`

## Ownership

This file owns:
- Implement VkResult mapping for setup, WSI acquire/present, memory, shader, validation, and device-loss contexts.
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
- `mapVkResult`
- `mapAcquireResult`
- `mapPresentResult`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: create or use the Vulkan objects named by this file, emit receipt fields, return backend-neutral outcomes, and preserve runtime state. Skip/fail: optional lanes may skip before unsafe work; strict lanes fail on required Vulkan gaps, validation errors, or unsupported devices.

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
file_plan=src/render/vulkan/VulkanResult.cpp
packet_order=4
allowed_to_implement_code_now=false
backend=vulkan
device_name=
api_version=
enabled_instance_extensions=
enabled_device_extensions=
validation=enabled|disabled|unavailable
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
fallback_area=Vulkan_result_mapping
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: Vulkan object creation, device queries, validation setup, or GPU resource setup according to module scope.
Per-frame cost: no runtime mutation; command/sync/resource modules pay only the documented frame work.
Resize cost: bounded wait/recreate/teardown for swapchain-dependent objects when the module owns them.
GPU memory cost: named allocations only; budget receipt required once resources are created.

## Tests And Verification

Unit tests:
- `tests/unit/render_result_mapping_tests.cpp`
- `tests/unit/render_reason_code_tests.cpp`

Smoke tests:
- none for this file; covered by unit tests

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

Mapping table requirements:
- Maintain a single table from explicitly handled `VkResult` values to raw name, default outcome, and default reason.
- Context may refine outcome for `VK_TIMEOUT`, `VK_NOT_READY`, `VK_SUBOPTIMAL_KHR`, and `VK_ERROR_OUT_OF_DATE_KHR`.
- Device lost, incompatible driver, missing extension, missing feature, validation failure, and native window errors must have stable reason codes.

Required first mappings:
```text
VK_SUCCESS -> pass, vk_success
VK_NOT_READY -> skip_frame, vk_not_ready
VK_TIMEOUT -> skip_frame, vk_timeout
VK_SUBOPTIMAL_KHR -> skip_frame, swapchain_suboptimal
VK_ERROR_OUT_OF_DATE_KHR -> skip_frame, swapchain_out_of_date
VK_ERROR_DEVICE_LOST -> device_lost, device_lost
VK_ERROR_EXTENSION_NOT_PRESENT -> unsupported, unsupported_required_extension
VK_ERROR_FEATURE_NOT_PRESENT -> unsupported, unsupported_required_feature
VK_ERROR_LAYER_NOT_PRESENT -> unsupported, validation_layer_missing
VK_ERROR_INCOMPATIBLE_DRIVER -> unsupported, incompatible_driver
VK_ERROR_FORMAT_NOT_SUPPORTED -> unsupported, format_not_supported
VK_ERROR_NATIVE_WINDOW_IN_USE_KHR -> fatal_renderer_error, native_window_in_use
VK_ERROR_VALIDATION_FAILED -> validation_failure, validation_failed
```

Behavior rules:
- Renderer core returns outcomes and reason codes only.
- Test boundary maps unsupported setup to skip or fail according to lane strictness.
- Runtime and projection code never branch on `VkResult`.
- Human-readable text may include raw result name and call context.

Receipt fields:
```text
vk_result=<raw-name>
vk_context=<context-name>
render_outcome=<stable-outcome>
reason_code=<stable-reason>
setup_failure=true|false
device_lost=true|false
surface_invalid=true|false
```

Verification:
- Unit tests assert every table row.
- Unit tests assert `VK_ERROR_DEVICE_LOST` never maps to swapchain recreate.
- Unit tests assert setup unsupported never becomes a skip inside renderer core.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `src/render/vulkan/VulkanResult.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
