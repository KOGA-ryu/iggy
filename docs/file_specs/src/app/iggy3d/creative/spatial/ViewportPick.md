# File Spec

Files: `src/app/iggy3d/creative/spatial/ViewportPick.hpp`, `src/app/iggy3d/creative/spatial/ViewportPick.cpp`

Verified at: `c5537031`

## Owns

- Creative viewport pointer-to-grid-cell picking over projected cells.
- Viewport packet, pick request/receipt packets, pick statuses, and depth mode.
- Pointer-to-grid coordinate conversion.
- Fixed-Z, highest-Z-first, and lowest-Z-first cell search.

## Does Not Own

- Spatial projection cell generation.
- Product window/input routing.
- UI hit region routing.
- Creative object mutation or selection behavior after a target is picked.
- Renderer coordinate scaling policy outside the supplied viewport packet.

## Reads

- Viewport rectangle, grid size, pointer coordinates, z/depth mode, and caller-supplied `CreativeSpatialCell` span.
- Spatial grid helpers from `SpatialProjection.*`.

## Writes / Mutates

- Writes `CreativeViewportPickReceipt`.
- Does not mutate cells, documents, facade state, or selection state.

## Calls Out To / Wires Out To

- Calls `isValidGridSize(...)`, `isInsideGrid(...)`, and `toGridIndex(...)`.
- Used by product creative viewport pick frame and window input frame to convert live pointer clicks into creative targets.

## Called By / Entry Points

- `isValidCreativeViewportPickViewport(...)`.
- `pointerToCreativeGridCoord(...)`.
- `pickCreativeViewportCell(...)`.
- Grep proof: `rg -n "CreativeViewportPick|pickCreativeViewportCell|pointerToCreativeGridCoord" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Invalid grid, invalid viewport, missing cells, and out-of-viewport inputs produce explicit non-hit statuses.
- Viewport bounds are half-open at right/bottom edges.
- Fixed-Z mode rejects out-of-grid coordinates before cell search.
- Non-fixed depth modes validate XY first and scan depth in the requested direction.
- Hits copy object id/kind/occupancy/index/cell index and target id when it fits.
- Miss receipts include the fixed coordinate and grid index when in range.

## Tests / Proof Commands

- `creative_viewport_pick_tests`.
- `product_creative_viewport_pick_frame_tests`.
- `product_creative_pick_flow_tests`.
- `rg -n "creative_viewport_pick_tests|product_creative_viewport_pick_frame_tests|product_creative_pick_flow_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/spatial/SpatialProjection.*` unless cell schema or grid math changes.
- `src/app/iggy3d/creative/bridge/ViewportPickFrame.*` unless product frame wiring changes.
- `src/app/iggy3d/window/InputFrame.*` unless live input routing changes.
- `src/app/iggy3d/creative/tools/Select.*` unless selection command behavior changes.

## Update When

- Viewport conversion, pick request/receipt fields, depth search behavior, hit/miss status semantics, or target copying changes.

## Do Not Update When

- Only projected-cell generation, window routing, or selection mutation changes after a pick receipt is produced.
