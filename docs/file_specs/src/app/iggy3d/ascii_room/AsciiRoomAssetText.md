# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomAssetText.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`

Verified at: `b49a78ae`

## Owns

- Export of a `RoomAsset` into deterministic ASCII-room asset text.
- Feet/meters conversion formatting for room metadata, meshes, openings, spatial surfaces, and anchors.
- Export validation for required room fields, conversion scale, meshes, anchors, and unsafe strings.
- Traversal tag normalization for exported spatial surfaces.

## Does Not Own

- Room asset construction, authored-room conversion, fixture file IO, TOML parsing, runtime collision, or renderer geometry.

## Reads

- `RoomAsset`, static meshes, openings, spatial surfaces, anchors, traversal tags, collision masks, and `AsciiRoomAssetTextConfig`.

## Writes / Mutates

- Returns `AsciiRoomAssetTextResult` with status, reason code, generated text, and count proofs.
- Does not mutate the room asset, app state, files, or runtime state.

## Calls Out To / Wires Out To

- Uses traversal tag helpers from `content/assets/TraversalTag.hpp`.
- Called by product ASCII authoring when `emitAssetText` is enabled.

## Called By / Entry Points

- `writeAsciiRoomAssetText(...)`.
- Grep proof: `rg -n "writeAsciiRoomAssetText|AsciiRoomAssetText" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Invalid conversion scale, missing required metadata, missing meshes, missing anchors, or unsafe strings must fail with explicit status.
- Exported text ends with a newline.
- Spatial-surface export preserves shape, role, points, normal, traversal tags, collision mask, blocker flags, opening id, and runtime owner stable name.
- Traversal tags derived from surface role are de-duplicated with valid existing tags.

## Tests / Proof Commands

- `rg -n "ascii_room_asset_text_tests|ascii_room_asset_text_fixture_tests|traversal_tag_catalog_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "writeAsciiRoomAssetText|ascii_room_asset_text_unsafe_string|traversal_tags" tests/unit/ascii_room_asset_text_tests.cpp tests/unit/ascii_room_asset_text_fixture_tests.cpp tests/unit/traversal_tag_catalog_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*` unless source room asset fields change.
- `src/content/assets/RoomAsset.hpp` unless room asset schema changes.
- Fixture files unless the text format intentionally changes.

## Update When

- Text format, validation rules, conversion config, traversal tag export, count proofs, or room asset fields emitted by this exporter change.

## Do Not Update When

- Only upstream grid/authored-room conversion or downstream fixture loading changes without changing generated asset text.
