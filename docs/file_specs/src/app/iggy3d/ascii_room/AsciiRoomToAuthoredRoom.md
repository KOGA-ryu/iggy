# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`

Verified at: `93b82ea7`

## Owns

- Conversion from `AsciiRoomGrid` cells into `SaveAuthoredRoomSection`.
- Authored floor, wall, object, and marker records derived from ASCII cells.
- Terrain surface metadata for elevated floors, ramps, blocked slopes, and normals.
- Authored-room compile config for tile size, wall/floor dimensions, story index, source name, and origin-centering.

## Does Not Own

- Source text parsing, grid-cell construction, runtime `RoomAsset` baking, active room state, save codec format, or room editor mutation.
- Object catalog behavior beyond selecting authored object ids/semantics from grid cell facts.

## Reads

- `AsciiRoomGrid`, `AsciiRoomCell`, terrain/elevation/object/marker fields, compile config, and traversal tag ids.

## Writes / Mutates

- Returns `AsciiRoomAuthoredRoomResult` containing authored-room save records, terrain surfaces, marker records, counts, and diagnostics.
- Does not mutate the input grid, files, app window state, or runtime session.

## Calls Out To / Wires Out To

- Consumed by `AsciiRoomToEditableRoom.*`, `AsciiRoomToRoomAsset.*`, product ASCII authoring, fixture tests, and save/active-room paths.
- Uses traversal tag ids from content assets for authored semantics.

## Called By / Entry Points

- `compileAsciiRoomToAuthoredRoom(...)`.
- Grep proof: `rg -n "compileAsciiRoomToAuthoredRoom" src tests/unit`.

## Invariants

- Empty grids must fail with explicit status/diagnostic instead of producing a present authored room.
- Authored room records must preserve source ids, source file/subset, row/column/source coordinates, story index, and object/marker facts.
- Walkable floors, blockers, ramps, and blocked slopes must carry semantics used later by collision and traversal.
- Terrain surface counts must align with authored floor and grid terrain facts.

## Tests / Proof Commands

- `rg -n "ascii_room_to_authored_room_tests|ascii_room_fixture_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "compileAsciiRoomToAuthoredRoom|blockedSlope|rampCount|authoredRoom" tests/unit/ascii_room_to_authored_room_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomGrid.*` unless cell facts change.
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*` unless authored-room to room-asset baking changes.
- `src/runtime/save/SaveEnvelope.hpp` unless authored-room save records change.

## Update When

- Authored floor/wall/object/marker records, terrain surfaces, compile config, semantics, counts, or diagnostics change.

## Do Not Update When

- Only source parsing, product UI, active room receipts, or render backend behavior changes.
