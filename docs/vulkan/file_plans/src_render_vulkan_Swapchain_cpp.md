# src/render/vulkan/Swapchain.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/vulkan/Swapchain.cpp`

Purpose: Implement swapchain creation, capability queries, format/present/extent selection, image views, and recreate handling.

## Build Position

Packet order: 5 - Swapchain, render loop, command recording, sync
Owner module: `swapchain`
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
- `docs/vulkan/swapchain_contract.md`
- `docs/vulkan/vulkan_resize_minimize_test_plan.md`

## Ownership

This file owns:
- Implement swapchain creation, capability queries, format/present/extent selection, image views, and recreate handling.
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

Allowed includes: standard library, `src/render/**`, `src/render/vulkan/**`, Vulkan SDK headers, and test harness headers for Vulkan smoke tests.

Forbidden includes: runtime mutation internals, content package validators, save/load internals, old repo headers, and public exposure of raw `Vk*` handles outside Vulkan-owned declarations.

Vulkan headers are allowed because this file is under `src/render/vulkan/**` or is an explicitly named Vulkan smoke test.

SDL headers are allowed only when the smoke test exercises the platform shell; otherwise keep SDL behind app/platform files.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `Swapchain::create`
- `recreate`
- `destroy`
- `markDirty`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: create or use the Vulkan objects named by this file, emit receipt fields, return backend-neutral outcomes, and preserve runtime state. Skip/fail: optional lanes may skip before unsafe work; strict lanes fail on required Vulkan gaps, validation errors, or unsupported devices.

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
file_plan=src/render/vulkan/Swapchain.cpp
packet_order=5
allowed_to_implement_code_now=false
backend=vulkan
device_name=
api_version=
enabled_instance_extensions=
enabled_device_extensions=
validation=enabled|disabled|unavailable
result=pass|fail|skip
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
fallback_area=swapchain
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: Vulkan object creation, device queries, validation setup, or GPU resource setup according to module scope.
Per-frame cost: no runtime mutation; command/sync/resource modules pay only the documented frame work.
Resize cost: bounded wait/recreate/teardown for swapchain-dependent objects when the module owns them.
GPU memory cost: named allocations only; budget receipt required once resources are created.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/vulkan_swapchain_smoke.cpp`
- `tests/smoke/vulkan_resize_minimize_smoke.cpp`

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

## Packet 5 Detailed Contract

Create sequence:
1. Reject zero drawable extent with `state=NotDrawable`.
2. Query surface capabilities, formats, and present modes.
3. Select color format and color space from supported surface formats.
4. Select present mode from config request, falling back to FIFO when the requested mode is unavailable.
5. Clamp extent to surface capability bounds.
6. Select image count within surface min/max.
7. Select exclusive sharing when graphics and present queue families match.
8. Select concurrent sharing for the first implementation when graphics and present families differ.
9. Create `VkSwapchainKHR`.
10. Retrieve swapchain images.
11. Create one color image view per swapchain image.
12. Emit create receipt.

Recreate sequence:
1. Mark `state=Recreating`.
2. Skip recreate when drawable extent is zero and set `state=NotDrawable`.
3. Wait using Packet 5 resize sync policy.
4. Re-query surface capabilities, formats, and present modes.
5. Create a new swapchain with old swapchain handle when safe.
6. Destroy old image views.
7. Destroy old swapchain.
8. Retrieve new images and create new image views.
9. Increment generation and recreate counter.
10. Set `state=Ready`.
11. Emit recreate receipt.

Acquire handling:
- `VK_SUCCESS`: image valid, submit allowed, present allowed.
- `VK_SUBOPTIMAL_KHR`: image valid, submit allowed, present allowed, recreate requested soon.
- `VK_ERROR_OUT_OF_DATE_KHR`: no image, no submit, no present, recreate requested.
- `VK_NOT_READY` and `VK_TIMEOUT`: no submit in first Packet 5 path; diagnose according to strictness at smoke boundary.
- `VK_ERROR_DEVICE_LOST`: mark device lost and block swapchain recreate.
- Other errors: fail with `reason_code=swapchain_acquire_failed`.

Present result handling:
- `VK_SUCCESS`: presented.
- `VK_SUBOPTIMAL_KHR`: presented, recreate requested soon.
- `VK_ERROR_OUT_OF_DATE_KHR`: submitted frame did not produce durable present; mark dirty for next safe recreate.
- `VK_ERROR_DEVICE_LOST`: mark device lost and block swapchain recreate.
- Surface-lost results are diagnosed separately from resize.

Zero drawable rules:
- Do not create a zero-sized swapchain.
- Do not acquire while known drawable extent is zero.
- Do not present while known drawable extent is zero.
- Do not mutate runtime tick policy; app owns minimized runtime tick behavior.

Compute cost:
- Create/recreate performs fixed WSI queries, swapchain creation, image retrieval, and image view creation.
- Per-frame cost is one acquire plus lightweight state/result mapping.
- No recursive probing, shader work, resource upload, or runtime work belongs here.

Receipt additions:
```text
surface_cap_min_images=<integer>
surface_cap_max_images=<integer-or-zero>
surface_cap_current_extent=<width>x<height-or-special>
available_present_modes=<comma-separated-names>
available_surface_formats=<count>
swapchain_recreate_count=<integer>
acquire_result=<vk-result-name-or-none>
acquire_action=<continue|skip|fail|mark_dirty|surface_lost|device_lost>
present_result=<vk-result-name-or-none>
present_action=<accepted|mark_dirty|surface_lost|device_lost|fail>
```

Verification:
- `vulkan_swapchain_smoke` asserts creation metadata and clean destroy.
- `vulkan_resize_minimize_smoke` asserts zero extent skip, recreate count, and no acquire while non-drawable.
- Strict lanes fail on missing required WSI support; non-strict unsupported host lanes skip before swapchain creation.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `src/render/vulkan/Swapchain.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
