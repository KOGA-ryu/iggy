# File Spec

Files: `src/app/iggy3d/room_editor/AuthoringController.hpp`, `src/app/iggy3d/room_editor/AuthoringController.cpp`

Verified at: `9efd172d`

## Owns

- Product room authoring controller over `EditableRoomSession`.
- Authoring snapshots that bundle editable document, baked `RoomAsset`, scene projection, counts, and undo/redo depth.
- Submit/undo/redo acceptance flow that only commits when edit, bake, and projection rebuild succeed.
- Start/apply wrapper functions used by product menu, ASCII draft, active room, automation, and cursor-place paths.

## Does Not Own

- Low-level room edit command semantics, cursor input mapping, projection renderer, runtime session tick, or save file format.
- UI/HUD presentation of snapshots.

## Reads

- `EditableRoomSession`, `EditableRoomDocument`, room edit commands, ASCII/active-room authoring requests, room bake results, and scene projection results.

## Writes / Mutates

- Mutates the controller-owned editable session only after candidate edit and rebuild succeed.
- Returns operation/start snapshots and result packets.
- Does not mutate app window state directly.

## Calls Out To / Wires Out To

- `bakeEditableRoomDocument(...)`.
- `buildSceneProjection(...)`.
- `startProductRoomEditingFromAscii(...)` and `startProductRoomEditingFromActiveRoom(...)`.
- `applyProductRoomEditingCommand(...)`, `undoProductRoomEditing(...)`, and `redoProductRoomEditing(...)`.
- `applyProductRoomEditorActions(...)` for cursor-place wrapper.

## Called By / Entry Points

- `ProductRoomAuthoringController::submit(...)`, `undo(...)`, `redo(...)`.
- `buildProductRoomAuthoringSnapshot(...)`.
- `startProductRoomAuthoringFromAsciiDraft(...)`.
- `startProductRoomAuthoringFromActiveRoom(...)`.
- `applyProductRoomAuthoringEditCommand(...)`.
- `applyProductRoomAuthoringCursorPlace(...)`.
- Grep proof: `rg -n "ProductRoomAuthoringController|startProductRoomAuthoring|applyProductRoomAuthoring" src tests/unit`.

## Invariants

- Rejected edit, failed bake, or failed projection must preserve the previous accepted snapshot.
- Snapshot counts must come from the rebuilt document/room/projection, not stale caller counts.
- Authoring controller owns product authoring state; content authoring remains the source of edit command truth.
- Empty `SessionState` use in scene projection is local snapshot projection, not gameplay state ownership.

## Tests / Proof Commands

- `rg -n "product_room_authoring_controller_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "ProductRoomAuthoringController|buildProductRoomAuthoringSnapshot|applyProductRoomAuthoringCursorPlace" tests/unit/product_room_authoring_controller_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/content/authoring/EditableRoomDocument.*` unless edit command semantics change.
- `src/projection/scene/SceneProjection.*` unless projection output contract changes.
- `src/app/iggy3d/ascii_room/*` unless ASCII authoring input changes.

## Update When

- Authoring snapshot fields, submit/undo/redo acceptance gates, rebuild behavior, start wrappers, or cursor-place wrappers change.

## Do Not Update When

- Only presentation/HUD labels, input binding names, or render backend behavior changes.
