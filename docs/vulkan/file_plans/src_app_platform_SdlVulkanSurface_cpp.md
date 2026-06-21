# src/app/platform/SdlVulkanSurface.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/app/platform/SdlVulkanSurface.cpp`

Purpose: Implement SDL Vulkan extension query and VkSurfaceKHR creation with platform diagnostics.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `SDL Vulkan surface bridge`
File kind: `source`
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
- `docs/vulkan/vulkan_surface_wsi_platforms.md`

## Ownership

This file owns:
- Implement SDL Vulkan extension query and VkSurfaceKHR creation with platform diagnostics.
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

Allowed includes: standard library, SDL3 window/Vulkan bridge headers, Vulkan SDK headers needed for surface creation, and diagnostics headers.

Forbidden includes: runtime mutation internals, content validators, save/load internals, projection production internals, and old repo headers.

Vulkan headers are allowed only for the surface bridge boundary and must not spread into generic app or runtime headers.

SDL headers are allowed because this file is app/platform glue.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `requiredInstanceExtensions`
- `createSurface`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: query platform-required instance extensions and create a Vulkan surface for the renderer. Skip/fail: optional lanes may skip when display/platform support is absent; strict lanes fail with platform reason codes.

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
file_plan=src/app/platform/SdlVulkanSurface.cpp
packet_order=3
allowed_to_implement_code_now=false
window_shell=sdl3|none|unavailable
surface_provider=sdl3|none|unavailable
drawable=true|false
platform_required_instance_extensions=
resource_root=
shader_root=
diagnostics_dir=
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: optional lanes may skip with `result=skip` and a reason code before required Vulkan work begins. Strict lanes fail with `result=fail`. Render-pass fallback and software Vulkan shipping substitution are not allowed by this file plan.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=SDL_Vulkan_surface_bridge
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: one SDL/window/path lookup setup and platform extension query.
Per-frame cost: event polling and drawable-size query only.
Resize cost: event bookkeeping; Vulkan swapchain recreate is owned elsewhere.
GPU memory cost: none in the platform shell.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/vulkan_platform_smoke.cpp`
- `tests/smoke/vulkan_device_smoke.cpp`

CTest labels:
```text
iggy3d;vulkan;smoke
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

## Packet 3 Detailed Contract

Implementation sequence:
1. Verify the `SdlWindow` has a native SDL window.
2. Query required Vulkan instance extensions through SDL before instance creation.
3. Return extension names as owned strings sorted by byte order for stable receipts.
4. After the Vulkan instance exists, call the SDL Vulkan surface creation API.
5. Return the surface handle to the caller without taking destruction ownership.

Dependency rules:
- SDL Vulkan APIs are used only in this bridge.
- Vulkan loader ownership remains with Vulkan bootstrap code.
- This file must not create `VkInstance`, select devices, create a logical device, create a swapchain, or submit commands.
- This file must not read runtime data or projection data.

Failure behavior:
- Non-strict platform smoke may skip before instance creation when SDL cannot expose Vulkan extensions on the host.
- Strict Vulkan lanes fail on missing SDL Vulkan extension support.
- Surface creation failure is a hard fail after the backend has committed to Vulkan startup.

Receipt fields:
```text
surface_provider=sdl3
required_instance_extensions=<comma-separated-names>
surface_create_attempted=<true|false>
surface_created=<true|false>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Platform behavior:
- macOS: extension list must carry the portability path needed for MoltenVK when SDL exposes it.
- Linux: extension list must reflect the active SDL video driver, such as X11 or Wayland.
- Windows: extension list must reflect Win32 surface support.
- Software Vulkan lanes still require real WSI support unless the smoke lane is explicitly device-only.

Compute cost:
- Startup cost is one extension query and at most one surface creation.
- No per-frame cost.
- Recreate cost is surface creation only when the owning backend requests a surface rebuild.

Verification:
- `vulkan_platform_smoke` checks extension list stability and skip/fail behavior.
- `vulkan_device_smoke` later checks that the surface can be paired with a selected physical device.
- Receipt output must be deterministic across repeated runs on the same machine.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `src/app/platform/SdlVulkanSurface.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
