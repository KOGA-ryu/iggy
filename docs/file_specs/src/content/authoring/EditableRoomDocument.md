# File Spec

Files: `src/content/authoring/EditableRoomDocument.hpp`, `src/content/authoring/EditableRoomDocument.cpp`

Verified at: `33a36cc1`

## Owns

- Editable room document model for floors, walls, objects, semantics, edit commands, edit results, bake results, and undo/redo session history.
- Validation and mutation of editable room primitives.
- Baking editable floors/walls/objects into `RoomAsset` meshes and spatial surfaces.
- Runtime id derivation for edited primitives.

## Does Not Own

- Product room-editor UI/controller flow, ASCII parsing, authored-room save records, active-room state, runtime collision baking, renderer geometry, or save/hash persistence.

## Reads

- `EditableRoomDocument`, `RoomEditCommand`, traversal tag catalog, `Vec3`, `Aabb3`, and primitive lock/hidden/semantics fields.

## Writes / Mutates

- `applyRoomEditCommand(...)` mutates an editable document.
- `EditableRoomSession::submit/undo/redo` mutates session document and history stacks.
- `bakeEditableRoomDocument(...)` returns a `RoomAsset`; it does not mutate runtime state.

## Calls Out To / Wires Out To

- Uses traversal tag helpers from `content/assets/TraversalTag.*`.
- Produces `RoomAsset` for room editor authoring, creative room baking, active-room rebuilds, ASCII editable conversion, and tests.

## Called By / Entry Points

- Command factories such as `addFloorCommand(...)`, `moveWallCommand(...)`, and `addObjectCommand(...)`.
- `applyRoomEditCommand(...)`, `bakeEditableRoomDocument(...)`, `roomEditStatusName(...)`, and `EditableRoomSession`.
- Grep proof: `rg -n "EditableRoomDocument|EditableRoomSession|applyRoomEditCommand|bakeEditableRoomDocument|RoomEditCommand" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Primitive ids must be non-empty and alphanumeric/underscore/hyphen.
- Duplicate primitive ids are rejected across floors, walls, and objects.
- Locked primitives reject destructive or mutating edits.
- Wall geometry remains finite and axis-aligned.
- Object blockers produce actor/projectile blocker surfaces when enabled.
- Undo/redo stores before/after document snapshots and clears redo on new submit.

## Tests / Proof Commands

- `rg -n "editable_room_document_tests|ascii_room_to_editable_room_tests|product_room_editor_preview_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "applyRoomEditCommand|bakeEditableRoomDocument|EditableRoomSession|RoomEditStatus" tests/unit/editable_room_document_tests.cpp tests/unit/ascii_room_to_editable_room_tests.cpp tests/unit/product_room_editor_preview_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/*` unless product editor control flow changes.
- `src/app/iggy3d/ascii_room/AsciiRoomToEditableRoom.*` unless authored-to-editable mapping changes.
- `src/runtime/physics/*` unless spatial surface semantics change.

## Update When

- Editable primitive fields, edit command semantics, validation policy, runtime id mapping, bake output, status names, or undo/redo behavior changes.

## Do Not Update When

- Only product UI, active-room refresh, renderer, or save serialization changes without changing editable document contracts.
