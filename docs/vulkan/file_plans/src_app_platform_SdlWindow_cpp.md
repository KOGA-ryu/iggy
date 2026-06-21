# src/app/platform/SdlWindow.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/app/platform/SdlWindow.cpp`

Purpose: Implement SDL video init, window lifetime, resize/minimize observation, and app-level event pumping.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `SDL platform shell`
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
- `docs/vulkan/platform_shell.md`
- `docs/vulkan/vulkan_resize_minimize_test_plan.md`

## Ownership

This file owns:
- Implement SDL video init, window lifetime, resize/minimize observation, and app-level event pumping.
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

Allowed includes: standard library, app config helpers, diagnostics, SDL3 headers for SDL window files, and platform OS path helpers.

Forbidden includes: Vulkan headers except through `SdlVulkanSurface.*`, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in this file unless the exact path is `SdlVulkanSurface.*`.

SDL headers are allowed only in SDL platform implementation files and not in runtime/content/projection/save.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `SdlWindow constructor/destructor`
- `pollEvents`
- `framebufferExtent`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

App/platform code owns window and path lookup objects. The renderer owns GPU resources. Runtime owns gameplay state. Platform events may be translated into app-level requests, but they do not mutate runtime state directly. Diagnostics receipts are emitted by app or render diagnostics helpers.

## Semantics

Normal path: resolve platform paths, window state, events, or diagnostics roots without touching runtime truth. Skip/fail: visual-only work may skip in optional graphics lanes and must fail in strict package lanes when required paths are missing.

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
file_plan=src/app/platform/SdlWindow.cpp
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

Fallback policy: platform lookup may use documented path candidates. Runtime/content/projection behavior never changes because a graphics path is missing.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=SDL_platform_shell
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
- `tests/smoke/vulkan_resize_minimize_smoke.cpp`

CTest labels:
```text
iggy3d;render
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
1. Initialize SDL video support through the app/platform layer.
2. Create one window using title, size, resizable, high-DPI, and Vulkan-capable flags from `SdlWindowCreateInfo`.
3. Query drawable extent after creation.
4. Poll events each frame and update `SdlWindowEventState`.
5. Destroy the native window before SDL video shutdown.

State rules:
- The implementation owns SDL init/shutdown balance for the visual app shell unless a later app context packet creates a shared SDL context.
- It stores only app-level window state and event-state bits.
- It must not create a Vulkan instance, surface, device, swapchain, command pool, or renderer object.
- It must not dispatch runtime commands or mutate runtime state.

Resize and minimize semantics:
- A resize event sets `resized=true` and records the latest window and drawable sizes.
- A minimized or zero-drawable window sets `minimized=true` or `isDrawable=false`.
- A restored window sets `restored=true` once and updates drawable extent.
- Event-state edge bits are reset at the start of each `pollEvents()` call, while persistent fields such as focus and latest extent remain current.

Error and receipt policy:
```text
window_shell=sdl3
sdl_video=<ok|failed>
window_created=<true|false>
window_width=<integer>
window_height=<integer>
drawable_width=<integer>
drawable_height=<integer>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Platform behavior:
- macOS: high-DPI drawable size may differ from window size and must be reported separately.
- Linux: display absence in non-strict smoke lanes may report `result=skip` with `reason_code=sdl_display_unavailable`.
- Windows: event polling must preserve minimize and restore events without using the current working directory.

Compute cost:
- Initialization is one SDL video init plus one window creation.
- Per-frame cost is bounded by available SDL events and one extent query.
- No GPU work belongs here.

Verification:
- `vulkan_platform_smoke` constructs and destroys a window, records drawable fields, and handles display-unavailable skip rules.
- `vulkan_resize_minimize_smoke` later stresses event reporting without demanding swapchain recreation from this file.
- Firewall scans prove no runtime/content/projection/save Vulkan leakage.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not rely on current working directory for installed package lookup.
- Do not let SDL event polling mutate runtime state directly.

## Completion Criteria

- File `src/app/platform/SdlWindow.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
