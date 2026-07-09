# File Spec

Files: `src/app/iggy3d/room_editor/EditingState.hpp`, `src/app/iggy3d/room_editor/EditingState.cpp`

Verified at: `f27cc3fc`

## Owns

- Product room-editing session state built around `ProductRoomAuthoringController`.
- Start flows from ASCII authoring or active-room authored-room data.
- Edit, undo, and redo operation wrappers that rebuild active-room and collision state after accepted authoring operations.
- Count mirrors for document, active room, collision, undo, and redo state.

## Does Not Own

- Low-level edit command semantics.
- Cursor-to-command conversion.
- Preview overlay drawing, HUD presentation, or automation command parsing.
- Runtime gameplay session state.

## Reads

- ASCII room authoring request/result.
- Active-room authored-room data.
- Room authoring controller snapshots.
- Active-room and active-room collision build results.

## Writes / Mutates

- Mutates `ProductRoomEditingState` when an accepted edit, undo, or redo rebuilds successfully.
- Returns start and operation result packets.
- Does not mutate runtime session or window state directly.

## Calls Out To / Wires Out To

- Calls `buildProductAsciiRoomEditing`.
- Calls `buildEditableRoomDocumentFromAuthoredRoom`.
- Calls `buildProductActiveRoomFromRoomAuthoringSnapshot`.
- Calls `buildProductActiveRoomCollision`.
- Used by room-editor action controller, authoring controller facade helpers, automation, and window input.

## Called By / Entry Points

- `buildProductRoomEditingState`.
- `startProductRoomEditingFromAscii`.
- `startProductRoomEditingFromActiveRoom`.
- `applyProductRoomEditingCommand`.
- `undoProductRoomEditing`.
- `redoProductRoomEditing`.
- Focused proof: `rg -n "room_editing_state|product_room_editing|startProductRoomEditing|applyProductRoomEditingCommand|undoProductRoomEditing|redoProductRoomEditing|buildProductRoomEditingState" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- State is ready only after authoring snapshot, active room, and collision state all build successfully.
- Failed starts preserve a concrete failed stage or reason code.
- Edit/undo/redo operate on a candidate controller and commit back only after rebuild succeeds.
- Count mirrors must be refreshed after each build and accepted operation.
- This state coordinates product room editing; it must not become the owner of reusable document kernels.

## Tests / Proof Commands

- `product_room_editing_state_tests` covers ASCII start, active-room start, failure cases, edit, undo, and redo.
- `product_room_editor_action_controller_tests` covers action-controller consumers.
- `product_room_editing_automation_smoke` covers no-window automation flow.
- `rg -n "product_room_editing_state_tests|product_room_editor_action_controller_tests|product_room_editing_automation_smoke" cmake/iggy3d_tests.cmake tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/AuthoringController.*` unless authoring controller facade changes.
- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active-room rebuild changes.
- `src/app/iggy3d/gameplay/ActiveRoomCollision.*` unless collision rebuild changes.
- `src/app/iggy3d/automation/AutomationRoomEditing.*` unless product automation flow changes.

## Update When

- Room-editing state fields, start flow, rebuild order, count mirrors, accepted-operation semantics, or failure statuses change.

## Do Not Update When

- Only low-level edit command contents or presentation/HUD formatting changes.
