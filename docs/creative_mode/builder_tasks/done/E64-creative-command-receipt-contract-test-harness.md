# E64: Creative Command Receipt Contract Test Harness

## Objective

Replace the command receipt tests' long field-by-field scripts with a compact
contract harness that proves externally emitted receipt keys without blessing the
current flat implementation shape.

## Problem

`product_creative_ui_command_receipt_tests.cpp` mostly tests the current
flat-field receipt implementation by enumerating every key in large chained
assertion functions. This gives green coverage, but it also makes the flat
receipt shape feel more permanent than it should be.

Receipt keys are an external contract and should remain covered. The test shape
should not make it painful to introduce typed diagnostic groups or shared
append/copy helpers.

## Dependencies

- Coordinate with E47. If E47 restructures command diagnostics first, build
  this harness on top of the new grouped shape. If this lands first, keep the
  harness compatible with E47's intended subreceipt direction.

## Required Reads

- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp/.cpp`
- `docs/creative_mode/builder_tasks/ready/E47-creative-command-diagnostic-subreceipts.md`

## Evidence

- `defaultWindowReceiptCarriesNotRequestedFields` is
  `product_creative_ui_command_receipt_tests.cpp:180-444`, 265 lines.
- Command-specific receipt tests are also long field chains:
  - create: `734-813`
  - delete: `815-898`
  - undo: `900-975`
  - room shell: `977-1048`
- `recorderPreservesNeighboringFields` is `1050-1125` and manually pins many
  unrelated neighboring fields.

## Scope

- Add a small receipt contract harness, for example:
  - expected field table helpers,
  - grouped command receipt fixtures,
  - helper to assert absent/default fields by group,
  - helper to assert command-specific fields by group.
- Preserve the emitted `RenderReceipt` keys and values unless an explicitly
  coordinated receipt migration changes them.
- Keep tests focused on external receipt contract, not implementation copying
  mechanics.

## Acceptance

- The default receipt contract is represented as data/grouped expectations
  rather than a 250+ line assertion chain.
- Create, delete, undo, and room-shell receipt proofs share the same assertion
  harness.
- Tests still fail if an emitted receipt key disappears, changes value, or is
  assigned to the wrong command group.
- The tests do not require every command diagnostic to remain as a flat
  `ProductAppWindowState` field internally.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_ui_command_receipt_tests product_creative_ui_command_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not remove externally useful receipt keys.
- Do not reduce command-specific coverage to only "accepted" or "changed".
- Do not make the harness depend on receipt field insertion order unless order
  is explicitly part of the contract.

## Completion Brief

- Files modified:
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- Added a compact `ReceiptFieldExpectation` table harness with
  `expectReceiptFields(...)` so receipt contract assertions are grouped by
  diagnostic namespace instead of long boolean chains.
- Converted default, default-recorded, facade-missing, tool-select, mutation,
  create, delete, undo, room-shell, and neighbor-preservation receipt checks to
  the shared harness.
- Preserved all existing external receipt key/value assertions and added
  coverage for newer grouped command keys that were not pinned by the old
  default test:
  - `creative_ui_command_tool`
  - `creative_ui_command_object_kind`
  - locked mutation fields in the main default contract
  - room-shell removed-object count
  - command baked-room refresh diagnostics
- No production code changes.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_ui_command_receipt_tests product_creative_ui_command_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over `tests/unit/product_creative_ui_command_receipt_tests.cpp`
