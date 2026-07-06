# E47: Creative Command Diagnostic Subreceipts

## Objective

Continue the E30 cleanup by replacing the remaining flat Creative command
diagnostic blocks with typed subreceipt/submirror structures, while preserving
the externally emitted receipt keys.

## Problem

E30 centralized baked-room refresh diagnostic reset/copy logic, but most of the
Creative UI command diagnostics are still represented as flat fields on
`ProductAppWindowState`:

- mutation fields
- create fields
- delete fields
- undo fields
- room-shell fields
- manual rebuild/auto-refresh fields

`recordProductCreativeUiCommandFrame(...)` still copies a long field list from
`ProductCreativeUiCommandFrameReceipt` to the window, and `buildRenderReceipt`
still appends a long flat key list. The receipt evidence is useful, but the
shape makes every new command add another hand-copied block.

## Required Reads

- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`

## Scope

- Introduce typed substructures for command diagnostic groups where they reduce
  duplication, e.g. create/delete/undo/shell diagnostics.
- Keep `ProductCreativeUiCommandFrameReceipt` behavior and public receipt keys
  stable unless the change is purely internal naming.
- Add small append/copy/reset helpers per group so new commands do not require
  editing three unrelated slabs.
- Preserve sticky last-command evidence used for live UI debugging.

## Acceptance

- The window state no longer carries every command-specific diagnostic as one
  unrelated flat field block.
- `recordProductCreativeUiCommandFrame(...)` is materially shorter and delegates
  group copy/defaulting.
- RenderReceipt still emits the same keys and values for default, create, delete,
  undo, room-shell, and rebuild cases.
- Receipt tests prove the emitted external contract, not only helper internals.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_receipt_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not remove receipt fields that tests or live diagnostics rely on.
- Do not change command routing or command behavior.
- Do not combine this with the E35 command catalog unless both diffs remain
  small and reviewable.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
- Added typed window-state diagnostic groups:
  - `ProductCreativeUiCommandDiagnostics`
  - `ProductCreativeUiCommandMutationDiagnostics`
  - `ProductCreativeUiCommandCreateDiagnostics`
  - `ProductCreativeUiCommandDeleteDiagnostics`
  - `ProductCreativeUiCommandUndoDiagnostics`
  - `ProductCreativeUiCommandRoomShellDiagnostics`
  - `ProductCreativeBakedRoomRefreshDiagnostics`
- `recordProductCreativeUiCommandFrame(...)` now delegates group copy/defaulting
  through helper functions and keeps sticky last-command evidence unchanged.
- `buildRenderReceipt(...)` now emits command, subdiagnostic, manual rebuild, and
  auto-refresh fields through append helpers while preserving external receipt
  key names.
- Updated focused tests that inspect internal `ProductAppWindowState` mirrors to
  read the grouped diagnostics. External receipt-key tests remain the contract.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_receipt_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_wireframe_frame_tests product_window_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_wireframe_frame_tests|product_window_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

Notes:

- Existing missing-field initializer warnings remain in
  `product_creative_ui_input_frame_tests.cpp` and
  `product_window_input_frame_tests.cpp`; this slice did not introduce or repair
  them.
