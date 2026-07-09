# File Spec

Files: `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`, `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`

Verified at: `c5537031`

## Owns

- Creative object projection into a dense 3D grid cell list.
- Grid size/coord/bounds/index packets and conversion helpers.
- `CreativeSpatialProjectionReceipt` and projection status/profile/occupancy string helpers.
- Object projection dispatch for point, box, volume, line, path, and link profiles.
- Aggregate projection across object spans.

## Does Not Own

- Descriptor table policy that chooses projection/occupancy profiles.
- Viewport hit testing over projected cells.
- Wireframe/debug-line rendering.
- Runtime collision, room bake, or save/load behavior.
- Tool command execution.

## Reads

- Creative objects, visibility, transforms, bounds, path points, and descriptor projection/occupancy policies.
- Projection request grid size, cell size, clamp policy, and authoring-only inclusion.

## Writes / Mutates

- Writes projection receipts and `CreativeSpatialCell` vectors.
- Does not mutate creative objects, documents, or descriptors.

## Calls Out To / Wires Out To

- Calls `describeObject(...)` through projection/occupancy helpers.
- Used by creative viewport pick frame, document wireframe, wireframe frame, and tests.

## Called By / Entry Points

- Grid helpers: `isValidGridSize(...)`, `isInsideGrid(...)`, `toGridIndex(...)`, `toGridCoord(...)`.
- Projection helpers: `projectObjectToGrid(...)`, typed projectors, and `projectObjectsToGrid(...)`.
- Grep proof: `rg -n "CreativeSpatialProjection|projectObjectToGrid|projectObjectsToGrid|toGridIndex|toGridCoord" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Invalid grid/cell-size requests reject before projecting.
- Invalid or hidden objects do not project cells.
- Authoring-only objects are excluded unless requested.
- Point projections require in-grid transformed positions.
- Bounds-backed projections use floor/ceil world bounds and clamp only when requested.
- Path/link projections require finite path points and dedupe sampled segment cells when requested.
- Aggregate projection reports projected when any cells are emitted.

## Tests / Proof Commands

- `creative_spatial_projection_tests`.
- `creative_viewport_pick_tests`.
- `product_creative_world_launch_tests`.
- `rg -n "creative_spatial_projection_tests|creative_viewport_pick_tests|product_creative_world_launch_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless projection/occupancy policy changes.
- `src/app/iggy3d/creative/spatial/ViewportPick.*` unless hit-test consumption changes.
- `src/app/iggy3d/creative/document/DocumentWireframe.*` unless wireframe projection consumption changes.
- `src/app/iggy3d/creative/bridge/ViewportPickFrame.*` unless product frame wiring changes.

## Update When

- Grid coordinate/index math, projection requests/receipts, status semantics, object projection rules, aggregate behavior, or descriptor projection consumption changes.

## Do Not Update When

- Only UI drawing, product frame receipt recording, or descriptor row values change without changing projection mechanics.
