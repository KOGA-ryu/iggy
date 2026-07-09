# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomSource.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomSource.cpp`

Verified at: `93b82ea7`

## Owns

- ASCII room source text normalization, row splitting, source-line offsets, and source-name retention.
- Source-level diagnostics for empty, ragged, invalid-scale, invalid-layer, and unknown-glyph input.
- Tile-scale directive parsing and layered floor directive parsing.
- Source offset lookup for flat and layered ASCII room coordinates.

## Does Not Own

- Glyph-to-cell semantics, room grid validation, authored-room conversion, room asset creation, product activation, or save behavior.
- File loading; callers supply source text.

## Reads

- Raw ASCII room text, optional source name, scale directive rows, layer directive rows, and glyph validity from `AsciiRoomGrid.*`.

## Writes / Mutates

- Returns `AsciiRoomSource` with normalized text, rows/layers, dimensions, directives, status, reason code, diagnostics, and offset tables.
- Does not mutate app window state, room editor state, runtime session, or files.

## Calls Out To / Wires Out To

- Uses `asciiRoomGlyphInfo(...)` to validate known glyphs.
- Feeds `buildAsciiRoomGrid(...)`, ASCII canvas conversion, product authoring, room fixture tests, and room asset conversion paths.

## Called By / Entry Points

- `parseAsciiRoomSource(...)`.
- `asciiRoomSourceOffset(...)`.
- Grep proof: `rg -n "parseAsciiRoomSource|asciiRoomSourceOffset" src tests/unit`.

## Invariants

- Newline normalization must support CRLF and LF while preserving useful source offsets.
- A successful source has status and reason code `ascii_room_ok`.
- Layered sources require layer directives, equal dimensions, and valid non-hole glyphs.
- Scale directives must be positive finite numbers.
- Source parsing should reject bad text before grid or authored-room conversion.

## Tests / Proof Commands

- `rg -n "ascii_room_source_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "parseAsciiRoomSource|asciiRoomSourceOffset|ascii_room_invalid_scale|ascii_room_unknown_glyph" tests/unit/ascii_room_source_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomGrid.*` unless glyph validation or grid cell semantics change.
- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.*` unless source fields consumed by conversion change.
- `src/app/iggy3d/ascii_room/AsciiRoomAssetText.*` unless fixture text loading changes.

## Update When

- Source text normalization, directive parsing, layer parsing, diagnostics, dimensions, offset mapping, or glyph validation gates change.

## Do Not Update When

- Only grid semantics, room asset baking, product activation, or renderer behavior changes.
