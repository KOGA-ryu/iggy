# tests/unit/render_replay_invariance_tests.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `tests/unit/render_replay_invariance_tests.cpp`

Purpose: Prove renderer consumption leaves runtime summary and state hash unchanged.

## Build Position

Packet order: 2 - Null renderer and invariance
Owner module: `replay invariance tests`
File kind: `unit test`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Prerequisite clarification: this test plan proves renderer submission does not mutate runtime hash, runtime state, command results, or replay truth. It does not require the replay tool or `CommandReplay` to be complete before Packet 1 backend-neutral renderer boundary or Packet 2 null renderer work begins.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/diagnostics_and_tests.md`

## Ownership

This file owns:
- Prove renderer consumption leaves runtime summary and state hash unchanged.
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
- `renderer replay invariance assertions`

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
file_plan=tests/unit/render_replay_invariance_tests.cpp
packet_order=2
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
fallback_area=replay_invariance_tests
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
- `tests/unit/render_replay_invariance_tests.cpp`

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

- File `tests/unit/render_replay_invariance_tests.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

## Packet 2 Detailed Contract

Test role: prove renderer consumption is outside deterministic runtime truth.

Required test groups:
```text
runtime_hash_same_without_renderer_and_with_null_renderer
runtime_summary_same_without_renderer_and_with_null_renderer
command_result_same_without_renderer_and_with_null_renderer
renderer_frame_index_does_not_change_runtime_hash
renderer_presentation_delta_does_not_change_runtime_hash
renderer_rejected_frame_does_not_mutate_runtime
renderer_shutdown_does_not_mutate_runtime
```

Required flow:
```text
load first_room fixture through public content/runtime APIs
run deterministic command script without renderer
capture runtime summary and state hash
reset or create equivalent runtime session
run same deterministic command script
after each projected tick, build FrameInput and submit to NullRenderer
capture runtime summary and state hash
compare summaries and hashes
```

Allowed dependencies:
- public content fixture loader;
- public runtime/session runner APIs;
- public projection APIs;
- public renderer API or `NullRenderer`;
- public state hash/replay helpers.

Forbidden dependencies:
- Vulkan headers;
- SDL/window headers;
- renderer backend internals beyond the null renderer under test;
- runtime mutation shortcuts not used by normal command flow;
- save/load private internals;
- old repo fixtures or paths.

Required receipt fields:
```text
backend=null
runtime_hash_before=
runtime_hash_after=
runtime_hash_without_renderer=
runtime_hash_with_null_renderer=
runtime_summary_without_renderer=
runtime_summary_with_null_renderer=
replay_invariant=true
renderer_frame_count=
reason_code=render_replay_invariant
```

Failure reason codes:
```text
render_replay_hash_changed
render_runtime_summary_changed
render_replay_command_result_changed
render_frame_submission_mutated_runtime
```

Test fixture rule:
- use the same first-room fixture family as headless acceptance;
- do not introduce a renderer-only fixture;
- renderer consumes projection output only.

Required scan:
```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' tests/unit/render_replay_invariance_tests.cpp
```

Expected scan result:
```text
no matches
```

CTest labels:
```text
iggy3d;unit;render;replay;invariance
```

Packet 2 completion proof:
```sh
ctest --test-dir build --output-on-failure -R 'render_replay_invariance|replay_state_hash'
```
