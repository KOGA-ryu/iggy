# tests/unit/render_boundary_tests.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/unit/render_boundary_tests.cpp`

Purpose: Prove public renderer headers expose no Vulkan or SDL dependency and preserve runtime firewall.

## Build Position

Packet order: 1 - Backend-neutral renderer boundary
Owner module: `renderer tests`
File kind: `unit test`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/boundaries.md`

## Ownership

This file owns:
- Prove public renderer headers expose no Vulkan or SDL dependency and preserve runtime firewall.
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
- `render boundary assertions`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Backend-neutral values are owned by renderer API callers for the call duration unless copied into diagnostics. Runtime/projection data remains authoritative outside the renderer. Diagnostics receipts are renderer-owned output. No GPU, window, or Vulkan object lifetime exists in this file.

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
file_plan=tests/unit/render_boundary_tests.cpp
packet_order=1
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

Fallback policy: optional smoke may skip with `result=skip`; strict smoke fails with `result=fail`; pass requires the documented receipt.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=renderer_tests
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
- `tests/unit/render_boundary_tests.cpp`

Smoke tests:
- none for this file; covered by unit tests

CTest labels:
```text
iggy3d;render;unit
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

## Completion Criteria

- File `tests/unit/render_boundary_tests.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

## Current Baseline Detailed Contract

Test role: prove the renderer boundary compiles and behaves without Vulkan, SDL, display access, shader artifacts, or package lookup, while preserving the current source baseline where the Null factory path is already implemented.

Required test groups:
```text
public_headers_compile_without_graphics
renderer_api_accepts_injected_test_backend
renderer_api_accepts_factory_null_backend
renderer_api_diagnoses_vulkan_without_surface_provider
renderer_api_does_not_expose_graphics_types
renderer_api_shutdown_is_idempotent
renderer_api_submit_after_shutdown_is_diagnosed
```

Required assertions:
- including `render/RendererApi.hpp`, `render/RenderBackend.hpp`, `render/FrameInput.hpp`, `render/RenderDiagnostics.hpp`, and `render/RendererConfig.hpp` does not require Vulkan or SDL build definitions.
- `RendererApi(std::unique_ptr<RenderBackend>)` can run through an injected test backend without including `src/render/null/**`.
- current baseline `createRenderer(RendererBackendKind::Null)` constructs a concrete `NullRenderer`.
- `createRenderer(RendererBackendKind::Vulkan)` remains diagnosed without an app-owned surface/provider unless current feature-gated source supplies one.
- `submitFrame` with invalid frame returns `reason_code=frame_input_invalid`.
- `shutdown` may be called twice.
- no test reaches into runtime mutation internals.

Required scan command:
```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/render/RendererApi.hpp src/render/RenderBackend.hpp src/render/FrameInput.hpp src/render/RenderDiagnostics.hpp src/render/RendererConfig.hpp tests/unit/render_boundary_tests.cpp
```

Expected scan result:
```text
no matches
```

CTest labels:
```text
iggy3d;unit;render;boundary
```

Packet 1 completion proof:
```sh
ctest --test-dir build --output-on-failure -R 'render_boundary'
```
