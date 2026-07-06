# E35: Creative Command Catalog Single Source

## Objective

Reduce feature-add friction for Creative UI commands by making command metadata
come from one catalog instead of several manually synchronized places.

## Problem

Adding a Creative command currently touches multiple surfaces:

- UI row emission in `creative/ui/Ui.cpp`
- Semantic id routing in `creative/bridge/UiCommandFrame.cpp`
- Command handler table in `UiCommandFrame.cpp`
- Receipt name switch in `ReceiptBuilder.cpp`
- Tests that pin row ids, command ids, and receipt names

E29 improved execution dispatch, but semantic rows and handler metadata can
still drift. A new row can be visible and still route to
`product_creative_ui_command_unknown_semantic`, or a command enum can exist
without a handler/name.

## Required Reads

- `src/app/iggy3d/creative/ui/Ui.cpp`
- `src/app/iggy3d/creative/ui/Ui.hpp`
- `src/app/iggy3d/creative/ui/UiDrawList.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/creative_ui_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`

## Scope

- Introduce a small catalog or shared spec for Creative command semantic ids,
  command kind, receipt name, and optional tool/object-kind payload.
- Preserve the current handler dispatch style from E29.
- Preserve externally emitted receipt keys.
- Add tests that every command row semantic id used by the UI resolves to a
  known command and every command kind has a receipt name/handler where needed.

## Acceptance

- Adding a new simple command should not require duplicating the semantic id in
  both UI construction and command routing.
- A missing handler/name should fail a focused test rather than fail only at
  runtime.
- Current commands still route exactly as before.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not reintroduce a branch ladder.
- Do not remove receipt evidence.
- Do not change command behavior or UI layout beyond catalog wiring.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
  - `src/app/iggy3d/creative/ui/Ui.cpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `tests/unit/creative_ui_tests.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
- Catalog/API added:
  - `ProductCreativeUiCommandCatalogEntry`
  - `productCreativeUiCommandCatalog()`
  - `findProductCreativeUiCommandBySemanticId(...)`
  - `findProductCreativeUiCommandByKind(...)`
  - `productCreativeUiCommandKindReceiptName(...)`
  - `productCreativeUiCommandKindHasHandler(...)`
- Behavior:
  - UI command row ids/labels for tool, rebuild, undo, create, selected-object
    visibility/lock/delete, and generate room shell now come from the shared
    catalog.
  - `UiCommandFrame` routes semantic ids through the shared catalog and keeps
    the E29 handler table dispatch.
  - `ReceiptBuilder` uses the catalog receipt-name helper instead of a separate
    command-kind switch.
  - Externally emitted semantic ids, receipt names, row order, and command
    behavior are unchanged.
- Tests added:
  - `creative_ui_tests` asserts every emitted command row resolves through the
    shared catalog and that row id/label/tool/object payloads match the catalog.
  - `product_creative_ui_command_frame_tests` asserts every catalog semantic id
    routes to its catalog command kind, has a handler, and has a matching
    receipt name.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/bridge/UiCommandFrame.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/bridge/UiCommandFrame.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/ui/Ui.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp /Users/kogaryu/iggy3d/tests/unit/creative_ui_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_creative_ui_command_frame_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E35-creative-command-catalog-single-source.md`
- Concerns/deferred:
  - The catalog is header-only to avoid CMake churn in this cleanup slice. If
    it grows beyond scalar metadata, moving it to a `.cpp` source would be the
    next cleanup.
