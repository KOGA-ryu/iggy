# E30: Command Receipt Diagnostics Shape

## Objective

Stop ProductAppWindowState and RenderReceipt from accumulating one flat field
set per Creative UI command.

## Problem

Recent work added broad flat fields for delete, undo, shell generation, manual
rebuild, auto-refresh, and stale-state diagnostics. The fields are useful, but
the structure is becoming hard to maintain and easy to copy incorrectly.

## Required Reads

- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`

## Scope

- Propose and implement a small grouping/helper structure for Creative UI command
  diagnostics, or a narrower copy/reset helper that removes repeated manual
  reset/copy slabs.
- Preserve existing externally asserted receipt keys unless the task explicitly
  records and updates every affected test.

## Acceptance

- Default/reset behavior is centralized.
- Command-specific receipt copying is less repetitive.
- Existing focused receipt/input tests pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_receipt_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not reduce runtime evidence quality.
- Do not break sticky receipt behavior used for live Creative UI debugging.
- Do not combine this with command routing changes unless E29 is already done or
  the diff remains small.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
- Behavior changed:
  - No external receipt keys or command behavior changed.
  - Added a local baked-room refresh diagnostic field bundle in
    `ReceiptBuilder.cpp` so manual Rebuild and auto-refresh diagnostics share
    reset/copy logic.
  - Moved baked-room refresh diagnostic recorders out of `InputFrame.cpp` and
    exposed them through `ReceiptBuilder.hpp`.
  - `recordProductCreativeUiCommandFrame(...)` now resets manual baked-room
    refresh diagnostics through one helper instead of a repeated field slab.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_receipt_tests product_creative_ui_input_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/window/InputFrame.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E30-command-receipt-diagnostics-shape.md`
- Evidence:
  - Focused CTest passed 2/2.
  - Focused build passed; existing unrelated `pointerLifecycle` initializer
    warnings remain in `InputFrame.cpp` and
    `product_creative_ui_input_frame_tests.cpp`.
- Concerns/deferred:
  - `ProductAppWindowState` still has flat command receipt mirrors for
    externally asserted receipt keys; this slice centralized copy/reset without
    renaming those externally visible fields.
