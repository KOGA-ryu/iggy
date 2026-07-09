# File Spec

Files: `src/core/grid/GreedyMesh.hpp`, `src/core/grid/GreedyMesh.cpp`

Verified at: `3b482611`

## Owns

- Pure deterministic greedy meshing of a keyed dense grid into axis-aligned quads.
- `GreedyMeshGrid`, `GreedyQuad`, and `GreedyMeshReceipt`.
- Row-major scan, width-first then depth extension for same-key cells.
- Exact coverage reporting for non-zero cells.

## Does Not Own

- Projection from room/editor geometry into grid cells.
- Mesh asset creation or material policy.
- Creative floor source ordering.
- Renderer/Vulkan triangle emission.
- Runtime collision or physics grids.

## Reads

- Caller-filled row-major key grid where zero means empty and non-zero keys identify merge-compatible cells.
- Grid dimensions and key vector size.

## Writes / Mutates

- Writes `GreedyMeshReceipt` with quads, filled-cell count, quad count, ok flag, and reason code.
- Allocates local consumed-cell vector.
- Does not mutate the input key grid.

## Calls Out To / Wires Out To

- Called by creative room bake greedy floor planning.
- Unit tests call it directly to prove exact coverage, key separation, holes, empty grids, and invalid grids.

## Called By / Entry Points

- `greedyMeshGrid(...)`.
- Grep proof: `rg -n "greedyMeshGrid|GreedyMeshGrid|GreedyMeshReceipt" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Grid dimensions must be positive and `keys.size()` must equal width times depth.
- Key zero cells are never covered by quads.
- Quads merge only cells with the same non-zero key.
- Every filled cell is covered exactly once.
- Quads must not overlap.
- The scan order and width-first growth are deterministic.
- Receipt `quadCount` must equal `quads.size()`.

## Tests / Proof Commands

- `greedy_mesh_tests`.
- `creative_document_room_bake_tests`.
- `rg -n "greedy_mesh_tests|creative_document_room_bake_tests|greedyMeshGrid" cmake/iggy3d_tests.cmake tests/unit src`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.*` unless creative floor merge policy changes.
- `src/core/grid/GridFootprint.*` unless footprint generation changes.
- `src/core/grid/Reachability.*` unless flood-fill behavior changes.

## Update When

- Greedy meshing admission, merge order, quad shape semantics, reason codes, or exact-coverage guarantees change.

## Do Not Update When

- Only a caller changes how it converts quads into room meshes or render geometry.
