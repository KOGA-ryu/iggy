# Vulkan Feature Query Chain

This document defines how `iggy3d` queries, records, validates, and enables Vulkan instance/device features and extensions.

The feature query chain is renderer-owned. Runtime/content/projection may receive a backend-neutral diagnostics summary, but they never own Vulkan feature structs, extension names, `pNext` chains, `Vk*` handles, or `VK_*` constants.

## Purpose

Turn [vulkan_feature_baseline.md](vulkan_feature_baseline.md) into implementation mechanics:

```text
query loader/instance capability
enumerate instance extensions and layers
create instance with required extensions and flags
create surface
enumerate physical devices
query properties, features, limits, extensions, queues, surface support
decide dynamic rendering source
build enabled extension list
build enabled feature pNext chain
create logical device
record exact support and enabled state in diagnostics
```

Rules:

- query support before enabling;
- support chain and enable chain are separate concepts;
- enable only accepted baseline features;
- optional feature availability is not permission to enable it;
- dynamic rendering is required;
- render-pass fallback is unapproved by default.

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for exact `pNext`, feature struct, extension, and device creation behavior.
- Vulkan Guide feature enabling page for the query-then-enable model.
- `VK_KHR_dynamic_rendering` reference page for core-vs-extension dynamic rendering names.
- `VkPhysicalDeviceVulkan13Features` reference page for `dynamicRendering`, `synchronization2`, and other Vulkan 1.3 feature members.
- `VkInstanceCreateFlagBits` reference page for portability enumeration behavior.
- LunarG macOS SDK and MoltenVK docs for Apple portability constraints.

## Scope

In scope:

- instance version query;
- instance extension/layer enumeration;
- portability enumeration flag policy;
- physical device feature/property query chain;
- device extension enumeration;
- dynamic rendering core-vs-extension path;
- optional feature query policy;
- logical device enable chain;
- diagnostics receipt fields;
- failure reason codes.

Out of scope:

- platform window creation;
- swapchain format/present mode choice;
- command recording details;
- shader compilation;
- VMA setup;
- render-pass fallback implementation;
- runtime/gameplay feature flags.

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
tests/unit/render_feature_query_policy_tests.cpp
tests/unit/render_device_policy_tests.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_feature_baseline_smoke.cpp
```

`InstanceDeviceSurface` should own the query chain, enabled extension list, enabled feature structs, logical device creation, and feature diagnostics.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| queried instance version | `InstanceDeviceSurface` | loader/instance capability |
| instance extension list | `InstanceDeviceSurface` | plus platform shell required extensions |
| validation layer list | `DebugValidation` or `InstanceDeviceSurface` | dev/strict lane |
| physical device properties | `InstanceDeviceSurface` | diagnostics/scoring |
| physical device features | `InstanceDeviceSurface` | support chain |
| enabled feature structs | `InstanceDeviceSurface` | device create chain |
| enabled extension names | `InstanceDeviceSurface` | device create input |
| dynamic rendering source | `InstanceDeviceSurface` | core 1.3 or KHR extension |
| portability state | `InstanceDeviceSurface` | Apple/portability diagnostics |

Rules:

- queried support structs may contain many `VK_TRUE` optional features;
- enabled structs must set only features chosen by docs/packets;
- runtime/content/projection do not receive raw feature structs;
- feature diagnostics expose names and booleans, not Vulkan ownership.

## Query Order

Recommended order:

1. Query loader/instance API version.
2. Enumerate instance extensions.
3. Enumerate instance layers.
4. Build instance extension list from platform shell plus renderer debug/portability needs.
5. Create instance.
6. Create debug messenger if enabled.
7. Create surface through platform shell.
8. Enumerate physical devices.
9. For each device, enumerate device extensions.
10. For each device, query device properties and limits.
11. For each device, query device features through the support `pNext` chain.
12. For each device, query queue families and surface present support.
13. For each device, query swapchain surface support.
14. Reject devices that fail hard gates.
15. Score accepted devices.
16. Build selected device enabled extension list.
17. Build selected device enabled feature chain.
18. Create logical device.
19. Retrieve queues.
20. Print feature receipt.

Rules:

- surface must exist before present support and swapchain support queries;
- support queries should happen before scoring;
- logical device creation should use the same selected-device decision record printed in diagnostics.

## Instance Version Query

Policy:

```text
requested_api_version=VK_API_VERSION_1_3
minimum_accepted_runtime_path=Vulkan 1.2 plus VK_KHR_dynamic_rendering only if approved by baseline compatibility gate
```

Implementation expectations:

- use `vkEnumerateInstanceVersion` when available;
- if unavailable, treat the loader as Vulkan 1.0-era and reject the modern Vulkan lane unless a later compatibility packet explicitly supports it;
- print loader/instance version separately from selected physical device version;
- do not infer runtime capability from installed headers or SDK version.

Diagnostics:

```text
requested_api_version=
instance_api_version=
loader_version_source=vkEnumerateInstanceVersion|assumed_1_0|unavailable
```

## Instance Extension Policy

Instance extension inputs:

```text
platform_required_instance_extensions
renderer_debug_instance_extensions
renderer_portability_instance_extensions
```

Required categories:

- WSI/surface extensions from the platform shell;
- `VK_EXT_debug_utils` when debug messenger/validation output is enabled;
- `VK_KHR_portability_enumeration` when needed to enumerate Apple portability devices.

Rules:

- platform shell provides surface extension names but does not create Vulkan policy;
- missing platform-required surface extension rejects Vulkan visual lane;
- missing `VK_EXT_debug_utils` fails strict validation lane and degrades only in non-strict mode;
- `VK_KHR_portability_enumeration` is enabled only when available and needed;
- enabled instance extensions are stable diagnostics fields.

## Portability Enumeration

Apple portability policy:

```text
if VK_KHR_portability_enumeration is available and Apple portability lane is active:
  add VK_KHR_portability_enumeration to enabled instance extensions
  add VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR to VkInstanceCreateInfo.flags
```

Rules:

- this flag allows portability-compliant physical devices to be enumerated;
- do not set the flag without the extension being available/enabled;
- Linux and Windows native lanes should report portability enumeration as disabled or unavailable unless a portability implementation is explicitly selected;
- portability enumeration affects device discovery only; it does not change runtime/gameplay behavior.

Diagnostics:

```text
portability_enumeration_available=true|false
portability_enumeration_enabled=true|false
instance_create_enumerate_portability_bit=true|false
```

## Instance Layer Policy

Validation layer:

```text
VK_LAYER_KHRONOS_validation
```

Rules:

- strict validation lane requires this layer;
- non-strict local lane may run without it but must report absence;
- release lane does not require validation layers;
- sync validation is configured through validation settings, not a separate layer name.

Diagnostics:

```text
validation_layer_available=true|false
validation_layer_enabled=true|false
validation_strict=true|false
```

## Device Extension Enumeration

For every candidate physical device, record:

```text
available_device_extensions
required_device_extensions_missing
optional_device_extensions_available
```

Required first visual extension:

```text
VK_KHR_swapchain
```

Required only for compatibility dynamic-rendering path:

```text
VK_KHR_dynamic_rendering
```

Required when advertised/required by Apple portability implementation:

```text
VK_KHR_portability_subset
```

Optional query-only extensions:

```text
VK_KHR_timeline_semaphore
VK_KHR_synchronization2
VK_EXT_descriptor_indexing
```

Rules:

- a device missing `VK_KHR_swapchain` fails visual lane;
- a Vulkan 1.2 compatibility device missing `VK_KHR_dynamic_rendering` fails visual lane;
- optional extension presence is printed but not enabled unless a doc promotes it;
- broad extension enabling is forbidden.

## Feature Support Chain

Use one support chain per physical device candidate.

Conceptual support chain:

```text
VkPhysicalDeviceFeatures2 support_features2
  -> VkPhysicalDeviceVulkan13Features support_vulkan13 when device_api_version >= 1.3
  -> VkPhysicalDeviceVulkan12Features support_vulkan12 when device_api_version >= 1.2 or useful for optional queries
  -> VkPhysicalDeviceDynamicRenderingFeaturesKHR support_dynamic_rendering_khr when extension path is considered
  -> VkPhysicalDevicePortabilitySubsetFeaturesKHR support_portability_subset when extension is available and headers allow it
```

Rules:

- every struct must have correct `sType`;
- support chain structs are zero-initialized before query;
- query with `vkGetPhysicalDeviceFeatures2`;
- support chain should include optional feature structs needed for diagnostics;
- support chain does not decide enabled state by itself.

Do not copy the full support chain directly into device creation with every supported feature set to true.

## Dynamic Rendering Query Decision

Decision algorithm:

```text
if device_api_version >= Vulkan 1.3:
  query VkPhysicalDeviceVulkan13Features
  if dynamicRendering == VK_TRUE:
    dynamic_rendering_source=core_1_3
    accepted=true
  else:
    reject device
else if device_api_version >= Vulkan 1.2 and VK_KHR_dynamic_rendering is available:
  query VkPhysicalDeviceDynamicRenderingFeaturesKHR
  if dynamicRendering == VK_TRUE and compatibility path is approved:
    dynamic_rendering_source=khr_extension
    accepted=true
  else:
    reject device
else:
  reject device
```

Rules:

- core 1.3 path is preferred;
- KHR extension path is compatibility only;
- render-pass fallback is not selected by this algorithm;
- if dynamic rendering is missing, reject with `vulkan_dynamic_rendering_missing`;
- if present but not enabled in the create chain, fail with `vulkan_dynamic_rendering_feature_disabled`.

## Optional Feature Query Policy

Query and print these optional features when practical:

```text
timelineSemaphore
synchronization2
descriptorIndexing related fields
samplerAnisotropy
```

Default enabled state:

```text
timelineSemaphore=false
synchronization2=false unless separately adopted
descriptorIndexing=false
samplerAnisotropy=false until sampler policy promotes it
```

Rules:

- optional feature availability is useful diagnostics;
- optional features must remain disabled until a doc/file plan promotes them;
- if a future packet enables one, update [vulkan_feature_baseline.md](vulkan_feature_baseline.md), tests, and receipt fields.

## Enabled Device Extension List

Build the enabled device extension list from the selected feature path.

Core 1.3 dynamic rendering path:

```text
enabled_device_extensions:
  VK_KHR_swapchain
  VK_KHR_portability_subset when required
```

Vulkan 1.2 compatibility dynamic rendering path:

```text
enabled_device_extensions:
  VK_KHR_swapchain
  VK_KHR_dynamic_rendering
  VK_KHR_portability_subset when required
```

Rules:

- do not enable `VK_KHR_timeline_semaphore` just because it is available;
- do not enable `VK_KHR_synchronization2` just because it is available;
- do not enable `VK_EXT_descriptor_indexing` just because it is available;
- enabled list must be printed exactly.

## Enabled Feature Chain

Use a separate enable chain for logical device creation.

Core 1.3 dynamic rendering enable chain:

```text
VkPhysicalDeviceFeatures2 enabled_features2
  -> VkPhysicalDeviceVulkan13Features enabled_vulkan13
```

Enabled fields:

```text
enabled_vulkan13.dynamicRendering = VK_TRUE
enabled_vulkan13.synchronization2 = VK_FALSE unless adopted
```

Compatibility dynamic rendering enable chain:

```text
VkPhysicalDeviceFeatures2 enabled_features2
  -> VkPhysicalDeviceDynamicRenderingFeaturesKHR enabled_dynamic_rendering_khr
```

Enabled fields:

```text
enabled_dynamic_rendering_khr.dynamicRendering = VK_TRUE
```

Optional future chain members:

```text
VkPhysicalDeviceVulkan12Features enabled_vulkan12 for timelineSemaphore or descriptorIndexing only after adoption
VkPhysicalDevicePortabilitySubsetFeaturesKHR enabled_portability_subset only when required and supported
```

Rules:

- never include both core and KHR dynamic rendering enable structs for the same purpose unless the implementation packet proves this is valid and necessary;
- `VkDeviceCreateInfo.pNext` points to the enabled chain;
- avoid `VkDeviceCreateInfo.pEnabledFeatures` if using `VkPhysicalDeviceFeatures2` for all features;
- leave unsupported or unadopted features false;
- the enabled chain must live long enough for `vkCreateDevice`.

## Core 1.0 Feature Policy

First-room required core 1.0 features should be minimal.

Initial default:

```text
VkPhysicalDeviceFeatures2.features = {}
```

Promote only when needed:

```text
samplerAnisotropy for anisotropic sampler gate
fillModeNonSolid for wireframe/debug polygon mode gate
wideLines for wide debug line gate
```

Rules:

- do not enable all core 1.0 features returned by support query;
- feature promotion requires diagnostics and tests;
- first-room vertex-color proof should not require optional core 1.0 features.

## Properties And Limits Query Chain

Use properties for scoring, limits, diagnostics, and portability details.

Conceptual properties chain:

```text
VkPhysicalDeviceProperties2 properties2
  -> VkPhysicalDeviceDriverProperties driver_properties when available
  -> VkPhysicalDevicePortabilitySubsetPropertiesKHR portability_properties when extension is available and headers allow it
```

Required limits:

```text
maxPushConstantsSize >= 64
```

Useful diagnostics:

```text
deviceName
deviceType
apiVersion
driverVersion
vendorID
deviceID
driverName when available
driverInfo when available
```

Rules:

- properties chain does not enable features;
- limits are hard gates only when documents define them;
- driver strings are diagnostics only and must not become gameplay logic.

## Format Feature Queries

Feature chain does not prove all format needs.

Required separate queries:

```text
depth format support through vkGetPhysicalDeviceFormatProperties
sampled texture format support for texture/material growth
surface format support through swapchain surface queries
```

Rules:

- dynamic rendering support does not prove chosen depth format support;
- `VK_FORMAT_R8G8B8A8_SRGB` texture support must be checked before first texture smoke;
- swapchain surface format/present mode support belongs to swapchain queries, not feature structs.

## Queue And Surface Support Queries

For every candidate:

```text
queue_family_count=
graphics_queue_family_index=
present_queue_family_index=
same_graphics_present_queue=true|false
surface_present_support=true|false
```

Rules:

- graphics queue is required;
- present queue for current surface is required;
- prefer same graphics/present queue for first path;
- separate graphics/present queues are allowed only when swapchain sharing and queue ownership policy are explicit.

## Candidate Decision Record

Every physical device candidate should produce a record:

```text
candidate_index=
device_name=
device_type=
api_version=
driver_version=
supports_swapchain=true|false
supports_dynamic_rendering=true|false
dynamic_rendering_source=core_1_3|khr_extension|unavailable
supports_graphics_queue=true|false
supports_present_queue=true|false
supports_required_depth_format=true|false
portability_subset_required=true|false
portability_subset_enabled_planned=true|false
hard_gate_pass=true|false
reject_reason=
score=
```

Rules:

- rejected candidates still print a reason in verbose diagnostics;
- selected candidate must be reproducible from gates and score;
- override may select only a candidate that passes hard gates.

## Device Creation Preconditions

Before `vkCreateDevice`:

- selected candidate passed hard gates;
- enabled device extension list is complete;
- enabled feature chain has only accepted features set true;
- queue create infos are built for selected queue families;
- portability subset is enabled when required;
- dynamic rendering source is selected;
- diagnostics record is prepared.

If any precondition fails, do not call `vkCreateDevice`.

## Diagnostics Receipt

Feature query smoke should print:

```text
feature_query_chain=modern_dynamic_rendering_v1
requested_api_version=
instance_api_version=
loader_version_source=
available_instance_extensions=
enabled_instance_extensions=
available_instance_layers=
enabled_instance_layers=
portability_enumeration_available=true|false
portability_enumeration_enabled=true|false
instance_create_enumerate_portability_bit=true|false
candidate_device_count=
selected_device_index=
selected_device_name=
selected_device_api_version=
selected_device_driver_version=
enabled_device_extensions=
dynamic_rendering_supported=true|false
dynamic_rendering_source=core_1_3|khr_extension|unavailable
dynamic_rendering_enabled=true|false
timeline_semaphore_available=true|false
timeline_semaphore_enabled=false
synchronization2_available=true|false
synchronization2_enabled=false
descriptor_indexing_available=true|false
descriptor_indexing_enabled=false
sampler_anisotropy_available=true|false
sampler_anisotropy_enabled=false
portability_subset_available=true|false
portability_subset_enabled=true|false
max_push_constants_size=
feature_query_clean=true|false
reason=
```

Rules:

- availability and enabled state must be separate fields;
- optional features should usually show `available=true enabled=false`;
- missing required features must show stable failure reasons.

## Failure Reason Codes

Use stable reason codes:

```text
feature_query_scope_blocked
instance_version_query_failed
instance_extension_query_failed
instance_required_extension_missing
validation_layer_required_missing
portability_enumeration_required_missing
surface_create_before_device_query_failed
physical_device_query_failed
device_extension_query_failed
device_required_extension_missing
device_feature_query_failed
dynamic_rendering_query_missing
dynamic_rendering_unsupported
dynamic_rendering_enable_chain_missing
dynamic_rendering_enabled_source_conflict
portability_subset_required_missing
portability_subset_enable_chain_missing
push_constant_limit_too_small
queue_family_query_failed
graphics_queue_missing
present_queue_missing
swapchain_support_query_failed
device_create_precondition_failed
logical_device_create_failed
feature_query_leak_to_runtime
```

Rules:

- reason codes should appear in smoke output;
- reason codes must not vary by platform;
- native Vulkan errors may be appended after stable reason codes.

## Validation Expectations

Validation should catch:

- enabling unsupported extensions;
- invalid `pNext` chain structs;
- missing required feature enablement for used commands;
- using dynamic rendering commands without enabling the feature/path;
- invalid portability subset handling;
- invalid device creation parameters.

Tests should catch:

- support chain and enable chain separation;
- optional features queried but disabled;
- dynamic rendering core path selected for Vulkan 1.3 device;
- KHR dynamic rendering path selected only when compatibility gate allows it;
- render-pass fallback blocked by default;
- runtime/content/projection Vulkan leakage.

Suggested firewall scan:

```sh
rg -n "vulkan/vulkan.h|Vk[A-Z]|VK_" src/runtime src/content src/projection src/runtime/save
```

Expected result: no production runtime/content/projection/save Vulkan leakage.

## Platform Notes

macOS/Apple portability:

- expect portability enumeration/subset to matter;
- query actual device API version and advertised extensions;
- do not infer dynamic rendering support from SDK headers;
- print portability fields in every device smoke receipt.

Linux:

- native Vulkan loader/ICD is the required shipping lane;
- software Vulkan may be a dev/CI lane only;
- print whether selected device is CPU/software.

Windows:

- native Vulkan runtime/SDK is the required shipping lane;
- multi-config shader paths do not affect feature chain;
- driver/device version must be printed in smoke.

## Tests

Expected future tests:

```text
tests/unit/render_feature_query_policy_tests.cpp
tests/unit/render_device_policy_tests.cpp
tests/smoke/vulkan_feature_baseline_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_surface_smoke.cpp
```

Unit tests should cover:

- required extension list assembly;
- optional feature availability vs enabled state;
- dynamic rendering source decision table;
- portability enumeration decision table;
- failure reason stability;
- render-pass fallback blocked by default.

Smoke tests should cover:

- at least one selected device receipt;
- dynamic rendering enabled receipt;
- portability receipt on macOS;
- strict validation layer missing failure;
- non-strict unsupported-device diagnostic skip;
- no Vulkan impact on headless runtime tests.

## Acceptance Criteria

Feature query chain work is acceptable only when:

- instance extensions and layers are enumerated before instance creation;
- portability enumeration is handled explicitly when needed;
- every physical device candidate gets extension, feature, queue, surface, and swapchain support queries;
- dynamic rendering is proven and enabled through exactly one accepted source;
- optional timeline/sync2/descriptor-indexing features are queried but disabled;
- enabled feature chain contains only accepted features;
- selected device diagnostics include API version, driver version, enabled extensions, enabled features, and reject reasons for failed candidates;
- runtime/content/projection/save scans show no Vulkan leakage;
- macOS, Linux, and Windows receipts use the same schema.

## Open Detail Items

The next detailed pass should define:

- exact C++ struct wrapper for support chain lifetime;
- exact C++ struct wrapper for enable chain lifetime;
- exact device-candidate record type;
- exact core-vs-KHR dynamic rendering function loading policy;
- exact portability subset feature/property structs supported by selected SDK headers;
- exact validation settings `pNext` interaction with instance creation;
- exact optional smoke exit code for unsupported device;
- exact verbose vs normal candidate diagnostics format.
