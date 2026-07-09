# File Spec

Files: `src/app/iggy3d/room_editor/ActionController.hpp`, `src/app/iggy3d/room_editor/ActionController.cpp`

Verified at: `9efd172d`

## Owns

- Room editor action application from `ActionStateEntry` and `ActionState`.
- Mapping editor input actions to cursor movement, tool selection, wall rotation, place/apply, delete, undo, redo, and mouse-pick result packets.
- Delete target lookup for floor, wall, and object at the current cursor position.
- `ProductRoomEditorActionResult` status/operation/primitive proof fields.

## Does Not Own

- Keyboard/gamepad binding, top-level input ownership, room authoring session internals, render presentation, or save behavior.
- Map maker toggle policy.

## Reads

- `ProductRoomEditingState`, `ProductRoomEditorCursorState`, action entries, cursor placement commands, editable room document rows, and room authoring operation results.

## Writes / Mutates

- Returns updated editing state and cursor state in `ProductRoomEditorActionResult`.
- Does not mutate `ProductAppWindowState` directly; callers copy result state into window-owned authoring state.

## Calls Out To / Wires Out To

- `Cursor.*` for movement, tool cycling, wall rotation, placement command, and mouse picking.
- `EditingState.*` and `AuthoringController.*` paths for apply/undo/redo.
- Content authoring delete command factories for delete actions.

## Called By / Entry Points

- `applyProductRoomEditorAction(...)`.
- `applyProductRoomEditorActions(...)`.
- `applyProductRoomEditorMousePick(...)`.
- `window/InputFrame.*` and automation room-editing helpers.
- Grep proof: `rg -n "applyProductRoomEditorAction|applyProductRoomEditorActions|applyProductRoomEditorMousePick" src tests/unit`.

## Invariants

- Non-editor input actions must not mutate editor state.
- Not-ready editing state returns handled/not-ready instead of applying edits.
- Button actions must require pressed/down intent.
- Delete must target an existing primitive and must not fabricate ids.
- Batch application preserves the latest handled/non-ignored result while carrying forward editing/cursor state.

## Tests / Proof Commands

- `rg -n "product_room_editor_action_controller_tests|room_editor_input_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "applyProductRoomEditorAction|applyProductRoomEditorMousePick|EditorDelete|EditorUndo|EditorRedo" tests/unit/product_room_editor_action_controller_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/input/*` unless action enum/binding semantics change.
- `src/app/iggy3d/window/InputFrame.*` unless top-level routing changes.
- `src/content/authoring/EditableRoomDocument.*` unless edit command semantics change.

## Update When

- Editor action mapping, result status fields, delete targeting, mouse-pick behavior, or apply/undo/redo dispatch changes.

## Do Not Update When

- Only HUD labels, preview metrics, or rendering changes without action-controller behavior changes.
