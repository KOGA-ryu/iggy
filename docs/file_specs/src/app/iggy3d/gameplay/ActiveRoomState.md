# File Spec

Files: `src/app/iggy3d/gameplay/ActiveRoomState.hpp`, `src/app/iggy3d/gameplay/ActiveRoomState.cpp`

Verified at: `93b82ea7`

## Owns

- Product active room state packet used by gameplay, projection, receipts, collision, editor, and save/load flows.
- Builders from ASCII authoring result, package room asset, saved authored room, and room authoring snapshot.
- Active room count projection for static meshes, anchors, openings, spatial surfaces, walkable surfaces, actor blockers, projectile blockers, and authored records.
- Active room revision bump helper.

## Does Not Own

- Room asset baking internals, active room collision query building, product room store accessors, save codec format, runtime session state, or renderer resources.
- Creative document editing; creative baked-room refresh may call this but remains a separate owner.

## Reads

- `ProductAsciiRoomAuthoringRequest`, `ProductAsciiRoomAuthoringResult`, `RoomAsset`, package/scenario ids, `SaveAuthoredRoomSection`, `ProductRoomAuthoringSnapshot`, and app window room store revision.

## Writes / Mutates

- Returns `ProductActiveRoomState` with loaded/status/source/id/counts and copied room/authored-room data.
- `bumpActiveRoomRevision(...)` increments the window-owned active room revision.
- Does not build collision state directly.

## Calls Out To / Wires Out To

- `buildRoomAssetFromAsciiRoom(...)` for saved authored room rebuild.
- `buildAuthoredRoomFromEditableRoomDocument(...)` for editor snapshot authored-room mirror.
- `activeRoomRevision(...)` from `ProductRoomStore.*`.
- Called by new world/session launch, ASCII activation, creative baked-room refresh, tape runner, automation, and tests.

## Called By / Entry Points

- `buildProductActiveRoomFromAsciiAuthoring(...)`.
- `buildProductActiveRoomFromPackageRoom(...)`.
- `buildProductActiveRoomFromSavedAuthoredRoom(...)`.
- `buildProductActiveRoomFromRoomAuthoringSnapshot(...)`.
- `bumpActiveRoomRevision(...)`.
- Grep proof: `rg -n "buildProductActiveRoomFromAsciiAuthoring|buildProductActiveRoomFromPackageRoom|buildProductActiveRoomFromSavedAuthoredRoom|buildProductActiveRoomFromRoomAuthoringSnapshot|bumpActiveRoomRevision" src tests/unit`.

## Invariants

- Failed source builders preserve failure status/reason and must not claim loaded active room.
- Loaded active rooms report `active_room_loaded` status and reason.
- Count fields must be derived from the copied room/authored-room data.
- Active room revision must bump when callers replace active room state.
- Collision readiness is owned by `ActiveRoomCollision.*`, not this file.

## Tests / Proof Commands

- `rg -n "product_active_room_state_tests|product_active_room_collision_tests|product_ascii_room_activation_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductActiveRoomFromAsciiAuthoring|buildProductActiveRoomFromPackageRoom|buildProductActiveRoomFromSavedAuthoredRoom|bumpActiveRoomRevision" tests/unit/product_active_room_state_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomCollision.*` unless collision surface construction changes.
- `src/app/iggy3d/gameplay/ProductRoomStore.*` unless storage/revision access changes.
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*` unless saved authored-room rebuild changes.

## Update When

- Active room packet fields, source builders, loaded/failure statuses, count derivation, authored-room mirroring, or revision bump semantics change.

## Do Not Update When

- Only collision query contents, renderer presentation, source parsing, or save text format changes without active-room packet contract changes.
