# Vulkan Device Selection

This document defines physical device selection, logical device creation, queue family selection, required extensions, required features, portability handling, and diagnostics for `iggy3d`.

Device selection is renderer-owned. It decides which GPU/API surface the Vulkan backend can use. It does not decide gameplay truth, command legality, camera mode truth, save data, replay determinism, or projection semantics.

## Purpose

Define the first device-selection contract:

```text
enumerate instance extensions and layers
create instance with platform-required extensions
create surface
enumerate physical devices
query device properties/features/extensions
query queue families and surface present support
reject devices missing required gates
score remaining candidates
select physical device
create logical device and queues
print diagnostics receipt
```

This document sits upstream of [swapchain_contract.md](swapchain_contract.md), [sync_contract.md](sync_contract.md), [command_recording.md](command_recording.md), [shader_pipeline.md](shader_pipeline.md), and [resource_model.md](resource_model.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for exact device, queue, feature, extension, and portability behavior.
- Khronos Vulkan Tutorial instance/device/surface/swapchain chapters for first implementation shape.
- Vulkan Guide device, features, extensions, portability, and validation topics for explanation.
- LunarG macOS Vulkan SDK docs for Apple SDK expectations.
- MoltenVK Runtime User Guide for Apple portability constraints.
- Vulkan Samples only after first device/swapchain smoke is validation-clean.
- How to Vulkan in 2026 as a secondary practical reference for physical-device enumeration, queue-family selection, presentation-support checks, Vulkan 1.3 feature chaining, and `VK_KHR_swapchain` enablement.

MoltenVK is an Apple portability target. It does not define cross-platform Vulkan behavior.

## Scope

In scope:

- instance extension/layer validation;
- API version baseline;
- physical device enumeration;
- physical device rejection gates;
- physical device scoring;
- queue family selection;
- surface present support;
- required device extensions;
- required device features;
- dynamic rendering support gate;
- synchronization feature policy;
- portability subset handling;
- logical device creation;
- queue retrieval;
- diagnostics and failure reason codes.

Out of scope:

- platform window creation;
- native surface implementation details;
- swapchain format/present mode selection;
- command buffer allocation;
- memory allocation strategy;
- shader compilation;
- runtime or projection behavior.

## Local File Surface

Likely future files:

```text
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_surface_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
```

`InstanceDeviceSurface` owns instance, physical device choice, logical device, queues, surface handle wiring, selected features/extensions, and device diagnostics.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| Vulkan instance | `InstanceDeviceSurface` | created before surface |
| debug messenger | `DebugValidation` or `InstanceDeviceSurface` | development/smoke only |
| surface | backend surface owner | created through platform shell provider |
| selected physical device handle | `InstanceDeviceSurface` | borrowed handle, not destroyed |
| logical device | `InstanceDeviceSurface` | destroyed after child resources |
| queues | `InstanceDeviceSurface` | retrieved from logical device |
| enabled feature/extension record | `InstanceDeviceSurface` diagnostics | public as text/diagnostic only |

Rules:

- runtime/projection never see Vulkan handles or feature structs;
- platform shell provides required instance extensions and surface callback only;
- logical device must outlive swapchain, resources, command pools, sync objects, and pipelines;
- surface must exist before checking present support;
- diagnostics can expose names/versions/features, not raw handle values.

## Baseline Decision

Preferred first baseline:

```text
api_baseline=Vulkan 1.3 where available
rendering_path=dynamic_rendering
sync_policy=binary_wsi
timeline_semaphores=deferred
descriptor_indexing=deferred
vma_adoption=after_first_room_or_resource_growth
```

Compatibility gate:

- if macOS/MoltenVK, Linux native Vulkan, or Windows native Vulkan cannot support the selected baseline, decide whether to:
  - use Vulkan 1.3 core where available plus extension fallback;
  - lower the baseline to Vulkan 1.2 plus required extensions;
  - use render-pass fallback for a blocked platform.

Do not silently lower the baseline. Any fallback must be recorded in [decisions.md](decisions.md), [fallbacks.md](fallbacks.md), and diagnostics.

## Instance Requirements

Required instance inputs:

- app name/version;
- engine name/version;
- requested API version;
- validation layer request;
- platform-required instance extensions;
- debug utils extension if validation/debug messenger is enabled;
- portability enumeration extension on Apple/MoltenVK when required.

Rules:

- platform-required instance extensions must be queried before instance creation;
- missing required instance extension fails strict Vulkan smoke;
- validation layer absence fails only when strict validation requires it;
- release builds do not force validation layers;
- enabled instance extensions must be printed.

Diagnostics:

```text
requested_api_version=
created_api_version=
validation=enabled|disabled|unavailable
enabled_instance_extensions=
missing_instance_extensions=
enabled_instance_layers=
```

## Physical Device Enumeration

Enumeration rules:

- enumerate all physical devices visible to the Vulkan loader;
- gather properties for each candidate;
- gather features for each candidate;
- gather extension list for each candidate;
- gather queue families for each candidate;
- query surface present support for each queue family;
- gather swapchain support summary for each candidate.

No visible device is an environment/platform failure, not a runtime failure.

Diagnostics should include candidate count and one summary row per candidate when verbose diagnostics are enabled.

## Hard Rejection Gates

Reject a physical device if any required gate fails:

- device cannot create a logical device;
- no graphics queue family;
- no present queue family for the current surface;
- required swapchain device extension missing;
- no supported surface formats;
- no supported present modes;
- dynamic rendering unavailable when selected baseline requires it;
- required depth format support missing;
- required shader stage/push constant limit below first-room needs;
- portability subset requirements not enabled/handled when required;
- validation of required features/extensions fails.

First-room required limits:

```text
min_push_constant_bytes_required=64
required_vertex_input=true
required_depth_attachment=true
required_color_attachment=true
```

Rejected candidates must produce a stable reason code.

## Device Scoring

After hard gates pass, score devices deterministically.

Suggested scoring:

```text
discrete_gpu_bonus=1000
integrated_gpu_bonus=500
cpu_or_software_penalty=-500 unless software lane explicitly requested
dynamic_rendering_core_bonus=100
same_graphics_present_queue_bonus=50
dedicated_transfer_queue_bonus=10
larger_limits_small_bonus=bounded
```

Rules:

- scoring is tie-break behavior only;
- a device that fails a hard gate cannot win by score;
- user/device override may select a candidate only if it passes hard gates;
- selected device index and score must be printed;
- software Vulkan is acceptable only in explicitly diagnosed optional lanes, not as shipping proof.

## Queue Family Selection

Required queues:

- graphics queue;
- present queue for current surface.

Optional queues:

- compute queue for future compute work;
- transfer queue for future upload path.

First policy:

```text
graphics_queue=required
present_queue=required
compute_queue=optional
transfer_queue=optional
prefer_same_graphics_present=true
```

Rules:

- if one queue family supports both graphics and present, prefer it for first implementation;
- if graphics and present differ, select both and make swapchain sharing mode explicit;
- command pool uses graphics queue family;
- present queue is used only for presentation unless later design says otherwise;
- transfer queue is not used until ownership-transfer policy is designed.

Diagnostics:

```text
graphics_queue_family=
present_queue_family=
compute_queue_family=
transfer_queue_family=
graphics_present_same=true|false
queue_family_count=
```

## Required Device Extensions

Required first extension:

```text
VK_KHR_swapchain
```

Conditional extensions:

- dynamic rendering extension path if Vulkan 1.3 core is unavailable and Vulkan 1.2 plus extension fallback is selected;
- portability subset extension on MoltenVK/Apple when reported/required;
- shader-related extensions only if shader pipeline file plan requires them.

Deferred extensions:

- descriptor indexing;
- timeline semaphore, unless future internal sync adoption gate passes;
- acceleration structures/ray tracing;
- mesh shaders;
- advanced material/texture features.

Rules:

- enabled device extensions must be printed;
- missing required extension fails strict smoke;
- optional missing extension disables optional feature with diagnostics;
- extension choice must not leak into runtime/projection.

## Required Features

Required first features:

- features needed for dynamic rendering baseline or selected fallback;
- robust enough limits for push constant matrix;
- depth attachment support through selected depth format;
- shader module/pipeline support for first-room vertex/fragment shaders.

Preferred feature policy:

```text
dynamic_rendering=required_for_modern_baseline
timeline_semaphore=queried_deferred
descriptor_indexing=queried_deferred
sampler_anisotropy=deferred
wide_lines=deferred
fill_mode_non_solid=deferred
```

Rules:

- query features through the correct Vulkan feature chain for the selected API baseline;
- enable only features actually used by first implementation;
- print required, enabled, and unavailable feature summary;
- do not enable optional features just because they exist.

## Dynamic Rendering Gate

Dynamic rendering is the preferred renderer baseline.

Pass conditions:

- selected API/extension path supports dynamic rendering;
- feature is enabled if required by the chosen path;
- pipeline creation can name swapchain color and depth formats;
- command recording can begin/end dynamic rendering validation-clean;
- macOS/MoltenVK, Linux, and Windows lanes have an evidence path.

Fail behavior:

- if dynamic rendering is unavailable on a required lane, use [fallbacks.md](fallbacks.md) decision process;
- render-pass fallback must be explicit and diagnosed;
- old renderer style does not automatically decide the new baseline.

Diagnostics:

```text
dynamic_rendering_supported=true|false
dynamic_rendering_enabled=true|false
dynamic_rendering_source=core_1_3|extension|unavailable
rendering_path=dynamic_rendering|render_pass
```

## Portability And MoltenVK

Apple/MoltenVK lane requirements:

- instance portability enumeration handled when required;
- device portability subset extension handled when reported/required;
- MoltenVK constraints are printed as diagnostics;
- dynamic rendering support is verified through actual feature/extension evidence;
- selected behavior remains Vulkan-first and cross-platform-aware.

Rules:

- MoltenVK is not the cross-platform source of truth;
- Apple quirks may create diagnosed platform exceptions;
- required Linux and Windows native Vulkan lanes still gate shippable renderer status;
- do not design runtime/projection around MoltenVK behavior.

Diagnostics:

```text
platform=macos
vulkan_runtime=moltenvk
portability_enumeration=true|false|unavailable
portability_subset=true|false|unavailable
moltenvk=true|false
```

## Logical Device Creation

Logical device creation requires:

- unique queue create infos for selected queue families;
- queue priority values;
- required device extensions;
- feature chain for enabled features;
- validation-independent diagnostic record of enabled features/extensions.

Rules:

- do not request queues that were not selected;
- do not enable features not used or not supported;
- device creation failure is renderer initialization failure;
- after logical device creation, retrieve selected queues and verify non-null handles;
- device destroy happens after all child Vulkan objects are destroyed.

Failure reason:

```text
logical_device_create_failed
queue_retrieval_failed
```

## Device Override Policy

Useful for development:

```text
--vulkan-device-index=<n>
--vulkan-prefer-software
--vulkan-require-discrete
```

Rules:

- overrides are visual app/dev/test settings only;
- override cannot select a device that fails hard gates;
- software preference must be diagnosed;
- shipping validation cannot rely only on software Vulkan;
- device override must not affect runtime determinism.

If override fails, print candidate list and reason.

## Failure Policy

Device selection failures are renderer/platform failures.

Common failures:

- no Vulkan loader;
- instance creation failed;
- required instance extension missing;
- surface creation failed;
- no physical devices;
- no suitable physical device;
- missing graphics queue;
- missing present queue;
- missing swapchain extension;
- missing dynamic rendering support when required;
- missing depth support;
- logical device creation failed.

Rules:

- strict Vulkan smoke exits nonzero;
- optional GPU smoke may skip only through documented skip policy;
- failure receipt must name the failing gate;
- runtime/headless tests remain independent.

## Diagnostics Receipt Fields

Device diagnostics should include:

```text
vulkan_loader=found|missing|unavailable
requested_api_version=
created_api_version=
instance_created=true|false
surface_created=true|false
physical_device_count=
selected_physical_device_index=
device_name=
device_vendor_id=
device_id=
device_type=
driver_version=
api_version=
device_score=
device_reject_reasons=
graphics_queue_family=
present_queue_family=
compute_queue_family=
transfer_queue_family=
graphics_present_same=true|false
enabled_instance_extensions=
enabled_device_extensions=
enabled_features=
missing_required_extensions=
missing_required_features=
dynamic_rendering_supported=true|false
dynamic_rendering_enabled=true|false
dynamic_rendering_source=
timeline_semaphore_supported=true|false|unavailable
timeline_semaphore_enabled=false
descriptor_indexing_supported=true|false|unavailable
descriptor_indexing_enabled=false
portability_enumeration=true|false|unavailable
portability_subset=true|false|unavailable
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
reason=
```

Use `unavailable` only when a field truly does not apply.

## Failure Reason Codes

Recommended reason codes:

```text
no_vulkan_loader
instance_create_failed
missing_instance_extension
missing_validation_layer
surface_create_failed
physical_device_enumeration_failed
no_physical_devices
no_suitable_physical_device
missing_graphics_queue
missing_present_queue
missing_swapchain_extension
missing_surface_support
missing_surface_formats
missing_present_modes
missing_dynamic_rendering
missing_depth_format_support
push_constant_limit_too_small
portability_required_missing
logical_device_create_failed
queue_retrieval_failed
device_override_invalid
software_device_not_allowed
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) when file plans begin.

## Platform Notes

macOS/MoltenVK:

- expect Vulkan through MoltenVK, not native Apple Vulkan;
- portability instance/device handling is likely required;
- device may report portability subset;
- validation/sync validation availability must be printed honestly;
- local first lane proves Apple portability only, not native Vulkan shipping status.

Linux:

- native Vulkan lane must verify loader, ICD, validation layer availability, surface present support, and swapchain extension;
- Wayland/X11 surface support may differ by selected SDL backend;
- optional lavapipe/software lane may help CI but cannot replace hardware/native validation.

Windows:

- native Vulkan lane must verify loader/runtime, physical device, present support, validation layer availability, and swapchain extension;
- installed package smoke must distinguish missing loader from missing device;
- device selection must survive GPU driver differences without runtime changes.

## Tests

Future tests:

```text
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_surface_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
```

Smoke expectations:

- instance creates with required extensions;
- surface creates through platform provider;
- physical devices enumerate;
- selected device passes hard gates;
- graphics/present queue families are reported;
- logical device creates;
- dynamic rendering support decision is printed;
- validation is clean;
- no runtime hash/state changes occur because device smoke ran.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|vulkan_surface|vulkan_swapchain|vulkan_pipeline'
```

Platform validation:

- macOS/MoltenVK: local first smoke lane;
- Linux: native Vulkan device/surface/swapchain lane;
- Windows: native Vulkan device/surface/swapchain lane.

## Acceptance Criteria

This device-selection contract is ready for file plans when:

- API baseline and fallback gate are explicit;
- instance extension/layer rules are defined;
- physical device hard gates are defined;
- candidate scoring is deterministic;
- queue family selection is defined;
- required extensions are named;
- required/deferred features are named;
- dynamic rendering gate is defined;
- MoltenVK portability handling is explicit;
- logical device creation rules are defined;
- diagnostics and failure reason codes are defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- device selection failure cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact API version request value and fallback behavior;
- exact feature-chain structs for Vulkan 1.3 versus 1.2 extension path;
- exact dynamic rendering extension fallback;
- exact portability extension handling on MoltenVK;
- exact scoring constants after first hardware diagnostics;
- exact device override CLI;
- exact candidate verbose diagnostics format;
- exact validation layer enablement code path;
- exact Linux and Windows device smoke commands.
