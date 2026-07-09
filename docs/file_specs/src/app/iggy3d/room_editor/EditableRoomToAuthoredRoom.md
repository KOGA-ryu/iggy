# File Spec

Files: `src/app/iggy3d/room_editor/EditableRoomToAuthoredRoom.hpp`, `src/app/iggy3d/room_editor/EditableRoomToAuthoredRoom.cpp`

Verified at: `f27cc3fc`

## Owns

- Conversion from `EditableRoomDocument` into `SaveAuthoredRoomSection`.
- `EditableRoomToAuthoredRoomResult` conversion receipt.
- Field mapping for editable floors, walls, objects, and room metadata into save-authored room records.

## Does Not Own

- Editable-room command application or undo/redo.
- Save file encoding/decoding.
- Active-room construction from authored rooms.
- Room editor UI, cursor, preview, or presentation state.

## Reads

- Editable document id, version, source fields, floors, walls, objects, semantics, lock flags, and hidden flags.
- Save-authored room record shapes from `runtime/save/SaveEnvelope.hpp`.

## Writes / Mutates

- Builds `SaveAuthoredRoomSection` inside the result.
- Sets floor, wall, object, and marker counts.
- Does not mutate the editable document.

## Calls Out To / Wires Out To

- Uses local conversion helpers for semantics, floor, wall, and object records.
- Called by active-room state building from room-authoring snapshots.

## Called By / Entry Points

- `buildAuthoredRoomFromEditableRoomDocument`.
- `src/app/iggy3d/gameplay/ActiveRoomState.cpp` consumes the conversion result.
- Focused proof: `rg -n "editable_room_to_authored_room_tests|buildAuthoredRoomFromEditableRoomDocument|EditableRoomToAuthoredRoomResult|editable_room_authored_ready" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Result status for successful conversion is `editable_room_authored_ready`.
- Empty document metadata uses stable fallback authored-room metadata.
- Object semantics are derived from asset id and object/prop gameplay tags.
- Markers are not synthesized here; marker count reflects the authored section.
- This adapter stays a save/authored-room conversion seam, not a gameplay/runtime owner.

## Tests / Proof Commands

- `editable_room_to_authored_room_tests` covers authored-room conversion and fallback/default fields.
- Active-room consumers are covered through `ActiveRoomState` and room-editor state tests.
- `rg -n "editable_room_to_authored_room_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/content/authoring/EditableRoomDocument.*` unless editable document schema changes.
- `src/runtime/save/SaveEnvelope.hpp` unless save-authored room schema changes.
- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active-room conversion flow changes.

## Update When

- Editable-to-authored field mapping, fallback metadata, counts, object semantics, or save-authored room schema usage changes.

## Do Not Update When

- Only room-editor cursor, UI preview, or command application behavior changes.
