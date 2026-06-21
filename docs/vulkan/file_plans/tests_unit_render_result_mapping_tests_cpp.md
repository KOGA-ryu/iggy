# tests/unit/render_result_mapping_tests.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/unit/render_result_mapping_tests.cpp`

Purpose: Prove VkResult mapping to renderer outcomes and reason codes for setup, WSI, memory, validation, shader, and device loss.

## Build Position

Packet order: 4 - Vulkan bootstrap, device, validation, diagnostics
Owner module: `result tests`
File kind: `unit test`
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
- Prove VkResult mapping to renderer outcomes and reason codes for setup, WSI, memory, validation, shader, and device loss.
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

Allowed includes: standard library, core math/id value types, projection value types where the frame contract permits them, and `src/render/**` backend-neutral headers.

Forbidden includes: Vulkan SDK headers, `Vk*` types, `VK_*` constants, SDL/window headers, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in backend-neutral renderer, null renderer, and non-Vulkan unit test files.

SDL/window headers are forbidden outside app/platform glue and Vulkan platform smoke tests.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `VkResult mapping assertions`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Backend-neutral values are owned by renderer API callers for the call duration unless copied into diagnostics. Runtime/projection data remains authoritative outside the renderer. Diagnostics receipts are renderer-owned output. No GPU, window, or Vulkan object lifetime exists in this file.

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
file_plan=tests/unit/render_result_mapping_tests.cpp
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

Fallback policy: optional smoke may skip with `result=skip`; strict smoke fails with `result=fail`; pass requires the documented receipt.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=result_tests
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
- `tests/unit/render_result_mapping_tests.cpp`

Smoke tests:
- none for this file; covered by unit tests

CTest labels:
```text
iggy3d;render;unit
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

Test groups:
- `success_maps_to_pass`
- `not_ready_maps_to_skip_frame`
- `timeout_maps_by_context`
- `out_of_date_maps_to_swapchain_recreate_signal`
- `suboptimal_maps_to_diagnosed_nonfatal_result`
- `device_lost_maps_to_device_lost`
- `missing_extension_maps_to_unsupported`
- `missing_feature_maps_to_unsupported`
- `missing_layer_maps_to_validation_layer_missing`
- `incompatible_driver_maps_to_unsupported`
- `validation_failed_maps_to_validation_failure`
- `unmapped_result_fails_table_audit`

Fixture rules:
- Tests may include Vulkan headers because they test Vulkan-private mapping.
- Tests do not create Vulkan instances or devices.
- Tests do not require display, SDL, validation layers, shader compiler, or runtime session.

Assertions:
- Every explicitly handled raw result returns a stable raw result name.
- Every mapping returns a backend-neutral `RenderOutcome`.
- Every mapping returns a stable `RenderReason`.
- Renderer core never converts unsupported setup to CTest skip.
- `VK_ERROR_DEVICE_LOST` never maps to swapchain recreate.

Receipt assertions:
```text
vk_result=<raw-name>
vk_context=<context-name>
render_outcome=<stable-outcome>
reason_code=<stable-reason>
setup_failure=<true|false>
device_lost=<true|false>
surface_invalid=<true|false>
```

CTest behavior:
- Label: `iggy3d;render;unit`
- Always runs on macOS, Linux, and Windows without graphics dependencies.
- Fails on missing mapping, unstable reason, or renderer-core skip mapping.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.

## Completion Criteria

- File `tests/unit/render_result_mapping_tests.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
