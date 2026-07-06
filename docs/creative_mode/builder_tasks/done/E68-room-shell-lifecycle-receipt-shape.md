# E68: RoomShell Lifecycle Receipt Shape

## Objective

Separate RoomShell generate/remove lifecycle status and receipt shape so future
room-shell operations do not keep accreting onto `CreativeRoomShellBuildStatus`.

## Problem

E39 added a useful remove lifecycle command, but it extended the existing
`CreativeRoomShellBuildStatus` enum with remove-specific outcomes. The result is
functionally correct but semantically muddled:

- Build receipts and remove receipts both report `CreativeRoomShellBuildStatus`.
- `CreativeRoomShellBuildStatus` now includes `NoGeneratedShell`, `Removed`, and
  `RemoveRejected`.
- Command-frame apply failures reuse the same status enum for create failures,
  remove failures, and install failures.

That raises the cost of the next lifecycle operation, especially regenerate:
builders have to infer whether a status belongs to planning, generation,
removal, or command apply. Tests can stay green by asserting strings like
`Removed`, while the API name still says "BuildStatus".

## Evidence

- `src/app/iggy3d/creative/tools/RoomShell.hpp:12` defines
  `CreativeRoomShellBuildStatus`.
- `src/app/iggy3d/creative/tools/RoomShell.hpp:20` includes remove-specific
  statuses `NoGeneratedShell` and `Removed` in the build status enum.
- `src/app/iggy3d/creative/tools/RoomShell.hpp:62` uses
  `CreativeRoomShellBuildStatus` as the remove receipt status type.
- `src/app/iggy3d/creative/tools/RoomShell.cpp:67` has a second `setStatus`
  overload for remove receipts but still takes `CreativeRoomShellBuildStatus`.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp:370` reports
  `RemoveRejected` through the same shell status channel used by generation.

## Required Reads

- `src/app/iggy3d/creative/tools/RoomShell.hpp`
- `src/app/iggy3d/creative/tools/RoomShell.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `tests/unit/creative_room_shell_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`

## Scope

- Rename or split the RoomShell status model so API names match lifecycle
  meaning. Acceptable shapes include:
  - `CreativeRoomShellStatus` for shared lifecycle outcomes, or
  - separate build/remove status enums plus a command-level shell operation
    receipt.
- Keep external command receipt strings stable unless a test is deliberately
  updated with a better name.
- Keep RoomShell generate/remove behavior unchanged.
- Keep E39's remove-only policy unchanged; do not add regeneration here.

## Acceptance

- Public API no longer reports remove outcomes through a type named
  `CreativeRoomShellBuildStatus`.
- Generate and remove unit tests still prove the same behavior and reason codes.
- Command-frame shell receipt tests still prove external diagnostics, but the
  internal type names no longer imply that every shell lifecycle operation is a
  build.
- No generic document cascade delete or regeneration policy is introduced.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add Room shell regeneration.
- Do not weaken the E39 guarantee that only generated shell children are
  removed.
- Do not combine this with E56's reusable batch transaction helper.
- Do not combine this with E57's provenance helper.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/tools/RoomShell.hpp`
  - `src/app/iggy3d/creative/tools/RoomShell.cpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
  - `tests/unit/creative_room_shell_tests.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- Replaced `CreativeRoomShellBuildStatus` with shared
  `CreativeRoomShellStatus`.
- `CreativeRoomShellBuildReceipt` and `CreativeRoomShellRemoveReceipt` now both
  report through `CreativeRoomShellStatus`, so remove outcomes no longer flow
  through a type named `BuildStatus`.
- Kept existing external status strings and reason codes stable:
  - `Generated`
  - `NoGeneratedShell`
  - `Removed`
  - `CreateRejected`
  - `RemoveRejected`
  - `InstallRejected`
  - existing `creative_room_shell_*` reason codes.
- No behavior change to generate/remove planning, provenance policy,
  remove-only policy, command execution, or external command receipt values.
- No regeneration or cascade delete policy added.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.
