# Vulkan Feature Baseline

This document defines the Vulkan API, feature, extension, and fallback baseline for the `iggy3d` Vulkan backend.

The decision is explicit: `iggy3d` targets a modern Vulkan path with dynamic rendering required. Legacy render-pass style is not the first implementation. It is an escape hatch only if platform evidence blocks dynamic rendering on a required lane.

## Purpose

Provide one feature authority for Vulkan implementation packets:

```text
preferred_api=Vulkan 1.3
required_rendering_path=dynamic_rendering
legacy_render_pass=escape_hatch_only
first_sync_policy=binary_wsi_semaphores_and_fences
timeline_semaphores=deferred
descriptor_indexing=deferred
vma=required_for_resource_growth
validation=required_for_strict_dev_smoke
apple_portability=MoltenVK_or_current_SDK_portability_lane
linux_windows=native_vulkan_required_lanes
```

This document is upstream of [device_selection.md](device_selection.md), [command_recording.md](command_recording.md), [sync_contract.md](sync_contract.md), [swapchain_contract.md](swapchain_contract.md), [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md), [shader_interface_contract.md](shader_interface_contract.md), [debug_validation.md](debug_validation.md), [platform_matrix.md](platform_matrix.md), and [fallbacks.md](fallbacks.md).

## Verified Source Anchors

Use these sources when implementing or reviewing this baseline:

| Source | URL | Baseline use |
| --- | --- | --- |
| Vulkan Specification/Registry | https://registry.khronos.org/vulkan/ | exact API, feature, extension, and valid-usage lawbook |
| Vulkan Guide versions/porting | https://docs.vulkan.org/guide/latest/versions.html | Vulkan 1.3 feature context and dynamic rendering guidance |
| Vulkan dynamic rendering sample | https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html | render-pass-free attachment and pipeline setup reference |
| How to Vulkan in 2026 | https://howtovulkan.com/ | secondary practical reference for modern Vulkan 1.3-era dynamic rendering, synchronization2 direction, descriptor indexing direction, and VMA/SDL/RenderDoc ecosystem shape |
| Vulkan timeline semaphore sample | https://docs.vulkan.org/samples/latest/samples/extensions/timeline_semaphore/README.html | later timeline semaphore reference |
| Khronos timeline semaphore blog | https://www.khronos.org/blog/vulkan-timeline-semaphores | rationale and WSI caveat for timeline semaphores |
| LunarG macOS Vulkan SDK guide | https://vulkan.lunarg.com/doc/view/latest/mac/getting_started.html | Apple SDK, loader, MoltenVK/KosmicKrisp, and validation packaging context |
| MoltenVK Runtime/User Guide entry | https://github.com/KhronosGroup/MoltenVK/ | Apple portability behavior and SPIR-V to MSL conversion context |
| Vulkan portability sample | https://docs.vulkan.org/samples/latest/samples/extensions/portability/README.html | portability enumeration/subset handling pattern |
| Vulkan Validation Layers | https://github.com/KhronosGroup/Vulkan-ValidationLayers | development validation layer authority |
| LunarG synchronization validation | https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html | sync validation setup and hazard checking |

Source conclusions:

- dynamic rendering is the chosen new path because Vulkan 1.3 promotes it and it removes first-path dependence on render pass and framebuffer objects;
- How to Vulkan reinforces the modern implementation direction, but it is not an authority for feature legality, Apple portability, or `iggy3d` packet boundaries;
- timeline semaphores are useful but deferred because WSI acquire/present still keeps binary semaphore constraints relevant for the first frame loop;
- Apple support is a Vulkan portability lane over Metal and must be queried/diagnosed, not assumed from headers;
- Linux and Windows require native Vulkan validation before the backend is called portable.

## Baseline Summary

Required first implementation:

```text
api_preferred=Vulkan 1.3
api_compatibility_floor=Vulkan 1.2 plus required extensions only when proven necessary
rendering_path=dynamic_rendering
swapchain_extension=VK_KHR_swapchain
sync_policy=binary_wsi
frames_in_flight=2
shader_input=SPIR-V
shader_authoring_initial=GLSL via glslang
descriptor_model=conventional_descriptor_sets
memory_growth=VMA
validation_layer=VK_LAYER_KHRONOS_validation when available
sync_validation=strict_lane_required_when_available
```

Deferred:

```text
legacy_render_pass
timeline_semaphores
synchronization2_as_hard_requirement
descriptor_indexing
bindless_resources
shader_objects
mesh_shaders
ray_tracing
compute_renderer_path
multi_queue_async_transfer
gpu_driven_rendering
```

Rules:

- device selection must reject devices that cannot support dynamic rendering through core Vulkan 1.3 or an accepted extension path;
- a render-pass fallback cannot be implemented silently;
- any lowered baseline must update this document, [decisions.md](decisions.md), [fallbacks.md](fallbacks.md), and diagnostics;
- runtime/content/projection must not care which Vulkan feature path is selected.

## API Version Policy

Preferred request:

```text
requested_api_version=VK_API_VERSION_1_3
```

Compatibility rule:

```text
if device_api_version >= Vulkan 1.3:
  prefer core Vulkan 1.3 dynamic rendering names and feature query path
else if device_api_version >= Vulkan 1.2 and VK_KHR_dynamic_rendering is available:
  allow compatibility path only after required platform evidence says it is needed
else:
  reject device for Vulkan visual lane
```

Rules:

- do not infer support from SDK/header version;
- query instance version, physical device API version, features, and extensions;
- print requested API version and selected device API version;
- Vulkan 1.4 is not required for first-room, asset/material, or smoke validation;
- using Vulkan 1.2 plus extensions is a compatibility path, not a lower design target.

## Dynamic Rendering Requirement

Dynamic rendering is required.

Accepted paths:

```text
dynamic_rendering_source=core_vulkan_1_3
dynamic_rendering_source=VK_KHR_dynamic_rendering_extension
```

Rejected first path:

```text
rendering_path=legacy_render_pass
```

Dynamic rendering implications:

- no first-path `VkRenderPass` ownership module;
- no first-path framebuffer object surface as renderer architecture;
- command recording begins rendering with dynamic rendering commands;
- pipeline creation uses dynamic rendering attachment format compatibility;
- color and depth formats become part of pipeline variant compatibility;
- swapchain/depth image views are referenced directly at render time.

Rules:

- `command_recording.md` is authoritative for record order;
- `pipeline_cache_and_variants.md` is authoritative for pipeline variant keys;
- `swapchain_contract.md` is authoritative for selected color format;
- `depth_and_coordinates.md` is authoritative for selected depth format and depth state;
- render-pass fallback requires a dedicated decision packet and cannot share the first-path acceptance criteria.

## Render-Pass Escape Hatch

Legacy render-pass fallback may be considered only when all are true:

- at least one required lane cannot support dynamic rendering after current SDK/driver evidence is checked;
- the lane is still required for shipping;
- lowering to Vulkan 1.2 plus `VK_KHR_dynamic_rendering` does not solve it;
- reviewer accepts the extra implementation surface;
- diagnostics can report which rendering path is active.

Fallback documentation must update:

```text
docs/vulkan/vulkan_feature_baseline.md
docs/vulkan/decisions.md
docs/vulkan/fallbacks.md
docs/vulkan/command_recording.md
docs/vulkan/pipeline_cache_and_variants.md
docs/vulkan/file_surface.md
```

Until that happens, builders should treat render-pass work as out of scope.

## Required Instance Extensions

Always platform-dependent:

```text
platform_surface_extensions_from_window_shell
```

Development/debug:

```text
VK_EXT_debug_utils when validation/debug messenger is enabled
```

Apple portability when exposed/required:

```text
VK_KHR_portability_enumeration
VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
```

Rules:

- platform shell provides WSI instance extension names;
- Vulkan backend validates requested instance extensions before instance creation;
- missing platform-required surface extension fails Vulkan visual lane;
- missing debug utils disables debug messenger only in non-strict mode;
- strict validation mode fails if required validation/debug extensions are unavailable;
- Apple portability enumeration must be explicit when needed to see MoltenVK-style devices.

## Required Device Extensions

Required first visual lane:

```text
VK_KHR_swapchain
```

Required compatibility path when dynamic rendering is not core:

```text
VK_KHR_dynamic_rendering
```

Apple portability when exposed by selected device:

```text
VK_KHR_portability_subset
```

Optional/deferred:

```text
VK_KHR_synchronization2
VK_KHR_timeline_semaphore
VK_EXT_descriptor_indexing
VK_EXT_debug_marker
VK_EXT_debug_utils device labels where applicable
```

Rules:

- if `VK_KHR_portability_subset` is advertised and required by the portability implementation, enable it and report it;
- optional extensions are query-and-print only until a document promotes them;
- do not enable broad optional extensions just because they exist;
- extension fallback paths must be visible in diagnostics.

## Required Physical Device Features

Required for first dynamic-rendering path:

```text
dynamicRendering=true
```

Required practical graphics capabilities:

```text
graphics_queue=true
present_queue_for_surface=true
swapchain_support=true
vertex_input=true
depth_attachment_format_supported=true
push_constant_size_at_least_64=true
sampled_image_support=true for material/texture growth
sampler_support=true for material/texture growth
```

Required only when promoted by phase:

```text
samplerAnisotropy=true only after anisotropy gate
timelineSemaphore=true only after timeline gate
synchronization2=true only after sync2 gate
descriptorIndexing=true only after descriptor indexing gate
```

Rules:

- first-room can run without texture/material features beyond baseline graphics and depth;
- material/texture smoke must require sampled image and sampler support;
- optional feature absence should degrade only when the relevant feature is optional;
- dynamic rendering absence fails the Vulkan visual lane.

## Synchronization Feature Policy

First path:

```text
sync_policy=binary_wsi
image_available=VkSemaphore binary
render_finished=VkSemaphore binary
frame_done=VkFence
frames_in_flight=2
```

Timeline semaphore policy:

```text
timeline_semaphores=deferred
timeline_use_cases=uploads, async transfer, frame pacing, multi-queue work
timeline_not_required_for_first_room=true
```

Synchronization2 policy:

```text
synchronization2=preferred_later
synchronization2_hard_gate=false_for_first_room
```

Rules:

- WSI acquire/present path must remain binary-semaphore-correct;
- do not introduce timeline semaphores to hide broken binary WSI flow;
- do not make synchronization2 a first-room blocker unless a later command/sync packet deliberately adopts it;
- sync validation must run against the selected sync path in strict lanes.

## Descriptor Feature Policy

First path:

```text
descriptor_indexing=deferred
bindless=deferred
update_after_bind=deferred
descriptor_buffer=deferred
```

Allowed first material path:

```text
conventional_descriptor_sets=true
set1_material.binding0=combined_image_sampler_base_color
```

Rules:

- first-room has zero descriptors;
- unlit textured material may use conventional descriptors only after descriptor gates pass;
- descriptor indexing is a future scalability choice, not an early architecture requirement;
- runtime/content/projection never own descriptor features or binding numbers.

## Memory Feature Policy

First-room baseline:

```text
manual_small_buffer_allocation=allowed_only_if_documented
vma_required_for_resource_growth=true
```

Resource growth baseline:

```text
allocator=VMA
allocation_names=required
memory_budget_diagnostics=required_when_available
```

Rules:

- VMA is not required before the smallest first-room proof if that would block basic wiring;
- VMA is required before texture/material/resource growth becomes normal development;
- no anonymous GPU memory in asset/material smoke;
- memory feature decisions live in renderer docs, not runtime state.

## Shader Feature Policy

First path:

```text
shader_ir=SPIR-V
shader_language_initial=GLSL
compiler_initial=glslangValidator or configured glslang path
shader_interface=manual_table_plus_tests
```

Deferred:

```text
Slang
HLSL
runtime_shader_generation
shader_objects
SPIR-V_reflection_as_hard_requirement
```

Rules:

- shader source and generated SPIR-V are renderer/build inputs;
- runtime/content/projection never compile or load shaders;
- shader interface names must match [shader_interface_contract.md](shader_interface_contract.md);
- Slang may be reopened only after toolchain and packaging evidence across macOS, Linux, and Windows.

## Validation Baseline

Development strict lane:

```text
validation_layer=VK_LAYER_KHRONOS_validation
debug_messenger=VK_EXT_debug_utils
validation_errors_fail=true
```

Synchronization strict lane:

```text
sync_validation=enabled
sync_hazards_fail=true
```

Non-strict local lane:

```text
validation_unavailable=diagnosed_skip_or_warning
```

Release lane:

```text
validation_layers_not_required=true
```

Rules:

- validation must be enabled during Vulkan backend development when available;
- sync validation should run after standard validation is clean;
- validation absence is acceptable only in non-strict local runs;
- strict smoke must fail if required validation tooling is missing.

## Platform Baseline

macOS:

```text
lane=Apple portability Vulkan over Metal
first_target=MoltenVK or current SDK-supported Apple Vulkan implementation
portability_enumeration=required_when_needed
portability_subset=required_when_advertised
validation=through_Vulkan_SDK_when_available
```

Linux:

```text
lane=native Vulkan
loader=native Vulkan loader
driver=native ICD
software_vulkan=optional_dev_or_ci_only
```

Windows:

```text
lane=native Vulkan
loader=native Vulkan runtime/SDK
multi_config_shader_paths=required
validation=through_Vulkan_SDK_when_available
```

Rules:

- macOS success does not prove Linux/Windows portability;
- Linux/Windows native Vulkan proof is required before calling the renderer portable;
- software Vulkan can help CI but cannot replace hardware/native shipping proof;
- platform-specific feature absence must be captured as diagnostics.

## Device Rejection Gates

Reject a Vulkan device for the visual lane if:

```text
no_physical_devices
no_graphics_queue
no_present_queue_for_surface
missing_swapchain_extension
no_surface_formats
no_present_modes
dynamic_rendering_unavailable
required_portability_extension_missing
required_depth_format_missing
push_constant_limit_too_small
validation_required_but_unavailable strict_only
sync_validation_required_but_unavailable strict_sync_only
```

Reject material/texture growth if:

```text
sampled_image_support_missing
sampler_creation_failed
descriptor_material_layout_unavailable
vma_required_but_unavailable
```

Rules:

- rejection must be explicit and diagnostic;
- do not fall back to null renderer without reporting why Vulkan was rejected;
- unsupported Vulkan backend must not affect headless runtime acceptance.

## Diagnostics Receipt

Vulkan feature baseline smoke should print:

```text
vulkan_feature_baseline=modern_dynamic_rendering_v1
requested_api_version=
instance_api_version=
selected_device_api_version=
selected_device_name=
selected_device_type=
selected_driver_version=
rendering_path=dynamic_rendering
dynamic_rendering_source=core_1_3|khr_extension|unavailable
legacy_render_pass_enabled=false
sync_policy=binary_wsi
timeline_semaphore_available=true|false
timeline_semaphore_enabled=false
synchronization2_available=true|false
synchronization2_enabled=true|false
descriptor_indexing_available=true|false
descriptor_indexing_enabled=false
swapchain_extension_enabled=true|false
portability_enumeration_enabled=true|false
portability_subset_enabled=true|false
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
enabled_instance_extensions=
enabled_device_extensions=
enabled_features=
unsupported_reason=
```

Rules:

- receipts should use the same field names on macOS, Linux, and Windows;
- optional features should print both availability and enabled state;
- fallback paths must be visible.

## Failure Reason Codes

Use stable reason codes:

```text
feature_baseline_scope_blocked
vulkan_loader_unavailable
vulkan_instance_version_too_low
vulkan_device_version_too_low
vulkan_no_physical_devices
vulkan_no_graphics_queue
vulkan_no_present_queue
vulkan_swapchain_extension_missing
vulkan_surface_support_missing
vulkan_dynamic_rendering_missing
vulkan_dynamic_rendering_feature_disabled
vulkan_render_pass_fallback_unapproved
vulkan_portability_enumeration_missing
vulkan_portability_subset_missing
vulkan_depth_format_missing
vulkan_push_constant_limit_too_small
vulkan_validation_required_unavailable
vulkan_sync_validation_required_unavailable
vulkan_material_feature_missing
vulkan_vma_required_unavailable
```

Rules:

- reason codes should appear in smoke output;
- reason codes must not vary by platform;
- platform-native error messages may be appended, but stable reason codes remain primary.

## Tests

Expected future tests:

```text
tests/unit/render_feature_baseline_tests.cpp
tests/unit/render_device_policy_tests.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_feature_baseline_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Unit tests should cover:

- dynamic rendering required policy;
- render-pass fallback blocked by default;
- optional feature availability vs enabled state;
- reason code stability;
- descriptor indexing deferred policy;
- timeline semaphore deferred policy.

Smoke tests should cover:

- feature receipt on selected device;
- dynamic rendering enabled path;
- strict validation unavailable failure;
- strict sync validation unavailable failure;
- non-strict unsupported-device diagnostic skip;
- null renderer/headless unaffected by Vulkan absence.

Suggested policy grep:

```sh
rg -n "VkRenderPass|VkFramebuffer|vkCmdBeginRenderPass" src/render/vulkan
```

Expected first implementation result: no production render-pass path unless a fallback packet is accepted.

Suggested firewall grep:

```sh
rg -n "vulkan/vulkan.h|Vk[A-Z]|VK_" src/runtime src/content src/projection src/runtime/save
```

Expected result: no runtime/content/projection/save Vulkan leakage.

## Acceptance Criteria

This baseline is satisfied when:

- selected device reports dynamic rendering support through core Vulkan 1.3 or approved KHR extension path;
- command recording uses dynamic rendering;
- pipeline diagnostics include dynamic rendering attachment formats;
- no first-path render-pass/framebuffer architecture exists;
- sync baseline uses binary WSI semaphores and fences;
- timeline semaphores are reported but disabled;
- descriptor indexing is reported but disabled;
- validation and sync validation behavior matches strict/non-strict lane rules;
- macOS portability lane reports portability enumeration/subset state;
- Linux and Windows native lanes have planned smoke receipts;
- missing Vulkan support does not break headless runtime acceptance.

## Open Detail Items

The next detailed pass should define:

- exact feature query structs and `pNext` chain order for Vulkan 1.3 and extension fallback;
- exact dynamic rendering function pointer/name policy for core vs KHR path;
- whether synchronization2 is adopted before or after first-room proof;
- exact macOS SDK implementation target: MoltenVK first, KosmicKrisp trial, or both as diagnostics;
- exact Linux native smoke command and required packages;
- exact Windows native smoke command and Vulkan SDK discovery;
- exact strict vs optional smoke exit codes;
- whether VMA is fetched as source, package, or submodule later;
- whether feature baseline receipts are emitted by `vulkan_device_smoke` or a separate smoke binary.
