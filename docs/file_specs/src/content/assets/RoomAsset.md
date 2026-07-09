# File Spec

Files: `src/content/assets/RoomAsset.hpp`, `src/content/assets/RoomAsset.cpp`

Verified at: `33a36cc1`

## Owns

- Room asset data model for static meshes, anchors, openings, and spatial surfaces.
- Text parser for room asset TOML-like fixtures exported by ASCII/authoring paths.
- Spatial surface validation for point counts, normalized normals, traversal tags, collision masks, source mesh links, and opening links.

## Does Not Own

- Room asset generation from ASCII/editable/creative documents, package loading, runtime collision bake, movement traversal logic, renderer mesh optimization, or save persistence.

## Reads

- Room asset text, conversion tables, mesh/anchor/opening/spatial surface sections, traversal tag ids, and feet-to-meters values.

## Writes / Mutates

- Returns `RoomAssetParseResult` with a populated `RoomAsset` on success.
- Normalizes parsed spatial surface normals.
- Does not mutate app state, runtime state, files, or renderer resources.

## Calls Out To / Wires Out To

- Uses `validTraversalTag(...)` and `traversalTagId(...)`.
- Parsed room assets feed package loading, active room state, runtime movement/collision, AI reasoning, projection/view, renderer, and tests.

## Called By / Entry Points

- `parseRoomAssetText(...)`.
- `PackageLoader` calls this for room asset files.
- Grep proof: `rg -n "parseRoomAssetText|RoomAsset|RoomSpatialSurface|RoomStaticMeshAsset|RoomAnchorAsset" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- A valid room needs id/source file/source subset, at least one static mesh, and at least one anchor.
- Static meshes need ids, mesh ids, material ids, roles, and positive sizes.
- Spatial surfaces must have unique ids, known source mesh ids, valid point counts, finite points, valid normalized normals, valid traversal tags, and valid collision masks.
- Opening surfaces must reference a known opening and must not block actor/projectile.

## Tests / Proof Commands

- `rg -n "room_asset_loader_tests|ascii_room_asset_text_tests|ascii_room_asset_text_fixture_tests|render_room_mesh_geometry_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "parseRoomAssetText|room_asset_ok|room_invalid_spatial_surface|RoomSpatialSurfaceRole" tests/unit/room_asset_loader_tests.cpp tests/unit/ascii_room_asset_text_tests.cpp tests/unit/ascii_room_asset_text_fixture_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*` unless generated room asset fields change.
- `src/app/iggy3d/creative/adapters/RoomBake.*` unless creative bake output changes.
- `src/runtime/physics/*` unless collision interpretation changes.

## Update When

- Room asset fields, text parser keys, validation rules, spatial surface roles/shapes, traversal/collision mask contracts, or parse result meanings change.

## Do Not Update When

- Only a producer or consumer changes while preserving `RoomAsset` and parser contracts.
