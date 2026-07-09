# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomToEditableRoom.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomToEditableRoom.cpp`

Verified at: `b49a78ae`

## Owns

- Conversion from compiled ASCII authored-room records into `EditableRoomDocument`.
- Copying authored metadata, floor records, wall records, object records, semantics, lock/hidden flags, and conversion counts into an editable-room result.

## Does Not Own

- Source parsing, grid construction, authored-room compile rules, room editor commands, active-room state, save codec, or runtime room asset baking.

## Reads

- `AsciiRoomGrid`, `AsciiRoomCompileConfig`, `AsciiRoomAuthoredRoomResult`, `SaveAuthoredRoomSection`, floor/wall/object records, and authored semantics records.

## Writes / Mutates

- Returns `AsciiRoomToEditableRoomResult` with status, reason code, editable document, authored-room result, diagnostics, and count fields.
- Does not mutate the input grid, app window, active room, files, or runtime session.

## Calls Out To / Wires Out To

- `compileAsciiRoomToAuthoredRoom(...)`.
- `buildEditableRoomDocumentFromAuthoredRoom(...)`.
- Consumed by ASCII editing and room editor authoring state.

## Called By / Entry Points

- `buildEditableRoomFromAsciiRoom(...)`.
- `buildEditableRoomDocumentFromAuthoredRoom(...)`.
- Grep proof: `rg -n "buildEditableRoomFromAsciiRoom|buildEditableRoomDocumentFromAuthoredRoom" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Failed authored-room compile propagates status, reason, diagnostics, and counts without fabricating an editable document.
- Authored metadata gets stable fallbacks for empty id/source/source file/subset.
- Editable floors, walls, and objects preserve authored ids, story index, transforms, dimensions, semantics, and lock/hidden flags.
- Object blocker flags come from authored object semantics.

## Tests / Proof Commands

- `rg -n "ascii_room_to_editable_room_tests|product_room_authoring_controller_tests|editable_room_to_authored_room_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildEditableRoomFromAsciiRoom|buildEditableRoomDocumentFromAuthoredRoom" tests/unit/ascii_room_to_editable_room_tests.cpp tests/unit/product_room_authoring_controller_tests.cpp tests/unit/editable_room_to_authored_room_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.*` unless authored-room records change.
- `src/content/authoring/EditableRoomDocument.*` unless editable document schema changes.
- `src/app/iggy3d/room_editor/*` unless editor controller consumption changes.

## Update When

- Authored-to-editable field mapping, metadata fallbacks, diagnostics propagation, or count fields change.

## Do Not Update When

- Only ASCII source parsing, grid semantics, or room asset baking changes without changing editable-room conversion.
