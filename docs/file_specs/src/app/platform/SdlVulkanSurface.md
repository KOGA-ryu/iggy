# File Spec

Files: `src/app/platform/SdlVulkanSurface.hpp`, `src/app/platform/SdlVulkanSurface.cpp`

Verified at: `b50008a5`

## Owns

- SDL-backed Vulkan platform surface provider.
- Querying, sorting, and de-duplicating SDL-required Vulkan instance extension names.
- Creating `VkSurfaceKHR` from an `SdlWindow` and `VkInstance`.
- Render outcome and reason packets for unsupported or successful surface paths.

## Does Not Own

- SDL window lifetime.
- Vulkan instance/device/swapchain ownership.
- Surface destruction after backend ownership transfer.
- Product renderer readiness, menu state, or receipt field emission.

## Reads

- `SdlWindow` open/drawable status and native SDL window handle.
- SDL Vulkan extension list and surface creation result.
- Caller-provided Vulkan instance handle.

## Writes / Mutates

- Returns `SdlVulkanExtensionList`.
- Returns `SdlVulkanSurfaceCreateResult`.
- Does not mutate `SdlWindow`, renderer state, or app state.

## Calls Out To / Wires Out To

- SDL Vulkan APIs.
- `RendererLifecycle.cpp` consumes extension names and passes a create-surface callback into Vulkan backend bootstrap.
- Vulkan smoke tests use the provider directly.

## Called By / Entry Points

- `SdlVulkanSurfaceProvider::requiredInstanceExtensions(...)`.
- `SdlVulkanSurfaceProvider::createSurface(...)`.
- Focused proof: `rg -n "SdlVulkanSurfaceProvider|requiredInstanceExtensions|createSurface\\(" src tests`.

## Invariants

- Missing window, missing native handle, missing extension names, non-drawable window, or null instance returns unsupported with an explicit reason.
- Extension names are sorted and unique before being returned.
- Surface creation success returns `RenderOutcome::Ok` and a non-null surface.
- This provider bridges SDL to Vulkan only; it must not absorb renderer lifecycle or product policy.

## Tests / Proof Commands

- `rg -n "SdlVulkanSurfaceProvider|SdlVulkanExtensionList|SdlVulkanSurfaceCreateResult" tests/smoke src/app`.
- `rg -n "vulkan_platform_smoke|vulkan_swapchain_smoke|vulkan_pipeline_smoke" cmake/iggy3d_tests.cmake tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/platform/SdlWindow.*` unless native-window or drawable contracts change.
- `src/app/iggy3d/window/RendererLifecycle.*` unless backend bootstrap wiring changes.
- `src/render/vulkan/*` unless Vulkan backend surface ownership changes.

## Update When

- SDL Vulkan extension handling, surface creation, failure reason strings, or surface-provider ownership changes.

## Do Not Update When

- Only swapchain, command buffer, frame scheduling, or product receipt behavior changes without changing SDL Vulkan surface creation.
