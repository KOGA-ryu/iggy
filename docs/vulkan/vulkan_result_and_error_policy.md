# Vulkan Result And Error Policy

This document defines how `iggy3d` converts Vulkan `VkResult` values into renderer-owned outcomes, stable reason codes, diagnostics receipts, and smoke-test exit behavior.

Raw Vulkan result values are backend details. They must not become runtime truth, gameplay command legality, save/load state, replay state, camera truth, package validation, or projection semantics.

## Purpose

Define one result policy before implementation:

```text
central_vkresult_mapping=true
runtime_receives_vkresult=false
renderer_reason_codes=stable
optional_skip_policy=handled_at_test_boundary
strict_fail_policy=handled_at_test_boundary
recoverable_wsi_results=out_of_date_suboptimal_timeout_not_ready
device_loss=fatal_renderer_state_initially
runtime_mutation_on_error=false
```

This document narrows:

- [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md)
- [vulkan_device_loss_recovery.md](vulkan_device_loss_recovery.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [lifetime.md](lifetime.md)
- [sync_contract.md](sync_contract.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [debug_validation.md](debug_validation.md)
- [fallbacks.md](fallbacks.md)
- [packaging.md](packaging.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan `VkResult` reference page: https://docs.vulkan.org/refpages/latest/refpages/source/VkResult.html | exact result-code names, success/error category, WSI results, memory errors, validation failure, and unknown-error behavior |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html | final source of truth for exact command return behavior and valid usage |
| `vkAcquireNextImageKHR`: https://docs.vulkan.org/refpages/latest/refpages/source/vkAcquireNextImageKHR.html | acquire-specific success/failure mapping |
| `vkQueuePresentKHR`: https://docs.vulkan.org/refpages/latest/refpages/source/vkQueuePresentKHR.html | present-specific success/failure mapping and present-side semaphore consequences |
| Vulkan lost-device section: https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-lost-device | lost logical device behavior, finite wait behavior, child-object cleanup obligations |
| CTest `SKIP_RETURN_CODE`: https://cmake.org/cmake/help/latest/prop_test/SKIP_RETURN_CODE.html | mapping optional smoke skips to process exit `77` at the test boundary |
| Project swapchain, device-loss, memory, validation, and CI docs | local ownership rules, reason-code names, strict/optional lane policy, and runtime firewall |

Priority rule:

```text
vkresult_truth=Vulkan Specification and reference pages
wsi_context_truth=acquire_present_reference_pages
device_loss_truth=Vulkan lost-device spec section
test_exit_truth=CMake/CTest docs
project_behavior=iggy3d renderer boundary docs
```

## Scope

In scope:

- `VkResult` to backend-neutral renderer outcome mapping;
- call-site context rules;
- WSI acquire/present mapping;
- device-loss mapping;
- memory/allocation mapping;
- shader/pipeline mapping;
- debug/validation mapping;
- optional versus strict smoke behavior;
- reason codes;
- diagnostics receipt fields;
- runtime firewall tests.

Out of scope:

- implementing renderer code now;
- changing Vulkan API behavior;
- gameplay/runtime error taxonomy;
- save/replay error taxonomy;
- user-facing UI wording;
- automatic device recreation;
- platform-specific crash upload;
- broad exception policy for the whole project.

## Local File Surface

Likely future files:

```text
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/RendererApi.hpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/PipelinesShaders.cpp
tests/unit/render_result_mapping_tests.cpp
tests/unit/render_reason_code_tests.cpp
tests/unit/render_runtime_firewall_tests.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
tests/smoke/vulkan_device_lost_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| raw `VkResult` values | `src/render/vulkan/**` | runtime, content, projection, save |
| result conversion | `VulkanResult` Vulkan-private helper | gameplay semantics |
| backend-neutral renderer outcome | renderer API/diagnostics | save/replay truth |
| reason code strings | `RenderDiagnostics` / result mapper | driver behavior |
| process exit code | app/test harness | renderer core |
| optional versus strict lane choice | CMake/test harness/app config | runtime command legality |
| failure receipt | diagnostics module | source assets |

Rules:

- runtime/projection never see `VkResult`, `VK_*`, or Vulkan-specific recovery categories;
- renderer may report stable backend-neutral outcomes and reason codes;
- test harness maps renderer outcomes to process exit `0`, `1`, or `77`;
- renderer core must not know that CTest skip is `77`;
- result mapping must preserve original call site, raw result name, and high-level recovery category;
- renderer errors must not affect deterministic runtime hashes or replay results.

## Conceptual Renderer Outcomes

The implementation does not need this exact enum name, but it needs this shape:

```text
ok
skip_frame
poll_not_ready
recreate_swapchain
surface_lost
device_lost
unsupported
out_of_memory
validation_failure
pipeline_or_shader_failure
fatal_renderer_error
```

Required fields for any converted result:

```text
vulkan_call
vk_result
vk_result_string
result_context
renderer_outcome
reason_code
recoverable
frame_submission_allowed
present_allowed
swapchain_recreate_requested
surface_recreate_requested
device_shutdown_requested
runtime_state_touched
```

Rules:

- `VK_SUCCESS` maps to `ok`;
- non-fatal success/status codes still need call-site context;
- WSI results must not be mapped with a generic helper that ignores acquire versus present;
- device loss always wins over swapchain recovery;
- unknown or unmapped result values map to fatal renderer error with the raw integer preserved.

## General Result Mapping

General mapping before call-site overrides:

| Vulkan result | Renderer outcome | Default recovery | Reason code |
| --- | --- | --- | --- |
| `VK_SUCCESS` | `ok` | continue | `vk_success` |
| `VK_NOT_READY` | `poll_not_ready` | call-site dependent | `vk_not_ready` |
| `VK_TIMEOUT` | `skip_frame` or `fatal_renderer_error` | call-site and strictness dependent | `vk_timeout` |
| `VK_EVENT_SET` | internal status only | out of first renderer path unless events are adopted | `vk_event_set` |
| `VK_EVENT_RESET` | internal status only | out of first renderer path unless events are adopted | `vk_event_reset` |
| `VK_INCOMPLETE` | retry/query resize | call-site dependent | `vk_incomplete` |
| `VK_SUBOPTIMAL_KHR` | WSI accepted with recreate request | acquire/present dependent | `surface_suboptimal` |
| `VK_ERROR_OUT_OF_DATE_KHR` | `recreate_swapchain` | WSI recreate | `surface_out_of_date` |
| `VK_ERROR_SURFACE_LOST_KHR` | `surface_lost` | surface recovery or fail | `surface_lost` |
| `VK_ERROR_NATIVE_WINDOW_IN_USE_KHR` | `fatal_renderer_error` or `unsupported` | setup fail | `native_window_in_use` |
| `VK_ERROR_DEVICE_LOST` | `device_lost` | stop submissions and shutdown | `device_lost` |
| `VK_ERROR_OUT_OF_HOST_MEMORY` | `out_of_memory` | fail required path | `out_of_host_memory` |
| `VK_ERROR_OUT_OF_DEVICE_MEMORY` | `out_of_memory` | fail required resource or use explicit optional fallback | `out_of_device_memory` |
| `VK_ERROR_MEMORY_MAP_FAILED` | `out_of_memory` or `fatal_renderer_error` | fail mapped allocation path | `memory_map_failed` |
| `VK_ERROR_INITIALIZATION_FAILED` | `fatal_renderer_error` | setup fail | `initialization_failed` |
| `VK_ERROR_LAYER_NOT_PRESENT` | `unsupported` | optional skip or strict fail | `validation_layer_missing` |
| `VK_ERROR_EXTENSION_NOT_PRESENT` | `unsupported` | optional skip or strict fail | `unsupported_required_extension` |
| `VK_ERROR_FEATURE_NOT_PRESENT` | `unsupported` | optional skip or strict fail | `unsupported_required_feature` |
| `VK_ERROR_INCOMPATIBLE_DRIVER` | `unsupported` | optional skip or strict fail | `incompatible_driver` |
| `VK_ERROR_FORMAT_NOT_SUPPORTED` | `unsupported` | use documented fallback or fail | `format_not_supported` |
| `VK_ERROR_TOO_MANY_OBJECTS` | `fatal_renderer_error` | fail and diagnose leak/limit | `too_many_objects` |
| `VK_ERROR_FRAGMENTED_POOL` | `fatal_renderer_error` | fail descriptor/pool path | `fragmented_pool` |
| `VK_ERROR_OUT_OF_POOL_MEMORY` | `fatal_renderer_error` | fail descriptor/pool path | `out_of_pool_memory` |
| `VK_ERROR_FRAGMENTATION` | `fatal_renderer_error` | fail pool/allocator path | `fragmentation` |
| `VK_ERROR_INVALID_SHADER_NV` | `pipeline_or_shader_failure` | fail shader path | `invalid_shader` |
| `VK_PIPELINE_COMPILE_REQUIRED` | `pipeline_or_shader_failure` | fail unless pipeline-cache policy explicitly allows retry | `pipeline_compile_required` |
| `VK_ERROR_VALIDATION_FAILED` | `validation_failure` | strict fail | `validation_failed` |
| `VK_ERROR_UNKNOWN` | `fatal_renderer_error` | fail and preserve context | `vk_error_unknown` |

Rules:

- this table is not a replacement for per-command return-code checks;
- only include result codes the selected baseline can actually produce in first implementation tests;
- if a Vulkan call starts using a new extension result code, add it here before merging that call site;
- aliases should map to one stable reason code unless the extension-specific distinction matters for diagnostics.

## WSI Acquire Mapping

`vkAcquireNextImageKHR` must use acquire-specific handling:

| Acquire result | Frame submission | Present | Renderer outcome | Reason code |
| --- | --- | --- | --- | --- |
| `VK_SUCCESS` | allowed | allowed after submit | `ok` | `vk_success` |
| `VK_SUBOPTIMAL_KHR` | allowed | allowed after submit | `ok` with recreate soon | `surface_suboptimal` |
| `VK_ERROR_OUT_OF_DATE_KHR` | forbidden | forbidden | `recreate_swapchain` | `surface_out_of_date` |
| `VK_NOT_READY` | forbidden | forbidden | `skip_frame` or strict fail | `vk_not_ready` |
| `VK_TIMEOUT` | forbidden | forbidden | `skip_frame` or strict fail | `vk_timeout` |
| `VK_ERROR_SURFACE_LOST_KHR` | forbidden | forbidden | `surface_lost` | `surface_lost` |
| `VK_ERROR_DEVICE_LOST` | forbidden | forbidden | `device_lost` | `device_lost` |
| `VK_ERROR_OUT_OF_HOST_MEMORY` | forbidden | forbidden | `out_of_memory` | `out_of_host_memory` |
| `VK_ERROR_OUT_OF_DEVICE_MEMORY` | forbidden | forbidden | `out_of_memory` | `out_of_device_memory` |
| `VK_ERROR_VALIDATION_FAILED` | forbidden | forbidden | `validation_failure` | `validation_failed` |
| `VK_ERROR_UNKNOWN` | forbidden | forbidden | `fatal_renderer_error` | `vk_error_unknown` |

Rules:

- acquire failure before submit must not reset the current frame fence;
- acquire out-of-date is not device loss;
- acquire suboptimal is an accepted image and may render this frame;
- acquire not-ready/timeout is not normal for first implementation unless finite-timeout acquire is deliberately adopted;
- maximum-timeout acquire must be reviewed against platform forward-progress rules before use.

## WSI Present Mapping

`vkQueuePresentKHR` must use present-specific handling:

| Present result | Submitted work happened | Renderer outcome | Reason code |
| --- | --- | --- | --- |
| `VK_SUCCESS` | yes | `ok` | `vk_success` |
| `VK_SUBOPTIMAL_KHR` | yes | `ok` with recreate soon | `surface_suboptimal` |
| `VK_ERROR_OUT_OF_DATE_KHR` | yes | `recreate_swapchain` | `surface_out_of_date` |
| `VK_ERROR_SURFACE_LOST_KHR` | yes or unknown | `surface_lost` | `surface_lost` |
| `VK_ERROR_DEVICE_LOST` | yes or unknown | `device_lost` | `device_lost` |
| `VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT` | yes | first path fails unless feature adopted | `fullscreen_exclusive_lost` |
| `VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT` | yes | first path fails unless present-timing adopted | `present_timing_queue_full` |
| `VK_ERROR_OUT_OF_HOST_MEMORY` | no enqueue guarantee | `out_of_memory` | `out_of_host_memory` |
| `VK_ERROR_OUT_OF_DEVICE_MEMORY` | no enqueue guarantee | `out_of_memory` | `out_of_device_memory` |
| `VK_ERROR_VALIDATION_FAILED` | no reliable present | `validation_failure` | `validation_failed` |
| `VK_ERROR_UNKNOWN` | unknown | `fatal_renderer_error` | `vk_error_unknown` |

Rules:

- present only runs after a successful submit;
- present out-of-date schedules recreate at the next safe point;
- present suboptimal is accepted and should not fail strict smoke by itself;
- present device loss is not a swapchain recreate request;
- present result diagnostics must note that queue work was already submitted.

## Setup And Unsupported Mapping

Instance/device/surface setup failures are not frame skips.

| Setup condition | Optional smoke | Strict smoke | Reason code |
| --- | --- | --- | --- |
| no Vulkan loader | skip `77` | fail `1` | `vulkan_loader_missing` |
| no usable ICD/driver | skip `77` | fail `1` | `incompatible_driver` |
| required instance extension missing | skip `77` | fail `1` | `unsupported_required_extension` |
| required device extension missing | skip `77` | fail `1` | `unsupported_required_extension` |
| required feature missing | skip `77` | fail `1` | `unsupported_required_feature` |
| validation layer missing when optional | continue without layer and report | fail only if required | `validation_layer_missing` |
| validation layer missing when required | skip `77` or fail by lane | fail `1` | `validation_layer_missing` |
| required surface extension missing | skip `77` | fail `1` | `surface_extension_missing` |
| native window already in use | fail `1` | fail `1` | `native_window_in_use` |

Rules:

- optional smoke may skip only before renderer work begins;
- strict lanes must fail missing required setup;
- shipping platform validation must not pass because Vulkan was unavailable;
- setup failures must not start or alter runtime gameplay state.

## Memory And Resource Mapping

Memory results must align with [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md):

| Condition | Required resource | Optional resource | Reason code |
| --- | --- | --- | --- |
| `VK_ERROR_OUT_OF_HOST_MEMORY` | fail | fail unless explicitly documented optional host path | `out_of_host_memory` |
| `VK_ERROR_OUT_OF_DEVICE_MEMORY` | fail | fallback only if fallback resource is predeclared | `out_of_device_memory` |
| VMA allocation failure | fail required allocation | fallback optional asset/material allocation | `vma_allocation_failed` |
| mapping failure | fail upload/resource path | fallback only for optional debug readback | `memory_map_failed` |
| budget exceeded by policy | fail or degrade only by explicit policy | use fallback material/texture | `memory_budget_exceeded` |

Rules:

- fallback is allowed only for optional visual resources, not required swapchain/depth/frame resources;
- fallback must be visible in diagnostics;
- allocation names must appear in receipts for failed VMA paths;
- texture/material growth must not hide repeated out-of-memory failures under generic fallback.

## Shader And Pipeline Mapping

Shader/pipeline failures are renderer build/runtime integration failures, not runtime command failures.

| Condition | Renderer outcome | Reason code |
| --- | --- | --- |
| shader compiler missing in build path | unsupported or strict fail at build/test boundary | `shader_compiler_missing` |
| generated SPIR-V missing | `pipeline_or_shader_failure` | `spirv_missing` |
| stale SPIR-V detected | `pipeline_or_shader_failure` | `spirv_stale` |
| shader module create fails | `pipeline_or_shader_failure` | `shader_module_create_failed` |
| pipeline layout mismatch | `pipeline_or_shader_failure` | `pipeline_layout_mismatch` |
| pipeline create fails | `pipeline_or_shader_failure` | `pipeline_create_failed` |
| `VK_PIPELINE_COMPILE_REQUIRED` | `pipeline_or_shader_failure` unless retry policy exists | `pipeline_compile_required` |

Rules:

- first-room shaders are required for strict visual lanes;
- null renderer lanes do not require shader artifacts;
- shader failures must print shader source identity, generated SPIR-V path, entry point, and pipeline variant key when available;
- renderer must never patch runtime/projection data to work around shader mismatch.

## Validation Mapping

Validation results and messages are development/test proof, not runtime authority.

Policy:

```text
validation_message_in_strict_lane=fail_if_error_severity
validation_message_in_optional_lane=diagnose_and_fail_if_renderer_bug
vk_error_validation_failed=strict_fail
sync_validation_error=strict_fail
validation_layer_missing=lane_dependent
```

Rules:

- validation errors must include message ID/name when available;
- sync validation errors must include frame index and command label when available;
- validation layer absence is not the same as validation passing;
- a strict lane requiring validation must fail if validation cannot be enabled;
- validation failures must not mutate runtime or replay state.

## Optional Versus Strict Exit Policy

Renderer result mapping does not return process exit codes directly.

Process exit mapping belongs in the smoke app/test wrapper:

| Renderer/test condition | Optional lane exit | Strict lane exit |
| --- | --- | --- |
| pass | `0` | `0` |
| unsupported environment before renderer work | `77` | `1` |
| missing optional validation layer | `0` with receipt or `77` if lane requires optional proof | `1` if required |
| missing required Vulkan feature | `77` | `1` |
| renderer bug | `1` | `1` |
| validation error | `1` | `1` |
| device lost during attempted render | `1` | `1` |
| runtime hash changed | `1` | `1` |

Hard rules:

- only optional smoke tests may return `77`;
- headless runtime tests must not skip because Vulkan is missing;
- strict platform validation must not skip missing Vulkan;
- renderer core must report `unsupported`; the test boundary decides whether that becomes skip or fail;
- all exits after a Vulkan attempt should include a receipt.

## Diagnostics Receipt Fields

Any attempted Vulkan smoke failure should emit:

```text
receipt_version
app_name
platform
lane_name
strict_lane
optional_lane
vulkan_enabled
validation_enabled
sync_validation_enabled
vulkan_call
vk_result
vk_result_string
vk_result_integer
result_context
renderer_outcome
reason_code
recoverable
frame_index
image_index
swapchain_generation
swapchain_recreate_requested
surface_lost
device_lost
out_of_memory
allocation_name
pipeline_name
shader_name
runtime_state_touched
runtime_hash_before
runtime_hash_after
exit_code
```

Rules:

- unknown result values must include the raw integer;
- successful smoke should include enough fields to prove no hidden skip occurred;
- if no Vulkan loader is present, fields that require a `VkInstance` should be marked unavailable rather than fabricated;
- runtime hash fields are required for any lane that also starts runtime simulation.

## Reason Code Registry

Initial result-related reason codes:

```text
vk_success
vk_not_ready
vk_timeout
vk_event_set
vk_event_reset
vk_incomplete
vk_unknown_result
vk_error_unknown
result_context_missing
result_mapping_missing
vulkan_loader_missing
unsupported_required_extension
unsupported_required_feature
unsupported_required_layer
validation_layer_missing
surface_extension_missing
incompatible_driver
format_not_supported
native_window_in_use
surface_out_of_date
surface_suboptimal
surface_lost
fullscreen_exclusive_lost
present_timing_queue_full
out_of_host_memory
out_of_device_memory
memory_map_failed
memory_budget_exceeded
optional_resource_fallback
required_resource_failed
vma_allocation_failed
device_lost
validation_failed
sync_validation_failed
shader_compiler_missing
spirv_missing
spirv_stale
shader_module_create_failed
pipeline_layout_mismatch
pipeline_create_failed
pipeline_compile_required
strict_lane_missing_requirement
optional_lane_skipped
runtime_result_leak
runtime_hash_changed
```

Rules:

- reason codes are stable machine-readable strings;
- human-readable messages may change, reason codes should not churn;
- every reason code used by smoke tests must be documented here or in a narrower Vulkan doc;
- duplicate meanings should be consolidated before code lands.

## Runtime Firewall

Forbidden:

```text
src/runtime/** includes Vulkan headers
src/content/** includes Vulkan headers
src/projection/** includes Vulkan headers
src/runtime/save/** includes Vulkan headers
runtime code branches on VkResult
runtime code branches on VK_* constants
projection code branches on VkResult
save/replay code serializes Vulkan result values
renderer errors mutate deterministic gameplay state
```

Allowed:

```text
src/render/vulkan/** maps VkResult
src/render/** receives backend-neutral renderer outcomes
apps/iggy3d_visual_demo/** maps renderer outcome to app exit/logging
tests/smoke/vulkan_* maps renderer outcome to CTest exit policy
```

Firewall scan:

```sh
rg -n "VkResult|VK_SUCCESS|VK_ERROR_|VK_SUBOPTIMAL|VK_TIMEOUT|VK_NOT_READY" \
  src/runtime src/content src/projection src/runtime/save \
  apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no production runtime/content/projection/save ownership leak
```

## Test Gates

Unit tests should prove:

- every explicitly used `VkResult` maps to one renderer outcome and one reason code;
- unknown result values map to `fatal_renderer_error` and `vk_unknown_result`;
- call-site context is required for WSI results;
- acquire out-of-date maps to swapchain recreate and does not allow submit;
- acquire suboptimal allows draw/present and requests recreate soon;
- present out-of-date records that submit already happened;
- device lost never maps to swapchain recreate;
- required-resource out-of-memory fails;
- optional-resource fallback is allowed only when the resource policy permits it;
- strict lanes never convert missing required Vulkan support into skip;
- optional lanes convert unsupported environment to `77` only at the test wrapper;
- runtime hash before/after is unchanged by renderer errors.

Smoke tests should prove:

- optional Vulkan smoke prints skip receipt and exits `77` when Vulkan is unavailable;
- strict Vulkan smoke exits `1` when Vulkan is unavailable;
- successful Vulkan smoke prints `vk_success` or a documented nonfatal WSI reason;
- validation-required smoke fails if validation cannot be enabled;
- device-lost or forced-failure test path emits `device_lost` without runtime mutation.

## Acceptance Criteria

This policy is ready for implementation when:

- [ ] every Vulkan call site in first renderer file plans has an assigned result context;
- [ ] `VulkanResult` private helper is the only raw result converter;
- [ ] `RenderDiagnostics` has stable fields for raw result, outcome, reason code, and context;
- [ ] optional versus strict exit mapping is implemented only in app/test wrapper code;
- [ ] unit tests cover all first-room result codes;
- [ ] smoke receipts include the required fields above;
- [ ] runtime firewall scan is clean;
- [ ] runtime replay/hash tests prove renderer errors do not mutate deterministic state.

## Non-Goals For First Renderer Pass

Do not add:

- generic exception policy for all `iggy3d`;
- automatic logical device recreation after device loss;
- fullscreen-exclusive result handling beyond a documented failure;
- present-timing extension behavior;
- event-based renderer flow;
- pipeline binary policy;
- vendor-specific crash dump upload;
- runtime recovery commands triggered directly by renderer failures.
