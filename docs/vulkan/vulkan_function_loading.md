# Vulkan Function Loading

This document defines how `iggy3d` loads Vulkan entry points and selects core vs extension function names.

Function loading is renderer-owned. Runtime/content/projection may receive backend-neutral diagnostics, but they never own function pointers, loader handles, dispatch tables, `PFN_vk*` types, `Vk*` handles, or `VK_*` constants.

## Purpose

Define a portable loading policy:

```text
load global/instance functions through the Vulkan loader
load instance and physical-device functions through vkGetInstanceProcAddr
load device functions through vkGetDeviceProcAddr after logical device creation
use core Vulkan 1.3 dynamic rendering names on core path
use KHR dynamic rendering names only on approved extension compatibility path
load debug utils instance functions only when VK_EXT_debug_utils is enabled
keep function tables scoped to the instance/device that loaded them
print function loading diagnostics
```

This document depends on [vulkan_feature_baseline.md](vulkan_feature_baseline.md), [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md), [device_selection.md](device_selection.md), [debug_validation.md](debug_validation.md), [command_recording.md](command_recording.md), and [platform_matrix.md](platform_matrix.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for exact function availability and dispatch behavior.
- Khronos Vulkan Loader application interface documentation for loader, instance, and device dispatch behavior.
- LunarG loader interface architecture for instance/device function loading guidance.
- `VK_KHR_dynamic_rendering` reference page for core Vulkan 1.3 vs `KHR` command names.
- Vulkan Guide `VK_EXT_debug_utils` page for debug messenger/object label loading pattern.
- VMA docs only when allocator dynamic function pointers are configured.
- How to Vulkan in 2026 as a secondary practical reference for Volk-style loading, SDL-driven extension discovery, dynamic rendering command loading, and VMA function handoff shape.

## Scope

In scope:

- global function bootstrapping;
- instance function loading;
- device function loading;
- dynamic rendering core-vs-KHR function name policy;
- debug utils function loading;
- swapchain function loading;
- VMA function handoff policy;
- loader behavior across macOS, Linux, and Windows;
- diagnostics and failure reasons.

Out of scope:

- platform window creation;
- feature/extension selection;
- shader compilation;
- command recording contents beyond function use;
- render-pass fallback implementation;
- runtime/gameplay API design.

## Local File Surface

Likely future files:

```text
src/render/vulkan/VulkanFunctions.hpp
src/render/vulkan/VulkanFunctions.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_vulkan_function_policy_tests.cpp
tests/smoke/vulkan_function_loading_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
```

If the first implementation links directly against the Vulkan loader and uses prototypes for core calls, this document still applies to extension functions and diagnostics. A later packet may decide whether a dedicated `VulkanFunctions` module is warranted.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| Vulkan loader access | platform/build plus Vulkan backend | linked loader or dynamic loader policy |
| global functions | backend bootstrap | available before instance |
| instance function table | `InstanceDeviceSurface` or `VulkanFunctions` | scoped to `VkInstance` |
| device function table | `InstanceDeviceSurface` or `VulkanFunctions` | scoped to `VkDevice` |
| debug utils functions | `DebugValidation` or instance table | require `VK_EXT_debug_utils` |
| dynamic rendering functions | device table | core or KHR path, not both as active path |
| swapchain functions | device table | `VK_KHR_swapchain` enabled |
| VMA function pointers | resource allocator setup | only when VMA dynamic function mode is used |

Rules:

- function pointers are backend-private;
- device-loaded pointers are valid only for the device used to load them;
- instance-loaded pointers are valid only for the instance/loader path used to load them;
- no renderer public header exposes `PFN_vk*`;
- no runtime/content/projection file includes function loading types.

## Loader Strategy

Initial acceptable strategy:

```text
link_to_vulkan_loader=true
use_vkGetInstanceProcAddr_for_instance_extension_functions=true
use_vkGetDeviceProcAddr_for_device_extension_functions=true
```

Deferred strategy:

```text
define_VK_NO_PROTOTYPES=false initially
manual_loader_open=false initially
volk_or_custom_dispatch_table=deferred
```

Rules:

- direct loader linking is acceptable for first Vulkan proof;
- extension functions still need explicit loading where prototypes/linking do not provide them portably;
- a custom loader or Volk-style dispatch table is a later performance/packaging decision;
- if `VK_NO_PROTOTYPES` is adopted later, all Vulkan calls must go through the owned function table.

## Load Order

Recommended order:

1. Make Vulkan loader available through link or dynamic library policy.
2. Resolve/use `vkGetInstanceProcAddr`.
3. Load/query global functions needed before instance if not using prototypes.
4. Create instance.
5. Load instance functions through `vkGetInstanceProcAddr`.
6. Load debug utils instance functions if `VK_EXT_debug_utils` is enabled.
7. Create debug messenger if enabled.
8. Create surface and select physical device.
9. Create logical device.
10. Load device functions through `vkGetDeviceProcAddr`.
11. Load swapchain functions if `VK_KHR_swapchain` is enabled.
12. Load dynamic rendering functions for the selected path.
13. Hand loader/device function pointers to VMA only if VMA dynamic functions are used.
14. Print function loading receipt.

Rules:

- do not load device functions before logical device creation;
- do not call debug utils functions before loading them;
- do not call dynamic rendering functions until feature/extension enablement has succeeded;
- do not keep old device function pointers after device destruction/recreation.

## Global Functions

Global/pre-instance functions:

```text
vkGetInstanceProcAddr
vkEnumerateInstanceVersion
vkEnumerateInstanceExtensionProperties
vkEnumerateInstanceLayerProperties
vkCreateInstance
```

Rules:

- if linking against the loader, these may be used through normal prototypes;
- if manual loading is adopted, `vkGetInstanceProcAddr` is the bootstrap function;
- missing `vkEnumerateInstanceVersion` is diagnosed by [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md);
- global function loading failure rejects Vulkan visual lane, not headless runtime.

## Instance Functions

Load instance and physical-device functions through `vkGetInstanceProcAddr`.

Expected instance functions include:

```text
vkDestroyInstance
vkEnumeratePhysicalDevices
vkGetPhysicalDeviceProperties
vkGetPhysicalDeviceProperties2
vkGetPhysicalDeviceFeatures2
vkGetPhysicalDeviceQueueFamilyProperties
vkGetPhysicalDeviceSurfaceSupportKHR
vkGetPhysicalDeviceSurfaceCapabilitiesKHR
vkGetPhysicalDeviceSurfaceFormatsKHR
vkGetPhysicalDeviceSurfacePresentModesKHR
vkEnumerateDeviceExtensionProperties
vkCreateDevice
vkDestroySurfaceKHR
```

Rules:

- instance functions are loaded after instance creation unless they are global functions;
- physical-device query functions belong to the instance loading phase;
- surface query functions require the relevant surface/WSI extensions to be enabled;
- missing required instance functions fail Vulkan startup with diagnostics.

## Device Functions

Load device functions through `vkGetDeviceProcAddr`.

Expected first device functions include:

```text
vkDestroyDevice
vkGetDeviceQueue
vkDeviceWaitIdle
vkCreateSwapchainKHR
vkDestroySwapchainKHR
vkGetSwapchainImagesKHR
vkAcquireNextImageKHR
vkQueuePresentKHR
vkCreateImageView
vkDestroyImageView
vkCreateShaderModule
vkDestroyShaderModule
vkCreatePipelineLayout
vkDestroyPipelineLayout
vkCreateGraphicsPipelines
vkDestroyPipeline
vkCreateCommandPool
vkDestroyCommandPool
vkAllocateCommandBuffers
vkResetCommandBuffer
vkBeginCommandBuffer
vkEndCommandBuffer
vkQueueSubmit
vkCreateSemaphore
vkDestroySemaphore
vkCreateFence
vkDestroyFence
vkWaitForFences
vkResetFences
vkCreateBuffer
vkDestroyBuffer
vkAllocateMemory
vkFreeMemory
vkBindBufferMemory
vkCreateImage
vkDestroyImage
vkBindImageMemory
vkCmdPipelineBarrier
vkCmdBindPipeline
vkCmdSetViewport
vkCmdSetScissor
vkCmdBindVertexBuffers
vkCmdBindIndexBuffer
vkCmdPushConstants
vkCmdDraw
vkCmdDrawIndexed
```

Rules:

- device functions are loaded after `vkCreateDevice`;
- device-loaded pointers are tied to the selected logical device;
- device function table is destroyed/invalidated before logical device destruction;
- device reload is required after device-loss recovery or logical device recreation.

## Dynamic Rendering Function Policy

Dynamic rendering selected by [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md) decides function names.

Core Vulkan 1.3 path:

```text
dynamic_rendering_source=core_1_3
begin_rendering_function=vkCmdBeginRendering
end_rendering_function=vkCmdEndRendering
rendering_info_struct=VkRenderingInfo
rendering_attachment_struct=VkRenderingAttachmentInfo
pipeline_rendering_struct=VkPipelineRenderingCreateInfo
```

KHR compatibility path:

```text
dynamic_rendering_source=khr_extension
begin_rendering_function=vkCmdBeginRenderingKHR
end_rendering_function=vkCmdEndRenderingKHR
rendering_info_struct=VkRenderingInfoKHR
rendering_attachment_struct=VkRenderingAttachmentInfoKHR
pipeline_rendering_struct=VkPipelineRenderingCreateInfoKHR
```

Rules:

- core 1.3 path is preferred;
- KHR path is allowed only when the compatibility gate is accepted;
- do not mix core and `KHR` dynamic rendering names in one active path;
- function loading diagnostics must print which names were loaded;
- missing selected dynamic rendering function fails Vulkan startup or command-recording setup;
- render-pass fallback is not selected here.

## Debug Utils Function Policy

Debug utils functions require `VK_EXT_debug_utils`.

Instance-loaded debug utils functions:

```text
vkCreateDebugUtilsMessengerEXT
vkDestroyDebugUtilsMessengerEXT
vkSubmitDebugUtilsMessageEXT optional
```

Device/command debug functions:

```text
vkSetDebugUtilsObjectNameEXT
vkCmdBeginDebugUtilsLabelEXT
vkCmdEndDebugUtilsLabelEXT
vkCmdInsertDebugUtilsLabelEXT
vkQueueBeginDebugUtilsLabelEXT optional
vkQueueEndDebugUtilsLabelEXT optional
```

Rules:

- debug messenger create/destroy functions are loaded with `vkGetInstanceProcAddr`;
- object naming and command labels are loaded only when debug utils is enabled;
- missing debug utils fails strict validation/debug lane;
- non-strict lane may run without labels/messenger if diagnostics say so;
- debug labels cannot be required for gameplay correctness.

## Swapchain Function Policy

Swapchain functions require `VK_KHR_swapchain`.

Load through `vkGetDeviceProcAddr` after logical device creation:

```text
vkCreateSwapchainKHR
vkDestroySwapchainKHR
vkGetSwapchainImagesKHR
vkAcquireNextImageKHR
vkQueuePresentKHR
```

Rules:

- missing swapchain function after enabling `VK_KHR_swapchain` is a Vulkan startup failure;
- swapchain functions are backend-private;
- headless runtime tests must not require swapchain functions;
- swapchain function loading does not select surface format or present mode.

## Portability Function Policy

Portability is mostly extension/feature/property query policy, not a broad function loading surface.

Rules:

- `VK_KHR_portability_enumeration` affects instance creation flags and physical-device enumeration;
- `VK_KHR_portability_subset` affects device extension enablement and optional feature/property structs;
- do not load Apple-only Metal functions in the cross-platform Vulkan backend;
- `VK_EXT_metal_objects` is out of scope unless a later Apple-specific packet explicitly adopts it;
- MoltenVK/KosmicKrisp differences are diagnostics, not runtime architecture forks.

## VMA Function Policy

VMA may use normal Vulkan prototypes or dynamic Vulkan functions depending on integration mode.

If VMA dynamic function mode is used, provide:

```text
vkGetInstanceProcAddr
vkGetDeviceProcAddr
required allocator function pointers
```

Rules:

- VMA function setup belongs to resource allocator initialization;
- VMA must use the same instance/device as the renderer;
- allocator function loading failure blocks resource-growth lane;
- first-room proof may avoid VMA only if [vulkan_feature_baseline.md](vulkan_feature_baseline.md) still allows the small manual allocation path.

## Function Table Lifetime

Suggested lifetime:

```text
GlobalFunctions: process/backend bootstrap lifetime
InstanceFunctions: instance lifetime
DeviceFunctions: logical device lifetime
DebugUtilsFunctions: instance/device lifetime depending on function
```

Rules:

- destroy debug messenger before instance function table becomes invalid;
- destroy swapchain/resources before device function table becomes invalid;
- set function table state to invalid after destroy;
- do not call loaded function pointers after device/instance teardown;
- device loss does not make runtime truth invalid, but it does invalidate device function usability once teardown begins.

## Core Vs Extension Name Table

Initial table:

| Feature | Core path | Extension path | Active path rule |
| --- | --- | --- | --- |
| Dynamic rendering begin | `vkCmdBeginRendering` | `vkCmdBeginRenderingKHR` | resolve from dynamic rendering source |
| Dynamic rendering end | `vkCmdEndRendering` | `vkCmdEndRenderingKHR` | resolve from dynamic rendering source |
| Rendering info | `VkRenderingInfo` | `VkRenderingInfoKHR` | match chosen function path |
| Pipeline rendering create info | `VkPipelineRenderingCreateInfo` | `VkPipelineRenderingCreateInfoKHR` | match chosen function path |
| Debug messenger | none | `vkCreateDebugUtilsMessengerEXT` | extension-only |
| Debug labels | none | `vkCmdBeginDebugUtilsLabelEXT` etc. | extension-only |
| Swapchain | none | `vkCreateSwapchainKHR` etc. | extension-only |

Rules:

- tables should be updated when sync2/timeline or maintenance functions are promoted into implementation;
- do not hide active path with untracked macro aliases;
- diagnostics must reveal active path.

## Platform Loader Notes

macOS:

- use the Vulkan SDK loader/portability implementation during development;
- packaged apps must follow SDK packaging rules from [packaging.md](packaging.md);
- MoltenVK and current Apple SDK implementations must still provide normal Vulkan loader entry points;
- portability devices may require portability enumeration before physical devices appear.

Linux:

- native loader path should use system Vulkan loader/ICD;
- CMake may link through `Vulkan::Vulkan` when available;
- `libvulkan.so.1` runtime availability is environment/platform validation;
- software Vulkan/lavapipe can be a dev lane only when labeled.

Windows:

- native loader path uses the Vulkan runtime/SDK loader;
- multi-config shader paths do not change function loading;
- missing `vulkan-1.dll` or SDK runtime is a platform failure for visual lane;
- do not bake debug/SDK-only loader paths into shipping assumptions.

## Diagnostics Receipt

Function loading smoke should print:

```text
function_loading=enabled
loader_strategy=linked_loader|manual_loader|volk|unknown
vk_get_instance_proc_addr_available=true|false
vk_get_device_proc_addr_available=true|false
instance_functions_loaded=true|false
device_functions_loaded=true|false
debug_utils_functions_loaded=true|false|not_requested
swapchain_functions_loaded=true|false
dynamic_rendering_source=core_1_3|khr_extension|unavailable
begin_rendering_function=vkCmdBeginRendering|vkCmdBeginRenderingKHR|unavailable
end_rendering_function=vkCmdEndRendering|vkCmdEndRenderingKHR|unavailable
core_khr_name_mix_detected=true|false
portability_function_policy=enumeration_and_subset_only
vma_dynamic_functions=enabled|disabled|not_configured
platform_loader_name=
function_loading_clean=true|false
reason=
```

Rules:

- receipt must separate function availability from feature enablement;
- selected dynamic rendering function names must match selected dynamic rendering source;
- optional debug labels may be unavailable in non-strict lanes only when diagnosed.

## Failure Reason Codes

Use stable reason codes:

```text
function_loading_scope_blocked
vulkan_loader_missing
vk_get_instance_proc_addr_missing
vk_get_device_proc_addr_missing
instance_function_missing
device_function_missing
debug_utils_function_missing
swapchain_function_missing
dynamic_rendering_function_missing
dynamic_rendering_core_khr_mismatch
dynamic_rendering_function_loaded_before_feature_enabled
debug_utils_loaded_without_extension
swapchain_loaded_without_extension
device_function_loaded_before_device_create
function_table_used_after_destroy
vma_function_table_incomplete
function_loading_leak_to_runtime
```

Rules:

- reason codes should appear in smoke output;
- reason codes must not vary by platform;
- native loader errors can be appended after stable reason codes.

## Validation Expectations

Validation may catch:

- commands used without feature/extension enablement;
- debug utils calls made without valid objects;
- swapchain calls with invalid handles;
- dynamic rendering use without proper feature enablement.

Tests should catch:

- core/KHR dynamic rendering mismatch;
- loading device functions before device creation;
- missing debug utils fallback behavior;
- function table invalidation after destroy;
- runtime/content/projection Vulkan leakage.

Suggested firewall scan:

```sh
rg -n "PFN_vk|vkGetInstanceProcAddr|vkGetDeviceProcAddr|vkCmdBeginRendering|vkCmdBeginRenderingKHR|vulkan/vulkan.h|Vk[A-Z]|VK_" src/runtime src/content src/projection src/runtime/save
```

Expected result: no production runtime/content/projection/save Vulkan leakage.

## Tests

Expected future tests:

```text
tests/unit/render_vulkan_function_policy_tests.cpp
tests/unit/render_feature_query_policy_tests.cpp
tests/smoke/vulkan_function_loading_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Unit tests should cover:

- active dynamic rendering function name decision;
- extension function requested only when extension enabled;
- optional debug utils behavior;
- reason code stability;
- function table lifetime state transitions.

Smoke tests should cover:

- loader available;
- instance functions loaded;
- device functions loaded;
- swapchain functions loaded;
- dynamic rendering selected function loaded;
- debug utils loaded when strict validation requires it;
- non-strict debug utils unavailable receipt.

## Acceptance Criteria

Function loading work is acceptable only when:

- instance functions are loaded through instance loader path;
- device functions are loaded through `vkGetDeviceProcAddr` after device creation;
- selected dynamic rendering function names match selected feature path;
- debug utils functions are loaded only when `VK_EXT_debug_utils` is enabled;
- swapchain functions are loaded only when `VK_KHR_swapchain` is enabled;
- device function tables are invalidated on device teardown;
- diagnostics print active loader strategy and loaded dynamic rendering function names;
- macOS, Linux, and Windows receipts use the same schema;
- runtime/content/projection/save scans show no function pointer or Vulkan leakage.

## Open Detail Items

The next detailed pass should define:

- whether first implementation links directly to `Vulkan::Vulkan` or uses manual loader opening;
- whether to introduce `VulkanFunctions.hpp` immediately or wait until KHR compatibility is implemented;
- exact C++ storage type for function tables;
- exact wrapper names for dynamic rendering begin/end;
- exact behavior if core 1.3 function names are unavailable despite feature support;
- whether VMA uses static prototypes or dynamic function table;
- whether RenderDoc/GFXReconstruct marker functions require a separate debug-label packet.
