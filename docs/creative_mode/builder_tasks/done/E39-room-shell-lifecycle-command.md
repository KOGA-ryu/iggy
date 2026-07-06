# E39: Room Shell Lifecycle Command

## Objective

Add an explicit way to remove or regenerate generated Room shell children after
E27/E28 made Room shell generation persistent and parent deletes reject while
children exist.

## Problem

Generate Room Shell creates authored Floor/Wall children under a Room and E28
rejects deleting parents with children. That protects integrity, but it leaves a
workflow gap: once generated, the shell cannot be removed as a group through the
Creative UI, and duplicate generation is rejected.

This makes Room bounds edits and room-regeneration awkward.

## Required Reads

- `src/app/iggy3d/creative/tools/RoomShell.hpp/.cpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/ui/Ui.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `tests/unit/creative_room_shell_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

- Add a small planner/helper that finds generated shell children for a selected
  Room using the existing provenance tags and parent id.
- Add a visible selected-Room command such as `Remove Room Shell` or a
  conservative `Regenerate Room Shell` if removal-first is cleaner.
- Apply the change atomically through a document copy + `Facade::installDocument`
  so one undo snapshot covers the whole lifecycle operation.
- Let existing E18 auto-refresh handle activeRoom/collision updates.

## Acceptance

- Selected Room with generated shell exposes the lifecycle row.
- Command removes only generated shell children for that Room, not arbitrary
  parented objects.
- Undo restores the shell and auto-refresh reloads the baked room.
- Parent delete rejection remains intact.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests product_creative_ui_command_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|product_creative_ui_command_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add cascade delete to generic document removal in this slice.
- Do not remove non-generated children.
- Do not silently regenerate over an existing shell without deterministic policy.

## Completion Brief

Status: done.

Files modified:
- `src/app/iggy3d/creative/tools/RoomShell.hpp`
- `src/app/iggy3d/creative/tools/RoomShell.cpp`
- `src/app/iggy3d/creative/Facade.cpp`
- `src/app/iggy3d/creative/ui/Ui.hpp`
- `src/app/iggy3d/creative/ui/Ui.cpp`
- `src/app/iggy3d/creative/ui/UiDrawList.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/creative_room_shell_tests.cpp`
- `tests/unit/creative_ui_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

Implementation:
- Added pure RoomShell remove planning:
  `creativeRoomHasGeneratedShellChildren(...)` and
  `findCreativeRoomShellChildren(...)`.
- Remove planning matches only children with:
  - `parentId == selected room id`
  - `generated_room_shell`
  - `source_room_<room id>`
- Added lifecycle statuses:
  - `NoGeneratedShell`
  - `Removed`
  - `RemoveRejected`
- Added product Creative UI command:
  - semantic id: `creative.row.selection.remove_room_shell`
  - label: `Remove Room Shell`
  - command kind: `RemoveSelectedRoomShell`
  - receipt name: `remove_selected_room_shell`
- Added `CreativeUiObjectSummary::hasGeneratedRoomShell`; `Facade::buildUiModel`
  computes it from RoomShell provenance and the Remove row is visible only for
  selected Rooms with generated shell children.
- Command execution stages removals in a document copy and applies them with
  `Facade::installDocument(...)`; generic parent delete behavior is unchanged.
- Added `shellRemovedObjectCount` diagnostics and receipt mirror fields.

Tests:
- RoomShell unit tests prove the finder returns only tagged generated children
  and rejects selected Rooms with no generated shell.
- Creative UI tests prove Generate is still shown for selected Rooms and Remove
  appears only when `hasGeneratedRoomShell` is true.
- Command-frame tests prove Remove deletes only generated children, preserves
  unrelated parented children, and rejects when no generated shell exists.
- Product launch/input scenario now proves Generate Room Shell, parent-delete
  rejection, Remove Room Shell, E18 auto-refresh clear, and Undo restoration
  with active-room reload.

Verification:
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests creative_ui_tests product_creative_ui_draw_list_tests product_creative_ui_command_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|creative_ui_tests|product_creative_ui_draw_list_tests|product_creative_ui_command_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- focused trailing-whitespace scan over touched files

Results:
- Build passed.
- Focused CTest passed: 5/5 tests.
- Diff check passed.
- Focused trailing-whitespace scan found no matches.

Concerns:
- The lifecycle command is removal-only. Regeneration remains a deliberate
  follow-up policy decision because silently replacing an existing shell would
  need clear undo/provenance semantics.
