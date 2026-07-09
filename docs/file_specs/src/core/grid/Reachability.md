# File Spec

Files: `src/core/grid/Reachability.hpp`, `src/core/grid/Reachability.cpp`

Verified at: `3b482611`

## Owns

- Pure dense-grid reachability flood-fill kernel.
- `ReachabilityGrid`, `ReachabilityCoord`, connectivity enum, and `ReachabilityReceipt`.
- Four-way and eight-way flood expansion over caller-provided walkable cells.
- Deterministic reached-cell and stranded-cell reporting.

## Does Not Own

- Projection from world geometry to a walkability grid.
- Seed selection policy.
- Runtime navigation graph generation.
- Creative-room bake status mapping.
- App/window/render behavior.

## Reads

- Caller-filled row-major walkable grid and seed coordinate span.
- Connectivity mode.

## Writes / Mutates

- Writes `ReachabilityReceipt`, including reached bitmap, counts, all-walkable-reached flag, and reason code.
- Allocates local frontier and reached vectors.
- Does not mutate the input grid or seed span.

## Calls Out To / Wires Out To

- Called by creative room bake reachability validation.
- Unit tests call it directly to validate deterministic seed behavior and connectivity modes.

## Called By / Entry Points

- `floodFillReachability(...)`.
- Grep proof: `rg -n "floodFillReachability|ReachabilityGrid|ReachabilityReceipt" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Grid dimensions must be positive and `walkable.size()` must equal width times depth.
- Off-grid and non-walkable seeds are ignored.
- Frontier is vector-backed FIFO for deterministic traversal.
- Reordering seeds may not change the reached set.
- Walkable count, reached count, and stranded count must remain internally consistent.
- Empty valid walkable regions can report ok while not all-walkable-reached.

## Tests / Proof Commands

- `reachability_tests`.
- `creative_document_room_bake_tests`.
- `rg -n "reachability_tests|creative_document_room_bake_tests|floodFillReachability" cmake/iggy3d_tests.cmake tests/unit src`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBakeReachability.*` unless creative room seed/projection policy changes.
- `src/core/grid/GridFootprint.*` unless world-to-grid conversion changes.
- `src/core/grid/GreedyMesh.*` unless rectangle meshing behavior changes.

## Update When

- Flood-fill admission, connectivity, determinism, receipt fields, reason codes, or reached/stranded counting changes.

## Do Not Update When

- Only a caller changes which cells are walkable or which seeds are supplied.
