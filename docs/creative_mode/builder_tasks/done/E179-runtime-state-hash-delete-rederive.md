# E179 - Runtime State Hash Delete/Re-derive

## Status

Ready.

## Objective

Delete the remaining top-level `ProductAppWindowState::runtimeStateHash` mirror
and rederive/pass the runtime hash at the two real consumers:

- receipt field `runtime_state_hash`;
- SDL gameplay panel rendering.

This is the last field currently owned by the TSV `delete` bucket.

## Background

E177 confirmed `runtimeStateHash` is still live despite stale target-map wording
that said it had already been removed. E178 moved `creativeWorldEpoch`; do not
touch that field in this card.

`runtimeStateHash` is a mirror of session state. It should not live on
`ProductAppWindowState`.

## Scope

Expected source areas:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/ReceiptBuilder.hpp/.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp/.cpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- writers that currently assign `window.runtimeStateHash`
- focused tests that currently seed/assert the window mirror
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- this task card

Use compiler-guided removal. Do not broad replace unrelated local/result fields
such as `ProductGameplayTapeRunResult::runtimeStateHash` or
`ProductAsciiRoomActivationResult::runtimeStateHash`.

## Required Policy

- No active session available -> runtime hash reported as `0`.
- Active session available -> runtime hash reported as `activeSession->stateHash()`.
- Local operation result structs may keep their own `runtimeStateHash` fields
  when they are operation outputs.
- The window must no longer store or mirror this value.

## Suggested Implementation Shape

Prefer a small explicit data-flow rather than introducing a new store:

1. Remove `std::uint64_t runtimeStateHash` from `ProductAppWindowState`.
2. Add an explicit `std::uint64_t runtimeStateHash = 0` value to the receipt
   builder path, e.g. a final defaulted parameter to `buildProductAppReceipt(...)`
   and a matching parameter to `appendProductGameplayRuntimeMovementFields(...)`.
   Production `AppKernel` should pass `activeSession ? activeSession->stateHash() : 0`.
   Existing fixture callers may rely on the default unless they specifically
   assert a nonzero hash.
3. Add a per-frame runtime hash to the gameplay projection/presenter path, e.g.
   `ProductGameplayProjectionFrame::runtimeStateHash`, populated by
   `buildProductGameplayProjectionFrame(...)` from the request active session.
   `FramePresenter` should read that frame payload instead of the window.
4. Remove writer-only assignments in `Operations.cpp`, `Activation.cpp`,
   `AutomationGameplay.cpp`, `Controller.cpp`, `ProjectionRefresh.cpp`, and
   `TapeRunner.cpp` unless a local result still needs the hash.
5. Remove the `runtimeStateHash	delete` row from
   `docs/god_struct_member_ownership.tsv`.
6. Update stale wording in `docs/god_struct_decomposition_target_map.md` and
   `PRIORITY.md` only after the implementation gates pass.

Alternative implementation is acceptable if it preserves the policy and removes
the window mirror completely.

## Do Not Touch

- Do not move or edit `creativeWorldEpoch`.
- Do not move or reshape `automationControl`.
- Do not change session hashing semantics.
- Do not change receipt key order or regenerate the receipt golden.
- Do not remove local result fields that are not `ProductAppWindowState`
  storage.
- Do not stage, commit, push, or launch a window.

## Required Greps

These should produce no output after the change:

```sh
rg -n "window\\.runtimeStateHash\\b|request\\.window\\.runtimeStateHash\\b|context\\.window\\.runtimeStateHash\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\bruntimeStateHash\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^runtimeStateHash\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

These may remain because they are local operation/result payloads or render
function parameters, not window storage:

```sh
rg -n "\\bruntimeStateHash\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Classify every remaining hit in the completion brief.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_creative_fly_tests|product_creative_world_launch_tests|product_ascii_room_activation_tests|product_vulkan_room_frame_tests|product_gameplay_tape_runner_tests)$' --output-on-failure
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed.
- Exact deletion/rederive shape.
- Receipt runtime hash source.
- SDL gameplay panel runtime hash source.
- Removed writer sites.
- Remaining `runtimeStateHash` hit classification.
- Required grep results.
- Receipt golden and ownership coverage results.
- Focused/full test results.
- Confirmation that `creativeWorldEpoch` and `automationControl` were untouched.
- Concerns/deferred work.

## Completion Brief - E179

- Files changed:
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/god_struct_member_ownership.tsv`
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
  - `src/app/iggy3d/receipt/ReceiptFields.hpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `tests/unit/product_ascii_room_activation_tests.cpp`
  - `tests/unit/product_creative_fly_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`

- Exact deletion/rederive shape:
  - Deleted top-level `ProductAppWindowState::runtimeStateHash`.
  - Removed the `runtimeStateHash	delete` row from `docs/god_struct_member_ownership.tsv`.
  - Updated the target map and priority text to describe `runtimeStateHash` as deleted/rederived, not rehomed.
  - Kept local operation/result payload fields that are not window storage.

- Receipt runtime hash source:
  - `buildProductAppReceipt(...)` now takes a final defaulted `std::uint64_t runtimeStateHash = 0` parameter.
  - `AppKernel.cpp` passes `activeSession.has_value() ? activeSession->stateHash() : 0U` for production receipts.
  - `appendProductGameplayRuntimeMovementFields(...)` receives that explicit value and writes the existing `runtime_state_hash` receipt key.
  - Receipt key order and values stayed golden-compatible.

- SDL gameplay panel runtime hash source:
  - Added `ProductGameplayProjectionFrame::runtimeStateHash`.
  - `buildProductGameplayProjectionFrame(...)` populates the frame value from the active session hash or `0U`.
  - `FramePresenter.cpp` now passes `request.projectionFrame.runtimeStateHash` to `drawOpeningMenuView(...)`.

- Removed writer sites:
  - Removed writer-only `window.runtimeStateHash` assignments from `Operations.cpp`, `ascii_room/Activation.cpp`, `automation/AutomationGameplay.cpp`, `gameplay/Controller.cpp`, `gameplay/ProjectionRefresh.cpp`, and `gameplay/TapeRunner.cpp`.
  - Tests no longer seed/assert the deleted window mirror; they continue asserting the owning local/result hashes where applicable.

- Remaining `runtimeStateHash` hit classification:
  - `ReceiptBuilder.*`, `receipt/ReceiptFields.hpp`, and `receipt/GameplayRuntimeMovementFields.cpp`: explicit receipt parameter plumbing.
  - `gameplay/ProjectionRefresh.*`, `window/FramePresenter.cpp`, and `view/OpeningMenuView.*`: per-frame projection payload and SDL gameplay panel parameter.
  - `ascii_room/Activation.*` and `gameplay/TapeRunner.*`: local operation/result payloads retained by scope.
  - Historical docs and completed task cards still mention prior `runtimeStateHash` context; they are not active window storage.

- Required grep results:
  - `rg -n "window\\.runtimeStateHash\\b|request\\.window\\.runtimeStateHash\\b|context\\.window\\.runtimeStateHash\\b" ...` produced no output.
  - `rg -n "\\bruntimeStateHash\\b" src/app/iggy3d/ProductAppWindowState.hpp docs/god_struct_member_ownership.tsv` produced no output.
  - Broad `runtimeStateHash` scan was classified above.

- Receipt golden and ownership coverage results:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`: `assigned=14 CreativeAuthoringStore=1 DebugHudStore=1 FrontendWindowShell=1 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=1 app-global-remainder=5`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` produced no output.

- Focused/full test results:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - Focused CTest passed: `7/7` for `product_receipt_key_order_tests`, `product_god_struct_ownership_coverage_tests`, `product_creative_fly_tests`, `product_creative_world_launch_tests`, `product_ascii_room_activation_tests`, `product_vulkan_room_frame_tests`, and `product_gameplay_tape_runner_tests`.
  - Full CTest passed: `260/260` (`/tmp/iggy3d_e179_ctest.log`).
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files produced no output.

- Confirmation that `creativeWorldEpoch` and `automationControl` were untouched:
  - No `creativeWorldEpoch` storage or code path was moved or reshaped by E179.
  - No `automationControl` storage or code path was moved or reshaped by E179.

- Concerns/deferred work:
  - None for E179. `runtimeStateHash` is no longer `ProductAppWindowState` storage; current consumers rederive/pass the value at their consumption seams.
