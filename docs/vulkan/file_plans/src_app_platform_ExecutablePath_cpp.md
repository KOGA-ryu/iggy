# src/app/platform/ExecutablePath.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/app/platform/ExecutablePath.cpp`

Purpose: Implement macOS, Linux, and Windows executable path discovery without relying on current working directory.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `platform path lookup`
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
- `docs/vulkan/vulkan_package_runtime_lookup.md`

## Ownership

This file owns:
- Implement macOS, Linux, and Windows executable path discovery without relying on current working directory.
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
- `resolveExecutablePath`

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
file_plan=src/app/platform/ExecutablePath.cpp
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
fallback_area=platform_path_lookup
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
- `tests/unit/package_runtime_lookup_tests.cpp`

Smoke tests:
- none for this file; covered by unit tests

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

Platform implementations:
- macOS: use `_NSGetExecutablePath`, then canonicalize the returned path.
- Linux: use `/proc/self/exe` through `readlink`, then canonicalize the returned path.
- Windows: use `GetModuleFileNameW`, then convert to `std::filesystem::path`.

Failure behavior:
- Buffer growth is allowed during path query.
- A path that cannot be canonicalized returns `executable_path_canonicalize_failed`.
- An unavailable platform path API returns `executable_path_unavailable`.
- No failure path may substitute current working directory.

Dependency rules:
- Platform OS headers stay in this `.cpp` file.
- This file does not include SDL or Vulkan.
- This file does not read environment variables; package lookup owns environment policy.

Receipt contribution:
```text
executable_path_resolved=<true|false>
executable_path=<absolute-path-or-empty>
executable_dir=<absolute-path-or-empty>
reason_code=<stable-reason>
```

Compute cost:
- Startup-only system call and canonicalization.
- Zero per-frame work.
- No directory traversal.

Verification:
- Unit tests prove stable failure propagation into `PackageRuntimeLookup`.
- Platform CI smoke proves the resolver returns an absolute executable path on macOS, Linux, and Windows lanes that build the visual app.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not rely on current working directory for installed package lookup.
- Do not let SDL event polling mutate runtime state directly.

## Completion Criteria

- File `src/app/platform/ExecutablePath.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
