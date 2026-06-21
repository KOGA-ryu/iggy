# tests/smoke/vulkan_resize_minimize_smoke.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/smoke/vulkan_resize_minimize_smoke.cpp`

Purpose: Prove resize storms, minimize/restore, zero extent, DPI/display changes, and bounded recreate diagnostics.

## Build Position

Packet order: 5 - Swapchain, render loop, command recording, sync
Owner module: `resize smoke`
File kind: `smoke test`
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
- `docs/vulkan/vulkan_resize_minimize_test_plan.md`

## Ownership

This file owns:
- Prove resize storms, minimize/restore, zero extent, DPI/display changes, and bounded recreate diagnostics.
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
- `vulkan_resize_minimize_smoke executable`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: expose or test the backend-neutral renderer contract. Skip/fail: invalid frame/config inputs fail locally; Vulkan availability is not evaluated by this file.

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
file_plan=tests/smoke/vulkan_resize_minimize_smoke.cpp
packet_order=5
allowed_to_implement_code_now=false
test_name=
platform_lane=
strict_vulkan=true|false
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
fallback_area=resize_smoke
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: test harness setup plus any feature-gated renderer objects under test.
Per-frame cost: bounded by the smoke frame count and receipt collection.
Resize cost: exercised only in resize smoke files.
GPU memory cost: limited to named resources under the tested module.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
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

Smoke purpose:
- Prove resize, zero extent, minimize/restore, out-of-date handling, and recreate counters do not deadlock and do not mutate runtime truth.
- The smoke may use real platform resize events and a forced zero-extent policy path for hosts where minimize automation is not stable.

Inputs:
```text
IGGY3D_REQUIRE_VULKAN_SMOKE=0|1
IGGY3D_RESIZE_SCRIPT=<simple_resize|resize_storm|minimize_restore|forced_zero_extent>
IGGY3D_RESIZE_ITERATIONS=<integer>
IGGY3D_RESIZE_MIN_WIDTH=<integer>
IGGY3D_RESIZE_MIN_HEIGHT=<integer>
IGGY3D_RESIZE_MAX_WIDTH=<integer>
IGGY3D_RESIZE_MAX_HEIGHT=<integer>
IGGY3D_DIAGNOSTICS_DIR=<path>
```

Required test cases:
- `simple_resize`: one drawable resize, one safe recreate, then at least one presented frame.
- `resize_storm`: coalesced recreate behavior after repeated drawable changes.
- `minimize_restore`: skip while non-drawable, recreate after restore, presentation resumes.
- `forced_zero_extent`: zero extent skips before acquire without creating a zero-sized swapchain.

Hard assertions:
- `acquire_attempts_while_zero_extent=0`.
- `submit_attempts_while_zero_extent=0`.
- `fence_resets_while_zero_extent=0`.
- `swapchain_create_attempted=false` during forced zero extent.
- Runtime hash before/after renderer resize path is unchanged when runtime fixture is involved.
- At least one final frame presents after restore or resize when the platform supports it.

Receipt fields:
```text
smoke=vulkan_resize_minimize
test_case=<simple_resize|resize_storm|minimize_restore|forced_zero_extent>
resize_event_count=<integer>
minimize_event_count=<integer>
restore_event_count=<integer>
zero_extent_frame_count=<integer>
swapchain_dirty_count=<integer>
swapchain_recreate_count=<integer>
acquire_attempts_while_zero_extent=<integer>
submit_attempts_while_zero_extent=<integer>
fence_resets_while_zero_extent=<integer>
frames_presented=<integer>
frames_presented_after_restore=<integer>
runtime_hash_before=<hash-or-none>
runtime_hash_after=<hash-or-none>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Skip/fail policy:
- Non-strict lane may skip display automation that the host cannot perform.
- Strict lane fails on deadlock, validation error, zero-sized swapchain attempt, acquire while zero extent, submit while zero extent, fence reset without submit, failed restore, or runtime mutation.

Verification:
- Smoke writes receipt for each case.
- Strict resize smoke runs with validation enabled when available.
- Software Vulkan does not replace native macOS/Linux/Windows resize proof.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `tests/smoke/vulkan_resize_minimize_smoke.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
