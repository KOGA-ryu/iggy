# Vulkan Surface WSI Platforms

This document defines how `iggy3d` creates and validates Vulkan presentation surfaces across macOS, Linux, and Windows.

WSI surface work is platform/render glue. It is not runtime authority, scene authority, camera truth, save truth, replay truth, or gameplay input policy. Runtime/content/projection must not own `VkSurfaceKHR`, platform WSI extensions, native window handles, SDL/GLFW objects, or presentation support decisions.

## Purpose

Define the platform WSI contract:

```text
platform shell creates a Vulkan-capable window
platform shell reports Vulkan instance extensions before instance creation
Vulkan backend creates instance with platform WSI extensions enabled
platform shell creates VkSurfaceKHR for that instance/window
Vulkan backend owns and destroys VkSurfaceKHR
device selection checks present support against that surface
swapchain owns presentation images after device/surface support is proven
platform receipts identify macOS, Linux, and Windows WSI path
```

This document narrows [platform_shell.md](platform_shell.md), [platform_matrix.md](platform_matrix.md), [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md), [vulkan_function_loading.md](vulkan_function_loading.md), [device_selection.md](device_selection.md), [swapchain_contract.md](swapchain_contract.md), and [lifetime.md](lifetime.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification WSI chapter for `VK_KHR_surface`, platform WSI extensions, `VkSurfaceKHR`, and presentation-support behavior.
- SDL3 Vulkan docs for `SDL_Vulkan_GetInstanceExtensions` and `SDL_Vulkan_CreateSurface`.
- GLFW Vulkan docs only if fallback is accepted.
- Khronos MoltenVK for `VK_EXT_metal_surface`, Metal layer requirements, and portability enumeration/subset behavior: https://github.com/KhronosGroup/MoltenVK/
- LunarG macOS Vulkan SDK guide for Apple loader/runtime packaging expectations: https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html
- Vulkan portability enumeration refpage for `VK_KHR_portability_enumeration`: https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_enumeration.html
- Vulkan Tutorial window surface chapter for first implementation order and present-queue reasoning.
- How to Vulkan in 2026 as a secondary practical reference for SDL-based instance extension discovery, surface creation, and presentation-support checks across desktop lanes.

## Scope

In scope:

- SDL3 first surface path;
- GLFW fallback surface path;
- macOS/MoltenVK Metal surface expectations;
- Linux X11/Wayland expectations through SDL3/GLFW;
- Windows Win32 surface expectations through SDL3/GLFW;
- required instance extension handling;
- `VkSurfaceKHR` ownership/lifetime;
- present support query preconditions;
- platform diagnostics and failure reasons.

Out of scope:

- runtime input handling;
- native Cocoa/X11/Wayland/Win32 surface code in first implementation;
- swapchain format/present-mode policy;
- command recording;
- renderer public API design;
- editor windows or multi-window support;
- headless/offscreen rendering.

## Local File Surface

Likely future files:

```text
apps/iggy3d_visual_demo/main.cpp
src/app/platform/SdlWindow.hpp
src/app/platform/SdlWindow.cpp
src/app/platform/SdlVulkanSurface.hpp
src/app/platform/SdlVulkanSurface.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_surface_policy_tests.cpp
tests/smoke/vulkan_platform_smoke.cpp
tests/smoke/vulkan_surface_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
```

If GLFW fallback is accepted later:

```text
src/app/platform/GlfwWindow.hpp
src/app/platform/GlfwWindow.cpp
src/app/platform/GlfwVulkanSurface.hpp
src/app/platform/GlfwVulkanSurface.cpp
tests/smoke/vulkan_glfw_surface_smoke.cpp
```

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| SDL/GLFW window | platform shell | must outlive Vulkan surface |
| required platform instance extensions | platform shell provider | queried before instance creation |
| renderer-required instance extensions | Vulkan backend | debug utils, portability enumeration |
| Vulkan instance | Vulkan backend | created after extension list is merged |
| `VkSurfaceKHR` | Vulkan backend | created via shell provider, destroyed by backend |
| native Cocoa/X11/Wayland/Win32 handles | platform library | not exposed to runtime/projection |
| present support result | Vulkan backend device selection | tied to physical device, queue family, surface |
| swapchain | Vulkan backend swapchain module | created after surface/device support |

Rules:

- platform shell owns the native window lifetime;
- Vulkan backend owns `VkSurfaceKHR` destruction with `vkDestroySurfaceKHR`;
- surface must be destroyed before the Vulkan instance;
- SDL/GLFW window must not be destroyed while surface/swapchain still use it;
- runtime/content/projection never see surface handles or native window handles.

## First Platform Decision

First path:

```text
window_surface_library=SDL3
surface_provider=SdlVulkanSurface
native_surface_code=deferred
```

Fallback:

```text
window_surface_library=GLFW
fallback_allowed_only_if=SDL3_dependency_or_surface_creation_blocks_visual_proof
```

Rejected for first implementation:

```text
hand_written_Cocoa_surface
hand_written_Xlib_or_XCB_surface
hand_written_Wayland_surface
hand_written_Win32_surface
```

Rules:

- SDL3 remains the proposed default until platform smoke accepts or rejects it;
- GLFW fallback requires a decision update and its own diagnostics;
- native WSI code is too much platform surface before renderer boundary is proven.

## SDL3 WSI Contract

SDL3 responsibilities:

```text
create SDL window with Vulkan capability
return required Vulkan instance extensions
create VkSurfaceKHR for existing VkInstance and SDL_Window
report platform/window errors as platform diagnostics
```

Required order:

1. Initialize SDL video.
2. Create `SDL_Window` with Vulkan capability.
3. Query `SDL_Vulkan_GetInstanceExtensions`.
4. Merge SDL extensions with renderer-required instance extensions.
5. Create Vulkan instance.
6. Call SDL surface creation for that instance/window.
7. Transfer returned `VkSurfaceKHR` ownership to Vulkan backend.
8. Device selection checks present support against that surface.

Rules:

- SDL extension query happens before Vulkan instance creation;
- the returned SDL extension list is platform data, not runtime data;
- duplicate extension names must be de-duplicated before `vkCreateInstance`;
- SDL errors become renderer/platform diagnostics;
- SDL window pointer must not be exposed through generic renderer API.

Diagnostics:

```text
surface_library=SDL3
sdl_vulkan_extensions_available=true|false
sdl_required_instance_extensions=
sdl_surface_create_result=
```

## GLFW Fallback Contract

GLFW fallback may be used only after SDL3 is blocked.

GLFW responsibilities:

```text
create window with no client API
return required Vulkan instance extensions through glfwGetRequiredInstanceExtensions
create VkSurfaceKHR through glfwCreateWindowSurface
report Vulkan loader and platform errors
```

Rules:

- GLFW fallback must preserve the same renderer/runtime firewall;
- `glfwGetRequiredInstanceExtensions` output is merged like SDL output;
- `glfwCreateWindowSurface` failure is a platform surface failure;
- GLFW does not destroy the surface; Vulkan backend still destroys `VkSurfaceKHR`;
- fallback must update [decisions.md](decisions.md), [platform_shell.md](platform_shell.md), and smoke receipts.

Diagnostics:

```text
surface_library=GLFW
glfw_vulkan_supported=true|false
glfw_required_instance_extensions=
glfw_surface_create_result=
```

## Required WSI Extension Categories

All visual surface paths require:

```text
VK_KHR_surface
one platform-specific surface extension
```

Likely platform-specific extensions when using native paths or library-reported extensions:

```text
macOS/Apple: VK_EXT_metal_surface
Linux X11: VK_KHR_xlib_surface or VK_KHR_xcb_surface
Linux Wayland: VK_KHR_wayland_surface
Windows: VK_KHR_win32_surface
```

Rules:

- do not hardcode the exact SDL/GLFW extension list as the only allowed list;
- ask the platform library for required instance extensions;
- verify every requested extension is present before instance creation;
- print the actual enabled extension list;
- platform-specific WSI extensions are instance extensions.

## Extension Merge Policy

Extension inputs:

```text
platform_required_instance_extensions
renderer_debug_instance_extensions
renderer_portability_instance_extensions
```

Merge rules:

- preserve all platform-required extensions;
- add `VK_EXT_debug_utils` only when debug messenger is requested;
- add `VK_KHR_portability_enumeration` when Apple portability enumeration is needed and available;
- de-duplicate exact string matches;
- fail strict surface smoke if a required extension is unavailable;
- print missing extensions by category.

Diagnostics:

```text
platform_required_instance_extensions=
renderer_required_instance_extensions=
enabled_instance_extensions=
missing_platform_instance_extensions=
missing_renderer_instance_extensions=
```

## macOS / MoltenVK WSI

macOS first lane:

```text
surface_backend=Metal surface through MoltenVK/current Apple Vulkan SDK implementation
expected_platform_extension=VK_EXT_metal_surface through SDL3/GLFW reported extensions
portability_enumeration=required_when_needed
portability_subset=required_when_advertised
```

Rules:

- MoltenVK uses Metal underneath but the renderer remains Vulkan-facing;
- `VK_EXT_metal_surface` is the expected visible surface extension for current MoltenVK paths;
- obsolete `VK_MVK_macos_surface` is not the preferred new path;
- portability enumeration may be required for MoltenVK physical devices to appear;
- Metal/CAMetalLayer details stay inside platform library or Apple-specific platform glue;
- do not design cross-platform renderer behavior around Apple-only quirks.

Required macOS receipt fields:

```text
platform=macOS
surface_backend=metal
surface_extension=VK_EXT_metal_surface|reported_by_library
portability_enumeration_enabled=true|false
portability_subset_enabled=true|false
moltenvk_or_apple_vulkan_device_detected=true|false
```

## Linux X11 / Wayland WSI

Linux first lane:

```text
surface_backend=SDL3 reported X11/Wayland surface
native_vulkan_required=true
software_vulkan=optional_dev_only
```

Expected possible extensions:

```text
VK_KHR_xlib_surface
VK_KHR_xcb_surface
VK_KHR_wayland_surface
```

Rules:

- do not select X11 vs Wayland in renderer code;
- SDL3/GLFW reports the required surface extension for the active platform backend;
- diagnostics must print which surface extension was enabled;
- Linux native Vulkan smoke must not be replaced by lavapipe/software proof for shipping;
- if display/session lacks required WSI support, optional smoke may skip with receipt and strict smoke must fail.

Required Linux receipt fields:

```text
platform=Linux
surface_backend=x11|xcb|wayland|unknown
surface_extension=
native_vulkan_driver=true|false
software_vulkan=true|false
display_server=
```

## Windows Win32 WSI

Windows first lane:

```text
surface_backend=SDL3 reported Win32 surface
expected_platform_extension=VK_KHR_win32_surface
native_vulkan_required=true
```

Rules:

- Win32 handles stay inside SDL3/GLFW or platform glue;
- renderer sees only `VkSurfaceKHR`;
- Windows multi-config shader paths do not affect WSI;
- missing Vulkan runtime/loader is a platform failure for visual smoke;
- strict smoke requires native Vulkan runtime and surface creation.

Required Windows receipt fields:

```text
platform=Windows
surface_backend=win32
surface_extension=VK_KHR_win32_surface|reported_by_library
native_vulkan_runtime=true|false
```

## Surface Creation Contract

Surface provider behavior:

```text
requiredInstanceExtensions() -> list of extension names
createSurface(instance) -> VkSurfaceKHR or diagnosed failure
```

Rules:

- extension query must be callable before Vulkan instance creation;
- surface creation requires a valid Vulkan instance;
- surface creation requires the platform window to still exist;
- returned `VkSurfaceKHR` becomes backend-owned;
- surface creation failure must include platform library error text when available;
- surface provider must not create logical device or swapchain.

Containment rule:

- any interface that exposes `VkInstance`, `VkSurfaceKHR`, or `VkAllocationCallbacks` must live under `src/render/vulkan/**` or Vulkan-specific app glue, not public runtime/projection headers.

## Present Support Contract

After surface creation and before device acceptance:

```text
query physical device queue families
query present support for each candidate queue family against VkSurfaceKHR
require at least one graphics queue
require at least one present queue for the surface
prefer same graphics/present queue for first implementation
```

Rules:

- surface can influence physical device selection;
- do not select a physical device before present support is known;
- present support must be queried per surface, not assumed from device name;
- macOS abstraction may report present support differently through the window library, but device selection still records the result it used.

Diagnostics:

```text
graphics_queue_family=
present_queue_family=
same_graphics_present_queue=true|false
present_support_checked=true|false
present_support_result=true|false
```

## Lifetime And Teardown

Required teardown order:

1. Stop submitting new frames.
2. Wait for device/frame work as required by [lifetime.md](lifetime.md).
3. Destroy swapchain and swapchain image views.
4. Destroy `VkSurfaceKHR`.
5. Destroy logical device children.
6. Destroy logical device.
7. Destroy debug messenger.
8. Destroy Vulkan instance.
9. Destroy platform window.
10. Shutdown SDL/GLFW.

Rules:

- surface must outlive swapchain;
- surface must not outlive instance;
- platform window must outlive surface;
- renderer must not present after surface teardown begins;
- resize/minimize changes do not mutate runtime truth.

## Resize / Minimize Interaction

Platform shell must report:

```text
drawable_width
drawable_height
window_minimized=true|false
resize_pending=true|false
```

Rules:

- zero drawable size means no swapchain creation or presentation;
- minimized windows should produce diagnosed frame skip, not runtime mutation;
- resize event marks swapchain dirty;
- actual swapchain extent is selected by [swapchain_contract.md](swapchain_contract.md);
- platform event storm must not create/destroy swapchain every raw event if a coalesced recreate is possible.

## Diagnostics Receipt

Surface WSI smoke should print:

```text
surface_wsi=enabled
surface_library=SDL3|GLFW|native|unknown
platform=macOS|Linux|Windows|unknown
surface_backend=metal|x11|xcb|wayland|win32|unknown
platform_required_instance_extensions=
renderer_required_instance_extensions=
enabled_instance_extensions=
surface_extension=
surface_created=true|false
surface_handle_valid=true|false
present_support_checked=true|false
present_support_result=true|false
graphics_queue_family=
present_queue_family=
same_graphics_present_queue=true|false
drawable_extent=
window_minimized=true|false
portability_enumeration_enabled=true|false|not_applicable
portability_subset_enabled=true|false|not_applicable
native_vulkan_required=true|false
software_vulkan=true|false
reason=
```

Rules:

- all platforms use the same field names;
- platform-specific values may differ;
- optional smoke may skip with reason when display/loader/surface is unavailable;
- strict smoke fails when required WSI support is missing.

## Failure Reason Codes

Use stable reason codes:

```text
surface_wsi_scope_blocked
surface_library_unavailable
surface_window_create_failed
surface_required_extension_query_failed
surface_required_extension_missing
surface_extension_merge_failed
surface_instance_missing_required_extension
surface_create_failed
surface_null_handle
surface_platform_not_supported
surface_present_support_query_failed
surface_present_support_missing
surface_zero_drawable_extent
surface_window_destroyed_before_surface
surface_destroy_order_invalid
surface_sdl_blocked
surface_glfw_fallback_unapproved
surface_native_wsi_unapproved
surface_portability_enumeration_missing
surface_runtime_leak
```

Rules:

- reason codes must appear in smoke output;
- reason codes must not vary by platform;
- platform-native error strings may be appended after stable reason codes.

## Validation Expectations

Validation should catch:

- creating surface without required instance extension;
- using destroyed surface in swapchain creation;
- destroying instance before surface;
- swapchain creation with unsupported surface/device pair;
- presenting with invalid swapchain/surface state.

Tests should catch:

- required extension query before instance creation;
- extension merge/de-dup behavior;
- surface ownership and teardown order;
- present support query before device selection acceptance;
- SDL vs GLFW fallback decision policy;
- runtime/content/projection leakage.

Suggested firewall scan:

```sh
rg -n "SDL_Window|SDL_Vulkan|GLFWwindow|glfwCreateWindowSurface|VkSurfaceKHR|VK_KHR_surface|VK_KHR_xlib_surface|VK_KHR_xcb_surface|VK_KHR_wayland_surface|VK_KHR_win32_surface|VK_EXT_metal_surface" src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result: no production runtime/content/projection/save/public-render leakage.

## Tests

Expected future tests:

```text
tests/unit/render_surface_policy_tests.cpp
tests/unit/render_platform_extension_merge_tests.cpp
tests/smoke/vulkan_platform_smoke.cpp
tests/smoke/vulkan_surface_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
```

Unit tests should cover:

- extension merge/de-dup;
- missing platform extension failure;
- fallback policy rejects GLFW unless accepted;
- lifetime state transitions;
- failure reason stability.

Smoke tests should cover:

- SDL3 surface creation;
- instance extension receipt;
- surface creation receipt;
- present support receipt;
- zero-size/minimized skip receipt;
- strict platform failure when WSI is unavailable.

## Acceptance Criteria

Surface WSI work is acceptable only when:

- SDL3 path queries required instance extensions before instance creation;
- Vulkan instance enables platform-required WSI extensions;
- surface creation returns a valid `VkSurfaceKHR`;
- Vulkan backend owns surface destruction;
- physical device selection checks present support against the created surface;
- swapchain creation is blocked until surface/device support is proven;
- macOS prints Metal/portability fields;
- Linux prints X11/XCB/Wayland surface backend when known;
- Windows prints Win32 surface backend when known;
- optional and strict smoke lanes produce the expected skip/fail behavior;
- runtime/content/projection/save scans show no WSI or Vulkan surface leakage.

## Open Detail Items

The next detailed pass should define:

- exact SDL3 wrapper method signatures;
- exact Vulkan surface provider interface location;
- exact platform receipt detection for X11 vs Wayland under SDL3;
- exact optional smoke skip code for missing display;
- exact strict smoke CTest labels per platform;
- whether GLFW fallback is worth implementing immediately or only after SDL3 failure;
- exact teardown owner for surface when renderer initialization fails halfway;
- exact macOS behavior if a future SDK replaces or layers over MoltenVK.
