# src/render/RenderDiagnostics.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/RenderDiagnostics.cpp`

Purpose: Format deterministic key-value renderer receipts and aggregate backend-neutral diagnostics.

## Build Position

Packet order: 1 - Backend-neutral renderer boundary
Owner module: `renderer diagnostics`
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
- Format deterministic key-value renderer receipts and aggregate backend-neutral diagnostics.
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
- `formatRenderReceipt`
- `appendReceiptField`
- `normalizeReceiptValue`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Backend-neutral values are owned by renderer API callers for the call duration unless copied into diagnostics. Runtime/projection data remains authoritative outside the renderer. Diagnostics receipts are renderer-owned output. No GPU, window, or Vulkan object lifetime exists in this file.

## Semantics

Normal path: format stable key-value receipts. Skip/fail: diagnostics record skip or fail decisions but do not make them; app/test harness maps outcomes to process exit.

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
file_plan=src/render/RenderDiagnostics.cpp
packet_order=1
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

Fallback policy: optional smoke may skip with `result=skip`; strict smoke fails with `result=fail`; pass requires the documented receipt.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=renderer_diagnostics
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
- `tests/unit/render_diagnostics_tests.cpp`

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

- File `src/render/RenderDiagnostics.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

## Packet 1 Detailed Contract

Source role: implement deterministic key-value receipt formatting and receipt-safe value normalization.

Required behavior:
```text
append field in call order
normalize newline, carriage return, and tab to spaces
normalize empty optional values to unavailable at call sites
format as key=value newline
preserve field order
do not sort fields
do not allocate per frame except receipt vector/string growth
```

Required output example:
```text
receipt_version=1
repo=iggy3d
backend=null
test_name=render_boundary_tests
result=pass
reason_code=renderer_ok
```

Invalid field handling:
- invalid key characters return or record `reason_code=receipt_key_invalid` in tests;
- values containing newlines are normalized, not emitted literally;
- values containing `=` are allowed because parsers split only on the first `=`;
- this file does not decide pass, fail, or skip; it only records the selected result.

Required reason codes owned by diagnostics:
```text
renderer_ok
receipt_key_invalid
receipt_value_normalized
receipt_missing_required_field
receipt_duplicate_unexpected
```

Performance:
- receipt assembly is CPU-only.
- no file IO in Packet 1 diagnostics formatting.
- artifact paths are strings only until later smoke tests write artifacts.

Packet 1 acceptance:
```sh
ctest --test-dir build --output-on-failure -R 'render_diagnostics|render_reason_code'
```

Expected proof:
```text
receipt ordering stable
newline normalization stable
required fields detected by tests
no graphics dependency
```
