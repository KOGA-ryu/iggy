# File Spec

Files: `src/core/grid/GridFootprint.hpp`, `src/core/grid/GridFootprint.cpp`

Verified at: `3b482611`

## Owns

- Core XZ grid footprint and point-to-cell math.
- `GridFootprint`, `GridCellCoord`, status/result packets, and `toString(GridFootprintStatus)`.
- Containing half-open footprint conversion for bounds.
- Aligned footprint conversion with tolerance.
- Containing cell conversion for points.

## Does Not Own

- Reachability flood-fill.
- Greedy meshing.
- Creative bake object classification.
- Runtime physics broadphase grids.
- Any app/window/render policy.

## Reads

- Numeric XZ bounds, XZ point coordinates, cell size, and optional alignment tolerance.
- Standard finite/range checks.

## Writes / Mutates

- Writes result packets containing ok/status/reason and calculated footprint or cell.
- Does not allocate persistent state or mutate caller-owned objects.

## Calls Out To / Wires Out To

- Used by creative room bake reachability projection.
- Used by creative room bake greedy floor planning.
- Tested directly by grid footprint unit tests.

## Called By / Entry Points

- `gridFootprintForContainingBounds(...)`.
- `gridFootprintForAlignedBounds(...)`.
- `gridCellForPoint(...)`.
- Grep proof: `rg -n "gridFootprintFor|gridCellForPoint" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Cell size must be finite and positive.
- Bounds and points must be finite.
- Empty or inverted bounds reject.
- Out-of-range integer conversions reject instead of wrapping.
- Containing bounds use floor for minimum cells and ceil for maximum exclusive cells.
- Aligned bounds require each boundary to be within tolerance of a grid line.
- Result reason codes are part of caller-visible diagnostics.

## Tests / Proof Commands

- `grid_footprint_tests`.
- `rg -n "grid_footprint_tests|gridFootprintForContainingBounds|gridFootprintForAlignedBounds|gridCellForPoint" cmake/iggy3d_tests.cmake tests/unit src`.

## Nearby Files Usually Not Touched

- `src/core/grid/Reachability.*` unless callers need flood-fill behavior changes.
- `src/core/grid/GreedyMesh.*` unless callers need mesh rectangle behavior changes.
- `src/app/iggy3d/creative/adapters/RoomBakeReachability.*` unless creative projection policy changes.
- `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.*` unless creative floor merge policy changes.

## Update When

- Footprint status packets, reason codes, integer conversion policy, half-open bounds behavior, alignment tolerance behavior, or point cell mapping changes.

## Do Not Update When

- Only a caller changes how it interprets valid footprints without changing this grid math contract.
