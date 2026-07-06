# E58: Command Catalog Row Versus Kind Metadata

## Objective

Follow up E35 by separating row-level command catalog entries from command-kind
metadata, so helper APIs do not imply that every `ProductCreativeUiCommandKind`
has exactly one row.

## Problem

E35 correctly centralized semantic ids, row ids, labels, command kinds, payloads,
and receipt names. But the resulting catalog is row-level while some helper APIs
are command-kind-level:

- `ProductCreativeUiCommandKind::SetActiveTool` has four rows.
- `ProductCreativeUiCommandKind::CreateObject` has two rows today.
- `findProductCreativeUiCommandByKind(...)` returns the first row with the
  requested kind (`UiCommandCatalog.hpp:141`).
- `productCreativeUiCommandKindReceiptName(...)` then derives command-kind
  metadata from that first row (`UiCommandCatalog.hpp:152`).

This works only because duplicate command-kind rows currently share the same
receipt name. The API shape is still misleading: a future command kind with
multiple row payloads can accidentally depend on first-row behavior.

## Required Reads

- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/ui/Ui.cpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/creative_ui_tests.cpp`

## Scope

- Keep the row catalog for semantic id, row id, label, and row payload.
- Add a separate command-kind metadata helper/table for kind-level facts such as
  receipt name and handler-required/handler-present expectations.
- Replace command-kind metadata calls that currently depend on first matching
  row lookup.
- Keep row construction from the row catalog unchanged where it is correct.
- Add a focused invariant test that duplicate row entries for the same command
  kind cannot accidentally disagree with command-kind metadata.

## Acceptance

- `findProductCreativeUiCommandByKind(...)` is removed, renamed to clarify
  first-row semantics, or no longer used for command-kind metadata.
- Receipt-name lookup is unambiguous and does not depend on row order.
- Handler coverage tests check command-kind metadata, while row routing tests
  check semantic ids and row payloads.
- Current external semantic ids, labels, row order, command routing, and
  RenderReceipt command names remain unchanged.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change visible Creative UI row order or labels.
- Do not remove the E35 row catalog.
- Do not change command behavior.
- Do not combine this with E47 receipt-substructure work.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
  - `src/app/iggy3d/creative/ui/Ui.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
- Removed receipt names from row-level catalog entries.
- Added `ProductCreativeUiCommandKindMetadata` and
  `productCreativeUiCommandKindMetadataCatalog()`.
- Added unambiguous kind-level helpers:
  - `findProductCreativeUiCommandKindMetadata(...)`
  - `productCreativeUiCommandKindReceiptName(...)`
  - `productCreativeUiCommandKindExpectsHandler(...)`
- Renamed first-row lookup to
  `findFirstProductCreativeUiCommandRowByKind(...)` for UI row construction.
- Row routing still uses semantic ids and row payloads from the row catalog.
- Receipt names now come only from command-kind metadata, not from first row
  lookup.
- Tests updated:
  - row routing checks semantic id, command kind, and handler presence;
  - kind metadata checks receipt-name lookup and handler expectations;
  - duplicate row kinds (`SetActiveTool`, `CreateObject`) are pinned without
    depending on row order for receipt metadata.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- Result: all checks passed.
