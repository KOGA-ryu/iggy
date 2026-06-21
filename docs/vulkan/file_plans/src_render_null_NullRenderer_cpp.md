# src/render/null/NullRenderer.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/null/NullRenderer.cpp`

Purpose: Implement no-op frame consumption, diagnostics, and clean lifecycle without GPU, window, or shader dependencies.

## Build Position

Packet order: 2 - Null renderer and invariance
Owner module: `null renderer`
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

## Ownership

This file owns:
- Implement no-op frame consumption, diagnostics, and clean lifecycle without GPU, window, or shader dependencies.
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

Allowed includes: standard library, core math/id value types, projection value types where the frame contract permits them, and `src/render/**` backend-neutral headers.

Forbidden includes: Vulkan SDK headers, `Vk*` types, `VK_*` constants, SDL/window headers, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in backend-neutral renderer, null renderer, and non-Vulkan unit test files.

SDL/window headers are forbidden outside app/platform glue and Vulkan platform smoke tests.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `NullRenderer::renderFrame`
- `NullRenderer::diagnostics`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Backend-neutral values are owned by renderer API callers for the call duration unless copied into diagnostics. Runtime/projection data remains authoritative outside the renderer. Diagnostics receipts are renderer-owned output. No GPU, window, or Vulkan object lifetime exists in this file.

## Semantics

Normal path: consume valid FrameInput, increment diagnostics, and perform no rendering side effects. Skip/fail: invalid input returns a backend-neutral failure; no graphics skip is needed.

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
file_plan=src/render/null/NullRenderer.cpp
packet_order=2
allowed_to_implement_code_now=false
backend=null|vulkan|unavailable
frame_input_valid=true|false|unavailable
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: no Vulkan fallback is needed. Null renderer fallback is explicit only when selected by app config or test harness.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=null_renderer
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: constant CPU setup and no GPU work.
Per-frame cost: linear in submitted frame items for validation/diagnostics; null renderer performs no draw work.
Resize cost: backend-neutral state update only.
GPU memory cost: none.

## Tests And Verification

Unit tests:
- `tests/unit/render_null_renderer_tests.cpp`
- `tests/unit/render_replay_invariance_tests.cpp`

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

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not add Vulkan or SDL includes to backend-neutral files.
- Do not hide runtime mutation inside frame validation or diagnostics.

## Completion Criteria

- File `src/render/null/NullRenderer.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

## Packet 2 Detailed Contract

Source role: implement the no-op backend with deterministic diagnostics and lifecycle behavior.

Factory ownership: Packet 2 owns the source-side wiring that maps `RendererBackendKind::Null` to `NullRenderer` through the public `createRenderer` entrypoint. Packet 1 `createRenderer` reports the null backend as missing until this file and its header are implemented.

Initialization:
```text
constructor sets lifecycle_state=ready
all counters start at 0
last_source_tick=0
last_frame_index=0
last_reason_code=null_renderer_ok
```

`submitFrame` normal path:
```text
increment submitted_frame_count
validate FrameInput
if valid and drawable:
  increment accepted_frame_count
  copy source_tick and frame_index
  copy scene/debug item counts
  set draw_count=0
  set reason_code=null_renderer_ok
  return outcome=Ok
if not drawable:
  increment rejected_frame_count
  set reason_code=null_renderer_not_drawable
  return outcome=SkipFrame
if invalid:
  increment rejected_frame_count
  set reason_code=null_renderer_frame_invalid
  return outcome=InvalidFrameInput
```

Shutdown behavior:
- `shutdown` sets lifecycle to `Shutdown`.
- `shutdown` is idempotent.
- `submitFrame` after shutdown returns `outcome=RendererNotReady` and `reason_code=null_renderer_shutdown`.
- `waitIdle` returns `Ok` because no GPU work exists.

Resize behavior:
- valid nonzero viewport updates last viewport fields and returns `outcome=Ok`.
- zero width or height returns `outcome=SkipFrame` and `reason_code=null_renderer_resize_not_drawable`.
- resize never affects runtime camera truth.

Projection handling:
- may read public scene/debug item count fields named by the projection result types;
- must not store projection pointers;
- must not inspect content package provenance;
- must not mutate scene/debug projection values.

Diagnostics:
```text
receipt_version=1
repo=iggy3d
backend=null
result=pass|fail|skip
reason_code=
renderer_lifecycle=
submitted_frame_count=
accepted_frame_count=
rejected_frame_count=
draw_count=0
frame_input_valid=
scene_item_count=
debug_item_count=
replay_invariant=unavailable
```

Compute cost:
- initialization: constant CPU only;
- per frame: one `FrameInput` validation plus projection item counts;
- resize: constant CPU only;
- GPU memory: none;
- file IO: none.

Packet 2 acceptance:
```sh
ctest --test-dir build --output-on-failure -R 'render_null|render_replay_invariance'
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/render/null/NullRenderer.cpp
```

Expected scan result:
```text
no matches
```
