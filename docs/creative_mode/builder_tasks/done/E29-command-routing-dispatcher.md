# E29: Creative UI Command Routing Dispatcher

## Objective

Reduce the branch-heavy Product Creative UI command routing path before more
commands are added.

## Problem

`UiCommandFrame.cpp` has a semantic table, but command execution is still a
manual branch ladder. Every new command adds more branch code and receipt
copying, making it easier to miss defaults, status fields, or tests.

## Required Reads

- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`

## Scope

- Keep the existing external command behavior.
- Move each command execution body behind a small handler/dispatcher structure.
- Keep the semantic id table or improve it, but avoid growing the central
  branch ladder.

## Acceptance

- Adding a command no longer requires editing one giant command branch.
- Existing command statuses/reasons remain stable.
- No behavior change for Select/Move/Measure/Navigate, Rebuild Room, Undo,
  Create Room, Create Crate, Toggle Visibility, Toggle Lock, Delete Selected, or
  Generate Room Shell.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add new product commands.
- Do not widen into InputFrame auto-refresh behavior.
- Do not remove existing receipt diagnostics unless replacing them with an
  equivalent tested structure.

## Completion Brief

- Modified `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`.
- Replaced the central command execution branch ladder with a local
  `ProductCreativeUiCommandHandlerEntry` table and small command handlers.
- Kept semantic id mapping, command kinds, status/reason strings, and receipt
  copy helpers intact.
- Preserved behavior for tool changes, Create Room/Crate, visibility/lock,
  Rebuild Room, Undo, Delete Selected, and Generate Room Shell.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/bridge/UiCommandFrame.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E29-command-routing-dispatcher.md`
