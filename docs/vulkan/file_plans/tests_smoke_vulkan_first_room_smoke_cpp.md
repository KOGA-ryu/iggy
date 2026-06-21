# tests/smoke/vulkan_first_room_smoke.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/smoke/vulkan_first_room_smoke.cpp`

Purpose: Prove runtime/projection FrameInput renders a visible first room with runtime hash unchanged.

## Build Position

Packet order: 7 - First visible room proof and packaging smoke
Owner module: `first room smoke`
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
- `docs/vulkan/first_room_render_contract.md`
- `docs/vulkan/vulkan_screenshot_and_frame_hash.md`

## Ownership

This file owns:
- Prove runtime/projection FrameInput renders a visible first room with runtime hash unchanged.
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
- `vulkan_first_room_smoke executable`

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
file_plan=tests/smoke/vulkan_first_room_smoke.cpp
packet_order=7
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
fallback_area=first_room_smoke
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
- `tests/smoke/vulkan_first_room_smoke.cpp`

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

## Packet 7 Detailed Contract

Smoke purpose:
- Prove the first visible Vulkan room using the renderer path created by Packets 1 through 6.
- The smoke proves runtime/projection data can become `FrameInput`, feed Vulkan, draw room proxy geometry, and present without mutating runtime truth.
- It is not final art, asset streaming, material lighting, UI, multiplayer visualization, or editor proof.

Required startup:
1. Create or load the approved first-room runtime fixture through public runtime APIs.
2. Capture runtime deterministic state hash before rendering.
3. Build `SceneProjection`, `DebugProjection`, and `CameraState` through projection-owned code.
4. Build `FrameInput` from projection outputs.
5. Create Vulkan bootstrap, swapchain, sync, command recording, shader modules, pipeline, and first-room resources.
6. Draw at least one visible room frame.
7. Capture runtime deterministic state hash after rendering.
8. Emit receipt and artifacts.

Visible proof criteria:
```text
first_room_visible=true
room_proxy_visible=true
player_marker_visible=true
non_background_pixel_coverage_gt_minimum=true
depth_enabled=true
draw_count_greater_than_zero=true
runtime_hash_before_equals_after=true
```

Required receipt fields:
```text
smoke=vulkan_first_room
pipeline_family=first_room
pipeline_variant=first_room.vertex_color.opaque.depth.backface
shader_language=glsl
frame_count_requested=<integer>
frames_presented=<integer>
draw_count=<integer>
vertex_buffer_count=<integer>
index_buffer_count=<integer>
depth_enabled=true|false
room_proxy_visible=true|false|unavailable
player_marker_visible=true|false|unavailable
marker_count=<integer>
first_room_visible=true|false
runtime_hash_before=<hash-or-none>
runtime_hash_after=<hash-or-none>
validation_error_count=<integer>
sync_validation_error_count=<integer>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Skip/fail policy:
- Non-strict lane may skip before Vulkan work when display, loader, surface, device, or shader artifacts are unavailable.
- Strict lane fails on missing required platform support, missing shaders, pipeline creation failure, zero presented frames, visible-room failure, validation error, sync validation error, runtime hash mutation, or malformed receipt.
- After screenshot capture exists, strict first-room proof must include screenshot artifact fields from the screenshot smoke policy.

Verification:
- Runtime hash before and after rendering must match.
- Renderer diagnostics must not become save/replay truth.
- The smoke must write a receipt under the configured diagnostics directory.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `tests/smoke/vulkan_first_room_smoke.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
