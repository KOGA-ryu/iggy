# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomCanvas.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomCanvas.cpp`

Verified at: `b49a78ae`

## Owns

- Tiny mutable ASCII canvas helper for tests/tools that construct room source text programmatically.
- Bounds-checked tile get/set, rectangle fill, room border/floor drawing, door placement, text serialization, and parsing into `AsciiRoomSource`.

## Does Not Own

- Glyph semantics, grid validation, authored-room conversion, product authoring, rendering, or runtime room state.

## Reads

- Canvas dimensions and tile vector.
- Optional source name for `parseAsciiRoomCanvas(...)`.

## Writes / Mutates

- Mutates `AsciiRoomCanvas::tiles` through set/fill/draw helpers.
- Returns canvas text or parsed `AsciiRoomSource`.
- Out-of-bounds writes return false or are ignored through helper calls.

## Calls Out To / Wires Out To

- `parseAsciiRoomCanvas(...)` calls `parseAsciiRoomSource(...)`.

## Called By / Entry Points

- Canvas test helpers and any product/unit tests that need generated ASCII room text.
- Grep proof: `rg -n "AsciiRoomCanvas|makeAsciiRoomCanvas|asciiRoomCanvas|parseAsciiRoomCanvas" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Tile storage is row-major with index `y * width + x`.
- Bounds checks reject negative coordinates and coordinates at or beyond width/height.
- `asciiRoomCanvasToText(...)` emits one newline per row.
- Canvas parsing delegates to source parsing and does not add glyph semantics.

## Tests / Proof Commands

- `rg -n "ascii_room_canvas_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "makeAsciiRoomCanvas|asciiRoomCanvasToText|parseAsciiRoomCanvas|out of bounds" tests/unit/ascii_room_canvas_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomSource.*` unless parse delegation changes.
- `src/app/iggy3d/ascii_room/AsciiRoomGrid.*` unless tests move from source text into grid semantics.

## Update When

- Canvas storage, bounds policy, drawing helpers, text serialization, or source parse delegation changes.

## Do Not Update When

- Only glyph validation, grid construction, or room authoring conversion changes.
