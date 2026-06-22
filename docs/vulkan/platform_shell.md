# Platform Shell

This document defines the first visual app shell: window creation, event polling, input routing, resize handling, and Vulkan surface handoff.

The proposed shell is SDL3. This is not accepted until Phase 2 platform smoke passes on the local macOS/MoltenVK lane and remains viable for Linux and Windows.

Packet 1 does not include SDL, window creation, platform input, or Vulkan surface work. Those concerns begin at the visual app/platform shell packet after the backend-neutral renderer boundary and null renderer shape are usable.

## Decision Status

Decision: window/surface library

Status: `Proposed`

Recommended default: SDL3 shell isolated in app/platform glue.

Fallback: GLFW if SDL3 dependency acquisition or Vulkan surface creation blocks the first visual proof.

Rejected for first implementation: native Cocoa/X11/Wayland/Win32 surface code in the app. Native platform code is too much surface area before the renderer boundary is proven.

Primary sources:

- SDL3 Vulkan category: https://wiki.libsdl.org/SDL3/CategoryVulkan
- SDL Vulkan surface creation: https://wiki.libsdl.org/SDL_Vulkan_CreateSurface
- GLFW Vulkan guide if fallback is needed: https://www.glfw.org/docs/3.3/vulkan_guide.html

## Local File Surface

Likely future paths:

```text
apps/iggy3d_visual_demo/main.cpp
src/app/platform/SdlWindow.hpp
src/app/platform/SdlWindow.cpp
src/app/platform/SdlVulkanSurface.hpp
src/app/platform/SdlVulkanSurface.cpp
src/render/RendererApi.hpp
src/render/RenderBackend.hpp
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/InstanceDeviceSurface.hpp
tests/smoke/vulkan_platform_smoke.cpp
```

These paths are proposals for file planning. This document does not implement them.

## Ownership

The visual app owns:

- process entry point;
- CLI flags for visual mode;
- SDL initialization and shutdown;
- window lifetime;
- event polling;
- translating input events into runtime commands or camera/input controller requests;
- building `FrameInput` from runtime/projection state;
- choosing when to call renderer frame submission.

The platform shell owns:

- SDL window wrapper;
- framebuffer size query;
- minimized/focused/quit state;
- SDL event decoding into backend-neutral app events;
- Vulkan surface creation callback for an existing Vulkan instance.

The Vulkan backend owns:

- Vulkan instance;
- physical/logical device;
- `VkSurfaceKHR` after creation;
- swapchain;
- presentation;
- resize/recreate lifecycle;
- wait-idle behavior;
- Vulkan diagnostics.

Runtime owns:

- gameplay truth;
- command legality;
- camera mode truth;
- save/load and replay determinism.

Projection owns:

- backend-neutral scene and debug outputs.

## Dependency Rules

Allowed SDL includes:

- `apps/iggy3d_visual_demo/**`;
- `src/app/platform/SdlWindow.*`;
- `src/app/platform/SdlVulkanSurface.*`;
- platform smoke tests.

Allowed Vulkan includes:

- `src/render/vulkan/**`;
- `src/app/platform/SdlVulkanSurface.*` only if needed to call SDL's Vulkan surface API;
- Vulkan-specific smoke tests.

Forbidden:

- `src/runtime/**` including SDL or Vulkan;
- `src/content/**` including SDL or Vulkan;
- `src/projection/**` including SDL or Vulkan;
- `src/runtime/save/**` including SDL or Vulkan;
- `src/render/FrameInput.hpp` including SDL or Vulkan;
- `src/render/RendererApi.hpp` exposing SDL or Vulkan concrete types unless a later decision explicitly accepts that public dependency.

Firewall scans:

```sh
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan)' src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
rg -n '#include[ <"]vulkan/|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result: no matches.

## Shell Shape

Recommended first shell split:

```text
SdlWindow
  owns SDL initialization/window state/event polling

SdlVulkanSurface
  owns SDL Vulkan extension query and surface creation callback

iggy3d_visual_demo
  wires runtime, projection, platform shell, and renderer API

VulkanBackend
  owns Vulkan instance/device/surface/swapchain and rendering
```

Do not put SDL event logic inside the Vulkan backend. Do not put Vulkan swapchain logic inside the SDL window wrapper.

## Proposed App Flow

Initial visual demo flow:

1. Parse visual demo CLI flags.
2. Load the first-room fixture through the same public runtime/content APIs used by headless tools.
3. Create runtime session.
4. Initialize SDL video.
5. Create an SDL window with Vulkan capability.
6. Create renderer API/backend configuration.
7. Renderer backend creates Vulkan instance.
8. Renderer backend asks platform surface hook for required instance extensions before instance creation.
9. Renderer backend asks platform surface hook to create a surface after instance creation.
10. Renderer backend owns the returned surface handle.
11. Each loop iteration polls SDL events.
12. App translates input into runtime commands or camera controller requests.
13. Runtime ticks through authoritative APIs.
14. Projection builds `SceneProjectionResult` and `DebugProjectionResult`.
15. App assembles `FrameInput`.
16. Renderer consumes `FrameInput`.
17. On quit, app calls renderer wait-idle/shutdown, then destroys shell/window.

The ordering around extension query and instance creation matters: required instance extensions must be queried before the Vulkan instance exists; surface creation needs the created instance.

## Surface Creation Contract

Recommended contract: callback-based Vulkan surface provider isolated to Vulkan/platform glue.

The Vulkan backend should not depend directly on `SDL_Window`. The SDL shell should not own Vulkan device/swapchain lifecycle.

Proposed shape for future file planning:

```cpp
namespace iggy3d {

struct VulkanInstanceExtensionList {
  std::vector<const char*> names;
};

struct VulkanSurfaceCreateRequest {
  VkInstance instance = VK_NULL_HANDLE;
  const VkAllocationCallbacks* allocator = nullptr;
};

struct VulkanSurfaceCreateResult {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  std::string errorCode;
  std::string message;
};

class VulkanSurfaceProvider {
 public:
  virtual ~VulkanSurfaceProvider() = default;
  virtual VulkanInstanceExtensionList requiredInstanceExtensions() const = 0;
  virtual VulkanSurfaceCreateResult createSurface(const VulkanSurfaceCreateRequest& request) = 0;
};

}  // namespace iggy3d
```

Important containment rule: this exact interface belongs under `src/render/vulkan/**` or Vulkan-specific app glue because it exposes Vulkan types. It must not become part of the generic runtime/projection contract.

Alternative if public `RendererApi` must stay completely Vulkan-free:

- keep `VulkanSurfaceProvider` private to `VulkanBackend`;
- construct the Vulkan backend from the visual app's Vulkan-specific build path;
- expose only backend-neutral renderer selection through `RendererApi`;
- revisit once the app/backend factory shape is planned.

Surface lifetime:

- app/platform shell owns the SDL window;
- SDL surface creation function creates the `VkSurfaceKHR`;
- Vulkan backend owns destroying the `VkSurfaceKHR`;
- surface must be destroyed before the Vulkan instance is destroyed;
- SDL window must outlive the Vulkan surface;
- renderer wait-idle must happen before surface/swapchain teardown.

Failure handling:

- missing required instance extensions is a platform smoke failure;
- surface creation failure returns a machine-readable renderer/platform diagnostic;
- visual app exits nonzero in strict smoke mode;
- normal local smoke may skip only when configured to skip missing display/GPU requirements.

## Required Instance Extensions

The platform shell must provide required instance extensions before instance creation.

SDL3 path:

- query SDL for required Vulkan instance extensions for the window/platform;
- pass the extension names into `InstanceDeviceSurface`;
- Vulkan backend appends validation/debug/portability extensions as needed;
- diagnostics print both platform-required and renderer-required extension lists.

Rules:

- platform shell does not decide Vulkan feature policy;
- renderer backend does not hardcode platform WSI extension names when SDL can provide them;
- MoltenVK portability extensions are backend/platform diagnostics, not runtime truth.

Needs later file-plan detail:

- exact SDL3 function signature used by the installed SDL version;
- memory/lifetime of returned extension strings;
- merge/deduplicate policy for extension lists;
- strict failure codes.

## Event Loop Contract

The platform shell decodes SDL events into app-level events. Runtime command creation remains outside the shell.

Recommended app event categories:

```text
QuitRequested
WindowResized
WindowMinimized
WindowRestored
WindowFocusGained
WindowFocusLost
KeyPressed
KeyReleased
PointerMoved
PointerButtonPressed
PointerButtonReleased
ControllerButtonPressed
ControllerButtonReleased
ControllerAxisChanged
```

Rules:

- SDL raw events are not persisted as gameplay commands;
- app/controller code maps app events to runtime commands;
- runtime command admission decides legality;
- camera mode changes go through runtime camera policy;
- renderer receives only derived `FrameInput` and resize/presentation notifications.

## Resize And Minimize Contract

Resize handling:

- platform shell records framebuffer size changes;
- app tells renderer that drawable size changed;
- renderer marks swapchain dirty;
- renderer recreates swapchain at a safe point;
- runtime state is unchanged by resize.

Minimize handling:

- zero framebuffer width or height means not drawable;
- renderer skips presentation or waits according to platform policy;
- app continues processing quit/restore events;
- runtime ticking during minimized state is an app policy decision, not renderer authority.

Restore handling:

- platform shell reports nonzero framebuffer size;
- renderer recreates swapchain if needed;
- next valid `FrameInput` can draw.

Focus handling:

- focus loss may suppress raw look/controller deltas in app input code;
- runtime camera mode truth is not changed merely because the window lost focus;
- if runtime policy needs input clear, it goes through camera/input policy, not SDL directly.

Wait-idle points:

- before swapchain teardown/recreate when required;
- before renderer shutdown;
- before destroying the SDL window if a surface/swapchain still exists.

## Input To Runtime Commands

The platform shell does not create gameplay truth. It only reports input facts to app/controller code.

Allowed route:

```text
SDL event -> AppEvent -> input/controller mapping -> runtime command -> command admission -> SessionState -> projection -> FrameInput -> renderer
```

Forbidden route:

```text
SDL event -> renderer -> mutable runtime state
```

First visual demo may use a very small input set:

- quit;
- move/turn if runtime command APIs exist;
- camera mode toggle through runtime camera policy;
- pause/step/resume if wired for visual proof;
- debug overlay toggle as app/render presentation setting only.

Debug overlay toggles must not affect replay hash.

Packet split:

- Packet 1 has no SDL, input, window, or event polling work;
- Packet 3 visual boot may start in scripted/fixed-frame mode with only quit/shutdown handling if dependency or input mapping decisions are still open;
- interactive keyboard, mouse, or gamepad mapping is a separate Packet 3 acceptance choice and must route through runtime commands or camera policy, never through renderer mutation.

## CLI Flags For Visual Demo

Proposed first flags:

```text
--fixture <path>
--backend null|vulkan
--width <pixels>
--height <pixels>
--frames <count>
--strict-vulkan
--print-renderer-receipt
--disable-validation
--enable-sync-validation
```

Rules:

- `--backend null` must not require SDL or Vulkan;
- `--backend vulkan` may require SDL and Vulkan;
- `--frames <count>` lets smoke tests run bounded loops;
- `--strict-vulkan` turns skips into failures;
- validation flags affect renderer diagnostics only, never runtime truth.

Needs later file-plan detail:

- whether visual demo shares `AppConfig`/`CliParser`;
- exact default resolution;
- exact exit codes.

## CMake And Dependency Acquisition

Proposed options:

```text
IGGY3D_ENABLE_VISUAL_DEMO
IGGY3D_ENABLE_VULKAN
IGGY3D_ENABLE_VULKAN_SMOKE
IGGY3D_REQUIRE_VULKAN_SMOKE
IGGY3D_USE_SYSTEM_SDL3
```

Rules:

- headless runtime apps must build with all visual/Vulkan options off;
- SDL3 dependency is required only for visual demo/platform smoke;
- Vulkan SDK is required only when Vulkan backend/smoke is enabled;
- dependency acquisition must be documented in build output;
- no configure-time network dependency unless explicitly selected by a CMake option.

Preferred first acquisition policy:

- find system SDL3 first when requested;
- allow vendored or externally provided SDL3 later if packaging requires it;
- do not vendor SDL3 in this docs pass.

Current unresolved dependency decision:

- SDL3 source/system/vendored acquisition is not settled here;
- Packet 1 must compile without SDL3, Vulkan, display access, shader compiler tools, or package-runtime lookup;
- the platform/dependency packet must either keep `IGGY3D_USE_SYSTEM_SDL3` as the first accepted lane or document a vendored/external SDL3 lane before strict visual boot depends on it.

Current dependency probe lane:

- default visual boot remains no-window `NullRenderer` and must not require SDL3, Vulkan, MoltenVK, display access, shader artifacts, or shader compiler tools;
- `--window` uses SDL3 only when CMake finds a system SDL3 target and otherwise exits with receipt `result=skip` and `reason_code=sdl3_unavailable`;
- macOS Vulkan readiness is probed through a diagnostics lane, not renderer work. The probe reports SDL3, Vulkan loader, SDK root, ICD path, MoltenVK, `glslc`, validation layers, sync validation, and portability-enumeration facts;
- the macOS portability layer is named MoltenVK. Do not use alternate names in receipts or docs.

Reference sources for the macOS probe:

- Khronos MoltenVK: https://github.com/KhronosGroup/MoltenVK/
- LunarG macOS Vulkan SDK getting started: https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html
- Vulkan portability enumeration: https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_enumeration.html

Needs later file-plan detail:

- exact `find_package` call;
- imported target name;
- fallback strategy;
- Windows DLL/runtime copy rules;
- Linux package names;
- macOS framework/dylib handling.

## Diagnostics Receipt Fields

Platform shell and renderer diagnostics should add:

```text
window_shell=sdl3
window_width=
window_height=
framebuffer_width=
framebuffer_height=
drawable=true|false
surface_provider=sdl3
platform_required_instance_extensions=
surface_created=true|false
resize_count=
minimize_count=
focus_state=focused|unfocused
```

Vulkan backend diagnostics should add the existing renderer fields from `diagnostics_and_tests.md`.

Rules:

- receipt is backend/platform diagnostic data only;
- receipt does not affect replay hash;
- strict smoke tests can parse it.

## Platform Smoke Tests

Recommended smoke binaries/tests:

```text
tests/smoke/vulkan_platform_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
```

Recommended labels:

```text
render
vulkan
gpu
requires_display
smoke
macos
linux
windows
moltenvk
native_vulkan
```

Smoke expectations:

- create SDL window;
- query required instance extensions;
- create Vulkan instance;
- create surface;
- select device/queues;
- create/destroy swapchain if test scope includes swapchain;
- shut down without validation errors;
- print diagnostics receipt.

Skip behavior:

- missing SDL3 when visual demo disabled: test not built;
- missing display/window server: skip unless `IGGY3D_REQUIRE_VULKAN_SMOKE=ON`;
- missing Vulkan loader/device: skip unless strict Vulkan is required;
- missing validation layers: skip or warn only if validation is optional; fail in strict validation lane.

Strict command:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Normal headless command must still work:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Platform Notes

### macOS/MoltenVK

- first local validation lane;
- SDL window must support Vulkan surface creation through MoltenVK;
- portability extension requirements are recorded by Vulkan diagnostics;
- MoltenVK behavior is not cross-platform renderer authority.

Needs later detail:

- SDK environment variables;
- dylib/framework placement;
- app bundle vs CLI visual demo choice;
- validation layer setup.

### Linux Native Vulkan

- required shipping lane;
- must validate native Vulkan, not only MoltenVK behavior;
- loader/ICD/display availability must be reported clearly.

Needs later detail:

- X11 vs Wayland SDL behavior;
- distro package assumptions;
- optional software Vulkan/lavapipe policy;
- CI/display strategy.

### Windows Native Vulkan

- required shipping lane;
- must validate native Vulkan with the same renderer contract;
- Visual Studio/multi-config output paths must not break shader/platform lookup.

Needs later detail:

- Vulkan SDK/runtime expectations;
- SDL3 DLL copy/install rules;
- validation layer setup;
- RenderDoc capture notes.

## Completion Criteria For This Shell Design

The platform shell design is ready for file plans when:

- SDL3 remains the proposed first shell or a fallback decision replaces it;
- exact shell files are listed;
- surface creation callback shape is accepted or replaced;
- CMake options are named;
- event loop ownership is clear;
- resize/minimize/focus behavior is defined;
- input-to-runtime command route is defined;
- smoke labels and skip behavior are defined;
- headless acceptance remains independent of SDL/Vulkan.

## Open Detail Items

These belong in future file plans or platform-specific docs:

- exact SDL3 CMake target and package discovery;
- exact SDL3 Vulkan function signatures for the chosen SDL version;
- concrete `SdlWindow` C++ API;
- concrete `SdlVulkanSurface` C++ API;
- exact visual demo CLI parser integration;
- exact platform exit codes;
- exact Windows DLL install behavior;
- exact macOS bundle/runtime layout.
