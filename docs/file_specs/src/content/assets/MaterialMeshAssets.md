# File Spec

Files: `src/content/assets/MaterialAsset.hpp`, `src/content/assets/MaterialAsset.cpp`, `src/content/assets/MeshAsset.hpp`, `src/content/assets/MeshAsset.cpp`

Verified at: `33a36cc1`

## Owns

- Material and primitive mesh asset packet shapes.
- Text parsers for `[[materials]]` and `[[primitive_meshes]]` fixture asset files.
- Basic asset validation for ids, supported material/mesh kinds, colors, size, and material references.

## Does Not Own

- Package file discovery, renderer material upload, renderer mesh generation, room asset parsing, or gameplay collision/traversal semantics.

## Reads

- Material asset text, mesh asset text, color vectors, mesh sizes in feet, kind strings, ids, and material ids.

## Writes / Mutates

- Returns `MaterialAssetParseResult` or `MeshAssetParseResult` with parsed libraries.
- Converts mesh sizes from feet to meters.
- Does not mutate files, package state, renderer state, or runtime state.

## Calls Out To / Wires Out To

- Called by `PackageLoader` when asset paths identify material or mesh asset files.

## Called By / Entry Points

- `parseMaterialAssetText(...)`.
- `parseMeshAssetText(...)`.
- Grep proof: `rg -n "parseMaterialAssetText|parseMeshAssetText|MaterialAssetLibrary|MeshAssetLibrary" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Material parser only accepts `vertex_color` records with non-empty ids.
- Mesh parser only accepts `box` primitive records with non-empty ids/material ids and positive sizes.
- Empty libraries fail explicitly.
- Parsers strip comments outside strings and reject keys outside known tables.

## Tests / Proof Commands

- `rg -n "room_asset_loader_tests|package_loader_tests|parseMaterialAssetText|parseMeshAssetText" cmake/iggy3d_tests.cmake tests/unit src/content`.

## Nearby Files Usually Not Touched

- `src/content/PackageLoader.*` unless asset dispatch changes.
- `src/render/*` unless renderer consumption changes.
- `src/content/assets/RoomAsset.*` unless room asset parser schema changes.

## Update When

- Material/mesh packet fields, parser grammar, supported kinds, unit conversion, validation, or package-loader integration changes.

## Do Not Update When

- Only renderer upload, room asset generation, or package manifest grammar changes.
