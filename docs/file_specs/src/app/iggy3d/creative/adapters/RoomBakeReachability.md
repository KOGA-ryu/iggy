# File Spec

Files: `src/app/iggy3d/creative/adapters/RoomBakeReachability.hpp`, `src/app/iggy3d/creative/adapters/RoomBakeReachability.cpp`

Verified at: `3b482611`

## Owns

- Reachability validation adapter for creative document room bakes.
- `initialCreativeRoomBakeReachabilityReceipt(...)` default requested/not-requested receipt setup.
- `validateCreativeRoomBakeReachability(...)` projection from baked room walkable surfaces and anchors into a grid flood-fill receipt.
- Creative-room bake reachability reason codes for invalid cell size, no walkable cells, grid too large, no usable seeds, connected, and islands found.

## Does Not Own

- Room asset generation from creative objects.
- Core flood-fill algorithm.
- Grid footprint math.
- Runtime navigation, AI pathfinding, or patrol route generation.
- Active room installation or receipt field serialization.

## Reads

- `RoomAsset` spatial surfaces, anchors, surface roles, points, and anchor positions.
- `CreativeRoomBakeRequest` reachability enable flag and cell size.
- Core grid footprint and reachability kernel results.

## Writes / Mutates

- Writes `CreativeRoomBakeReachabilityReceipt`.
- Builds local dense walkability projection and seed vectors.
- Does not mutate the room asset or creative document.

## Calls Out To / Wires Out To

- Calls `gridFootprintForContainingBounds(...)` for walkable spatial surface projection.
- Calls `gridCellForPoint(...)` for anchor seed placement.
- Calls `floodFillReachability(...)` with four-way connectivity.
- Called by `RoomBake.cpp` after static meshes, anchors, and spatial surfaces are emitted.

## Called By / Entry Points

- `initialCreativeRoomBakeReachabilityReceipt(...)`.
- `validateCreativeRoomBakeReachability(...)`.
- Grep proof: `rg -n "RoomBakeReachability|validateCreativeRoomBakeReachability|initialCreativeRoomBakeReachabilityReceipt|floodFillReachability|creative_room_bake_reachability" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Disabled validation returns a not-requested receipt without grid work.
- Invalid or non-positive cell size rejects before projection.
- Only walkable spatial surfaces contribute walkable cells.
- Only `spawn`, `npc`, and `monster` anchors can seed reachability.
- Off-grid, blocked, non-finite, or unsupported anchors are counted as blocked or ignored, not forced reachable.
- Absence of usable seeds reports all walkable cells as stranded.
- Flood-fill failure maps to not-checked using the core reason code.

## Tests / Proof Commands

- `creative_document_room_bake_tests`.
- `reachability_tests`.
- `rg -n "creative_document_room_bake_tests|reachability_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBake.*` unless bake receipt integration changes.
- `src/core/grid/GridFootprint.*` unless projection-to-grid math changes.
- `src/core/grid/Reachability.*` unless flood-fill behavior changes.
- `src/content/assets/RoomAsset.hpp` unless spatial surface or anchor schema changes.

## Update When

- Creative-room reachability admission, projection, seed rules, status/reason codes, or flood-fill wiring changes.

## Do Not Update When

- Only object classification, static mesh creation, greedy floor merging, or active-room installation changes without changing reachability validation.
