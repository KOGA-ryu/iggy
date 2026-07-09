# File Spec

Files: `src/app/iggy3d/ascii_room/Authoring.hpp`, `src/app/iggy3d/ascii_room/Authoring.cpp`

Verified at: `b49a78ae`

## Owns

- Product ASCII authoring pipeline from source text to parsed source, grid, authored-room save records, room asset, optional asset text, diagnostics, and count proofs.
- `ProductAsciiRoomAuthoringRequest` and `ProductAsciiRoomAuthoringResult`.
- Stage failure reporting for source, grid, authored-room, room-asset, and asset-text stages.

## Does Not Own

- Low-level source parsing, grid semantics, authored-room conversion rules, room-asset baking internals, asset text formatting, package/session activation, save persistence, or UI state recording.

## Reads

- Request source text, source name, room id, source subset, tile/wall/floor dimensions, story index, asset text config, and movement-lab injection flag.
- Source tile scale after parsing.

## Writes / Mutates

- Returns a result with all intermediate packets and counts.
- Mutates only the local authored-room result by appending optional movement test lab objects before room asset bake.
- Does not mutate app window state, active session, files, or runtime state.

## Calls Out To / Wires Out To

- `parseAsciiRoomSource(...)`.
- `buildAsciiRoomGrid(...)`.
- `compileAsciiRoomToAuthoredRoom(...)`.
- `buildProductMovementTestLabObjects(...)`.
- `buildRoomAssetFromAsciiRoom(...)`.
- `writeAsciiRoomAssetText(...)`.

## Called By / Entry Points

- `buildProductAsciiRoomAuthoring(...)`.
- New-world launch, preview, activation, active-room tests, room editing, built-in dungeon, gameplay tape, and smokes use this surface.
- Grep proof: `rg -n "buildProductAsciiRoomAuthoring|ProductAsciiRoomAuthoring" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- The pipeline stops at the first failed stage and records that stage.
- Successful result uses status/reason `product_ascii_room_ready`.
- Tile size is multiplied by source tile scale before compile and room asset bake.
- Counts are copied from the latest available intermediates even on failure.
- Optional movement test lab object injection is request-gated and remains product authoring policy.

## Tests / Proof Commands

- `rg -n "product_ascii_room_authoring_tests|product_ascii_authoring_smoke|product_ascii_gameplay_loop_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "buildProductAsciiRoomAuthoring|failedStage|product_ascii_room_ready|injectMovementTestLabObjects" tests/unit/product_ascii_room_authoring_tests.cpp tests/smoke/product_ascii_authoring_smoke.cpp tests/smoke/product_ascii_gameplay_loop_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomGrid.*` and `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*` unless their contracts change.
- `src/app/iggy3d/world/MovementTestLab.*` unless optional object injection changes.
- `src/app/iggy3d/ascii_room/Activation.*` unless package/session activation changes.

## Update When

- Pipeline stages, request/result fields, failure statuses, count propagation, movement-lab injection, or emitted intermediates change.

## Do Not Update When

- Only one lower-level converter changes internally while preserving this pipeline contract.
