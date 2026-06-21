# Vulkan Unsupported Device Policy

This document defines what happens when Vulkan exists on a machine but the selected device, driver, API version, features, extensions, queues, surface support, or formats do not meet the `iggy3d` Vulkan baseline.

Unsupported device handling is renderer/platform policy. It must not become runtime truth, command legality, content validation, camera truth, save/load truth, replay truth, or projection semantics.

## Purpose

Define one outcome contract:

```text
vulkan_loader_present_but_device_unsuitable=unsupported_device
unsupported_is_diagnostic_not_runtime_failure=true
optional_vulkan_smoke=skip_with_receipt_when_allowed
strict_vulkan_smoke=fail_with_receipt
visual_app=show_user_facing_message
headless_runtime=unaffected
null_renderer_fallback=allowed_only_when_requested_or_non_visual
runtime_state_mutation_from_unsupported_device=false
```

This document narrows:

- [vulkan_feature_baseline.md](vulkan_feature_baseline.md)
- [device_selection.md](device_selection.md)
- [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [vulkan_package_runtime_lookup.md](vulkan_package_runtime_lookup.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)
- [platform_matrix.md](platform_matrix.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Guide querying extensions/features: https://docs.vulkan.org/guide/latest/querying_extensions_features.html | required query-before-enable model for properties, features, extensions, limits, and formats |
| Vulkan Guide versions/porting: https://docs.vulkan.org/guide/latest/versions.html | target API version checks and version-specific feature structures |
| Vulkan Specification features chapter: https://docs.vulkan.org/spec/latest/chapters/features.html | feature support, enablement, and unsupported feature behavior |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/ | exact API, feature, extension, format, queue, and result behavior |
| Vulkan Tutorial physical devices and queue families | first-pass physical-device selection shape |
| Project feature baseline/device selection docs | local `iggy3d` required gates and deferred optional feature policy |
| Project CI/result docs | optional skip versus strict fail mapping |

Priority rule:

```text
exact_vulkan_behavior=Vulkan_Specification_and_Registry
query_model=Vulkan_Guide
project_required_gates=iggy3d_feature_baseline_and_device_selection
skip_fail_policy=iggy3d_ci_and_result_docs
runtime_truth=iggy3d_runtime_docs_not_renderer_capability
```

## Scope

In scope:

- unsupported API version;
- unsupported required instance extension;
- unsupported required device extension;
- unsupported required device feature;
- unsupported queue family/surface support;
- unsupported swapchain support;
- unsupported color/depth/texture formats;
- unsupported limits;
- unsupported Apple portability requirements;
- optional versus required feature absence;
- optional skip versus strict fail;
- user-facing message text;
- diagnostics receipts and reason codes;
- package/smoke behavior.

Out of scope:

- implementing device selection now;
- changing the Vulkan feature baseline;
- automatic GPU driver installation;
- GPU vendor-specific workaround database;
- render-pass fallback implementation;
- software Vulkan as shipping substitute;
- save/replay error handling;
- headless runtime acceptance changes.

## Local File Surface

Likely future files:

```text
src/render/RendererApi.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/VulkanFeatureSupport.hpp
src/render/vulkan/VulkanFeatureSupport.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
apps/iggy3d_visual_demo/main.cpp
tests/unit/render_unsupported_device_policy_tests.cpp
tests/unit/render_device_rejection_reason_tests.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_feature_baseline_smoke.cpp
tests/smoke/vulkan_optional_unsupported_smoke.cpp
tests/smoke/vulkan_strict_unsupported_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| device capability queries | Vulkan device-selection module | gameplay semantics |
| unsupported gate classification | Vulkan feature-support/result policy | runtime error taxonomy |
| skip/fail decision | app/test harness strictness | renderer core truth |
| user-facing message | visual app/startup layer | save/replay state |
| device rejection receipt | diagnostics | gameplay command legality |
| fallback-to-null decision | app config/test harness | hidden renderer downgrade |

Rules:

- runtime/content/projection/save code must not inspect device capability gates;
- renderer must not silently fall back to null renderer when `vulkan` is explicitly required;
- app/test harness maps unsupported renderer outcome to skip, fail, or user message;
- unsupported Vulkan never changes runtime hash or replay result;
- all rejected candidate devices must be diagnosable at least in verbose smoke output.

## Classification

Use these outcome classes:

```text
supported
unsupported_required_api
unsupported_required_instance_extension
unsupported_required_device_extension
unsupported_required_feature
unsupported_required_queue
unsupported_required_surface
unsupported_required_format
unsupported_required_limit
unsupported_required_portability
unsupported_optional_feature
unsupported_deferred_feature
environment_missing_loader_or_icd
environment_no_physical_devices
```

Rules:

- unsupported required gates reject the Vulkan backend for that lane;
- unsupported optional features do not reject the first-room backend unless the current phase promotes them to required;
- deferred feature absence is diagnostic only;
- missing loader/ICD is environment/runtime dependency failure, not unsupported selected device;
- no physical devices is environment/platform failure and may still produce an unsupported-style user message.

## Required Versus Optional Gates

Required first visual gates:

```text
loader_available
instance_created
surface_created
physical_device_count_gt_zero
device_api_version_meets_baseline_or_accepted_compatibility_path
graphics_queue_available
present_queue_for_surface_available
VK_KHR_swapchain_available
surface_formats_available
present_modes_available
dynamic_rendering_available
depth_format_available
push_constant_limit_at_least_64
portability_requirements_handled_on_apple
```

Optional or deferred gates:

```text
timeline_semaphore_available
synchronization2_available
descriptor_indexing_available
anisotropy_available
sampled_image_support_before_texture_growth
sampler_support_before_texture_growth
dedicated_transfer_queue_available
software_vulkan_device_available
```

Rules:

- optional/deferred feature absence must print availability and enabled state;
- material/texture packets may promote sampled image/sampler/descriptor gates from optional to required;
- sync2/timeline absence does not block first-room unless a later accepted packet changes the baseline;
- dynamic rendering absence blocks first-room unless an explicit render-pass fallback decision is accepted.

## Decision Table

| Context | Unsupported required gate | Optional feature missing | Message target | Process result |
| --- | --- | --- | --- | --- |
| headless runtime | ignored because Vulkan not initialized | ignored | none | pass/fail by runtime only |
| null renderer replay | ignored because Vulkan not initialized | ignored | diagnostics only if requested | pass/fail by replay |
| optional Vulkan smoke | skip with receipt | pass with diagnostic | stdout/CTest log | exit `77` for unsupported required gate |
| strict Vulkan smoke | fail with receipt | pass unless promoted | stdout/CTest log | exit `1` for unsupported required gate |
| visual demo default | fail visual startup with user-facing message | continue with diagnostic | stderr/window/message layer | app failure unless fallback requested |
| visual demo `--renderer=auto` non-strict | may select null only if visual proof not required | continue with diagnostic | stderr/receipt | pass only if app mode allows null |
| installed visual package smoke | fail if visual package requires Vulkan | continue with diagnostic | receipt/log | exit `1` in strict package lane |

Rules:

- optional smoke skip still prints `result=skip`;
- strict smoke never converts unsupported required gates into skip;
- user-facing visual app messages should avoid raw Vulkan jargon in the first sentence;
- detailed receipt still includes exact Vulkan gate names for developers.

## Candidate Rejection Flow

Device selection should evaluate candidates in this order:

```text
1. Confirm loader and instance path.
2. Create/receive surface when the lane requires presentation.
3. Enumerate physical devices.
4. For each candidate, collect properties, API version, limits, features, extensions, queues, surface support, formats, present modes.
5. Apply required gates.
6. Record every rejection reason for rejected candidates.
7. Score only candidates that pass required gates.
8. Select the best passing candidate.
9. If no candidate passes, report unsupported Vulkan backend with aggregate reasons.
```

Rules:

- do not call `vkCreateDevice` with unsupported requested features/extensions when pre-query can reject cleanly;
- if `vkCreateDevice` still returns `VK_ERROR_FEATURE_NOT_PRESENT` or `VK_ERROR_EXTENSION_NOT_PRESENT`, map it to unsupported and preserve the requested feature/extension if known;
- one rejected candidate must not hide another passing candidate;
- user-selected device index cannot bypass required gates.

## API Version Policy

Unsupported API version cases:

```text
instance_api_version_too_low
device_api_version_too_low
vulkan_1_3_unavailable_and_1_2_extension_path_unapproved
```

Rules:

- SDK/header version is not runtime support;
- instance version and selected physical device API version must be queried and printed;
- Vulkan 1.3 is preferred;
- Vulkan 1.2 plus required extensions is allowed only through the accepted compatibility path;
- a lower API path must print `compatibility_path=true` and name the enabled extensions.

## Extension Policy

Required instance extension failures:

```text
platform_surface_extension_missing
debug_utils_missing_when_strict_debug_required
portability_enumeration_missing_when_required
```

Required device extension failures:

```text
swapchain_extension_missing
dynamic_rendering_extension_missing_when_compat_path_required
portability_subset_missing_when_advertised_required
```

Rules:

- missing platform WSI extensions fail any visual surface lane;
- missing `VK_KHR_swapchain` fails any presentation lane;
- missing debug utils fails only when strict debug/validation policy requires it;
- missing optional/deferred extensions must not fail first-room smoke;
- extension names may appear in diagnostics, but public renderer API should use backend-neutral reason codes.

## Feature And Limit Policy

Required first-room feature/limit failures:

```text
dynamic_rendering_feature_missing
push_constant_limit_too_small
depth_attachment_support_missing
color_attachment_support_missing
```

Optional/deferred examples:

```text
timeline_semaphore_missing
synchronization2_missing
descriptor_indexing_missing
sampler_anisotropy_missing
dedicated_transfer_queue_missing
```

Rules:

- enabling unsupported features is a bug; reject before logical-device creation when possible;
- required feature absence maps to `unsupported_required_feature`;
- required limit failure maps to `unsupported_required_limit`;
- optional feature absence appears in receipts as `available=false` and `enabled=false`;
- deferred features cannot be used in code paths until a later packet promotes and gates them.

## Format Policy

Required format failures:

```text
no_supported_surface_formats
swapchain_format_policy_no_candidate
depth_format_missing
readback_format_unavailable_when_screenshot_required
texture_format_missing_when_texture_phase_required
```

Rules:

- surface format failure blocks presentation;
- depth format failure blocks first-room 3D proof;
- screenshot/readback format failure blocks screenshot smoke only when screenshot proof is required;
- texture format failure blocks texture/material smoke only after texture/material phase promotes that format;
- format fallback ladders must come from format policy docs, not ad hoc device-selection code.

## Surface And Queue Policy

Required WSI failures:

```text
no_graphics_queue
no_present_queue_for_surface
surface_present_support_missing
no_surface_formats
no_present_modes
surface_creation_failed
```

Rules:

- graphics queue without present support is not enough for visual presentation;
- a separate present queue is acceptable if sharing/sync policy can support it;
- no present queue rejects the device for visual surface lanes;
- headless/offscreen future lanes may define a different surface requirement, but first visual proof requires presentation.

## Apple Portability Policy

Apple/MoltenVK unsupported cases:

```text
portability_enumeration_required_but_missing
portability_subset_required_but_not_enabled
moltenvk_runtime_missing
apple_surface_path_unavailable
required_feature_missing_on_portability_device
```

Rules:

- MoltenVK is a portability target, not the cross-platform Vulkan source of truth;
- Apple lane must print portability enumeration and portability subset state;
- missing Apple runtime packaging is a package/runtime failure;
- missing required Vulkan features on the visible Apple portability device is unsupported device;
- do not lower the global renderer baseline around Apple quirks without a documented fallback decision.

## User-Facing Message Policy

User-facing message first sentence should be plain:

```text
This machine cannot run the Vulkan visual renderer required by this build.
```

Then add one specific reason:

```text
Reason: the selected graphics device does not support dynamic rendering.
Reason: no Vulkan device with presentation support was found.
Reason: the Vulkan runtime is installed, but no compatible graphics device passed the renderer requirements.
Reason: this Apple Vulkan runtime is missing required portability support.
```

Then add one action hint:

```text
Update your graphics driver or use a machine with Vulkan 1.3 support.
Install the platform Vulkan runtime and GPU driver for your graphics device.
Run the headless runtime demo instead; it does not require Vulkan.
Use an optional software Vulkan lane only for development diagnostics, not shipping proof.
```

Rules:

- user message must not expose raw `Vk*` structs in the first sentence;
- developer receipt may include exact Vulkan extension/feature names;
- visual demo should return failure when Vulkan was explicitly requested and unsupported;
- headless tools should not show unsupported Vulkan messages unless the user requested Vulkan.

## Receipt Fields

Unsupported-device receipts should include:

```text
result=skip|fail
reason_code=
renderer_outcome=unsupported
unsupported_class=
strict_vulkan=true|false
optional_skip_allowed=true|false
user_message=
developer_message=
vulkan_loader=found|missing|unavailable
vulkan_icd_status=found|missing|unavailable
instance_api_version=
requested_api_version=
physical_device_count=
candidate_device_count=
selected_device_index=unavailable
selected_device_name=unavailable
rejected_device_count=
primary_rejection_reason=
all_rejection_reasons=
missing_instance_extensions=
missing_device_extensions=
missing_features=
missing_formats=
missing_limits=
missing_queues=
compatibility_path_attempted=true|false
compatibility_path_allowed=true|false
null_renderer_fallback_selected=true|false
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
```

Rules:

- `user_message` is stable enough for app display but not a parser contract;
- `reason_code` is the parser contract;
- `all_rejection_reasons` may be comma-separated stable reason codes;
- runtime hash fields are required only when a runtime session was actually created for the smoke test;
- no raw Vulkan handles appear in receipts.

## Reason Codes

Use stable reason codes:

```text
unsupported_device
unsupported_required_api
unsupported_required_instance_extension
unsupported_required_device_extension
unsupported_required_feature
unsupported_required_queue
unsupported_required_surface
unsupported_required_format
unsupported_required_limit
unsupported_required_portability
optional_feature_unavailable
deferred_feature_unavailable
vulkan_loader_unavailable
vulkan_no_physical_devices
vulkan_instance_version_too_low
vulkan_device_version_too_low
vulkan_no_graphics_queue
vulkan_no_present_queue
vulkan_swapchain_extension_missing
vulkan_surface_support_missing
vulkan_surface_format_missing
vulkan_present_mode_missing
vulkan_dynamic_rendering_missing
vulkan_depth_format_missing
vulkan_push_constant_limit_too_small
vulkan_portability_enumeration_missing
vulkan_portability_subset_missing
vulkan_render_pass_fallback_unapproved
unsupported_device_strict_fail
unsupported_device_optional_skip
```

Rules:

- the primary `reason_code` should be as specific as possible;
- aggregate `unsupported_device` may be used only when multiple candidate failures are summarized and a primary reason is also printed;
- optional feature absence should not use a failure reason code unless the test specifically checks optional availability;
- strict/optional suffix codes are lane outcome hints, not Vulkan capability facts.

## Tests

Expected future tests:

```text
tests/unit/render_unsupported_device_policy_tests.cpp
tests/unit/render_device_rejection_reason_tests.cpp
tests/unit/render_strict_optional_mapping_tests.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_feature_baseline_smoke.cpp
tests/smoke/vulkan_optional_unsupported_smoke.cpp
tests/smoke/vulkan_strict_unsupported_smoke.cpp
```

Unit tests should cover:

- required gate maps to unsupported;
- optional missing feature maps to diagnostic-only;
- optional smoke maps unsupported required gate to skip;
- strict smoke maps unsupported required gate to fail;
- visual app user message avoids raw Vulkan-first wording;
- null renderer fallback requires explicit app/test mode.

Smoke tests should cover:

- receipt emitted when no suitable device is found;
- strict unsupported exits `1`;
- optional unsupported exits `77`;
- headless runtime still passes without Vulkan;
- rejected candidate reasons are present in verbose diagnostics.

## Acceptance Criteria

This policy is ready for implementation planning when:

- unsupported device is distinguished from missing loader/ICD;
- required, optional, and deferred capability gates are separated;
- optional smoke skip and strict smoke fail behavior are explicit;
- user-facing messages are plain and actionable;
- developer receipts include exact missing API/features/extensions/formats/queues;
- null renderer fallback is never silent for explicit Vulkan visual proof;
- Apple portability unsupported cases are separated from Linux/Windows native Vulkan cases;
- software Vulkan is not treated as shipping proof;
- runtime hash/replay invariance remains mandatory when runtime is involved;
- failure reason codes are named before tests are written.

