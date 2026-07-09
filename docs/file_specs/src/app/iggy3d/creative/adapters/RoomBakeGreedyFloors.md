# File Spec

Files: `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp`, `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.cpp`

Verified at: `791c40db`

## Owns

- Greedy merge planning for creative floor room-bake inputs.
- `RoomBakeGreedyFloorInput`, policy, source, and mesh-plan packets.
- Grouping floor candidates by height and material policy.
- Safe fallback to one mesh per input when footprints, overlap, grid limits, or greedy mesh receipt fail.
- Stable source tracking from merged floor mesh plans back to source creative object ids and document order.

## Does Not Own

- Creative object classification into floor candidates.
- Room asset bake receipt/status assignment.
- Walkable surface creation from greedy floor sources.
- Core greedy-mesh algorithm implementation.
- Renderer or Vulkan room mesh emission.

## Reads

- Aligned floor bounds and document order from `RoomBakeGreedyFloorInput`.
- `RoomBakeGreedyFloorPolicy` cell size, maximum cell budget, mesh id, material id, and role.
- Grid footprint and greedy mesh receipts from core grid helpers.

## Writes / Mutates

- Writes ordered `RoomBakeGreedyFloorMeshPlan` vectors.
- Emits fallback plans for unmergeable or unsafe candidates.
- Does not mutate the input span or source creative document.

## Calls Out To / Wires Out To

- Calls `gridFootprintForAlignedBounds(...)` to convert bounds to grid footprints.
- Calls `greedyMeshGrid(...)` to merge occupied cells into quads.
- Called by `RoomBake.cpp` before appending static floor meshes and walkable spatial surfaces.

## Called By / Entry Points

- `buildRoomBakeGreedyFloorPlan(...)`.
- Grep proof: `rg -n "RoomBakeGreedyFloor|buildRoomBakeGreedyFloorPlan|nearAlignedGreedyFloorDoesNotExpandAcrossNeighborCell|interleavedGreedyFloorSourcesKeepDocumentOrder" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Non-aligned or invalid grid footprints fall back to individual meshes.
- Candidate overlaps within a group fall back instead of merging ambiguous ownership.
- Grid size limits must prevent oversized allocation.
- Merged source records must be sorted by original document order.
- Final mesh plans must be sorted by first source document index, then mesh id.
- Single-source merged plans preserve the original single mesh id.

## Tests / Proof Commands

- `creative_document_room_bake_tests`.
- `rg -n "nearAlignedGreedyFloorDoesNotExpandAcrossNeighborCell|interleavedGreedyFloorSourcesKeepDocumentOrder|creative_document_room_bake_tests" tests/unit cmake/iggy3d_tests.cmake`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBake.*` unless caller candidate policy or source application changes.
- `src/core/grid/GridFootprint.*` unless aligned bounds footprint rules change.
- `src/core/grid/GreedyMesh.*` unless core greedy mesh behavior changes.
- `src/content/assets/RoomAsset.hpp` unless floor mesh asset fields change.

## Update When

- Floor merge grouping, fallback policy, source ordering, mesh id policy, grid footprint admission, or emitted mesh-plan semantics change.

## Do Not Update When

- Only non-floor bake behavior, active-room install, reachability validation, or renderer mesh presentation changes.
