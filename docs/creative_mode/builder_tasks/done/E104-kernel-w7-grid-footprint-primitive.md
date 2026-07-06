# E104: Kernel W7 - Grid Footprint Primitive

## Objective

Create a reusable core grid-footprint primitive for converting world-space
bounds/points into deterministic integer grid cells, then route RoomBake
reachability and greedy-floor grouping through it.

This card exists because the W-series mostly uses real kernels, but the adapter
glue still repeats brittle grid-boundary math. `RoomBake.cpp` currently owns
`cellCoordFor`, `ReachabilityFootprint`, `GreedyFloorFootprint`, aligned-cell
rounding, grid-origin bookkeeping, and oversized-grid checks locally. That is
the next kernel seam.

## Audit Evidence

- `src/app/iggy3d/creative/adapters/RoomBake.cpp` is now about 1,356 LOC with
  roughly 124 branch hits.
- The same file has multiple local grid-boundary concepts:
  - `cellCoordFor(...)`
  - `ReachabilityFootprint`
  - `GreedyFloorFootprint`
  - `greedyFloorFootprintForBounds(...)`
  - `walkableFootprintForSurface(...)`
  - seed anchor cell lookup
- A planner repair after E103 had to fix near-boundary aligned floor behavior
  because the adapter accepted rounded alignment but then used raw floor/ceil
  coordinates. That bug class should be impossible through one tested kernel
  primitive.

## Scope

Expected files:

- Add `src/core/grid/GridFootprint.hpp`
- Add `src/core/grid/GridFootprint.cpp`
- Add focused tests, likely `tests/unit/grid_footprint_tests.cpp`
- Update CMake/test registration as needed
- Update `src/app/iggy3d/creative/adapters/RoomBake.cpp` to consume the helper

Stay local to core grid + RoomBake adapter usage. Do not change RoomBake
semantics.

## Suggested API Shape

Use judgment, but keep this kind of shape:

- A small footprint struct:
  - `minCellX`
  - `minCellZ`
  - `maxCellXExclusive`
  - `maxCellZExclusive`
  - width/depth helpers if useful
- A status enum or receipt for invalid cell size, non-finite values,
  int32 overflow, empty footprint, and optional alignment failure.
- Two policies:
  - containing/half-open bounds: `floor(min/cell)` and `ceil(max/cell)`;
  - aligned/rounded bounds: all bounds must be within a tolerance of a grid
    line, then use rounded integer coordinates.
- A point-to-cell helper for seed anchors using containing-cell semantics.

Keep the primitive generic. It should not know about RoomBake, floors,
reachability, anchors, or Creative objects.

## Required Behavior

- RoomBake reachability projection remains behavior-identical.
- RoomBake greedy floor grouping remains behavior-identical.
- The near-boundary aligned-floor case remains pinned:
  - bounds `{0.99995, 0, 0}..{1.99995, 0.25, 1}` should produce one 1 m cell
    at x `[1,2)`, not expand to `[0,2)`.
- Negative coordinates and exact boundary coordinates must be explicitly tested.
- Oversized/out-of-int-range footprint behavior must be deterministic.

## Do Not

- Do not change RoomBake object eligibility.
- Do not change RoomBake mesh/anchor/spatial-surface output.
- Do not change reachability status policy except to route through the helper.
- Do not alter GreedyMesh, Reachability, Snap, descriptors, save/load,
  renderer/Vulkan, input, or standalone behavior.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/core/grid/Reachability.hpp`
- `src/core/grid/GreedyMesh.hpp`
- `src/core/math/Snap.hpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Acceptance

- New core grid-footprint tests pass.
- Existing `creative_document_room_bake_tests` pass unchanged except any
  expected include/test-target additions.
- Existing `render_room_mesh_geometry_tests` pass.
- `RoomBake.cpp` no longer owns duplicate floor/ceil/round footprint conversion
  logic for reachability and greedy floor grouping.
- No runtime output count changes for existing RoomBake tests.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d grid_footprint_tests creative_document_room_bake_tests render_room_mesh_geometry_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(grid_footprint_tests|creative_document_room_bake_tests|render_room_mesh_geometry_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Core API added:
- RoomBake routing changed:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `CMakeLists.txt`
  - `cmake/iggy3d_tests.cmake`
  - `src/core/grid/GridFootprint.hpp`
  - `src/core/grid/GridFootprint.cpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/grid_footprint_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/done/E104-kernel-w7-grid-footprint-primitive.md`
- Core API added:
  - `GridFootprint`, `GridCellCoord`, `GridFootprintStatus`
  - `GridFootprintResult`, `GridCellCoordResult`
  - `gridFootprintForContainingBounds(...)`
  - `gridFootprintForAlignedBounds(...)`
  - `gridCellForPoint(...)`
  - stable `toString(GridFootprintStatus)`
- RoomBake routing changed:
  - Reachability walkable surface projection now uses
    `gridFootprintForContainingBounds(...)`.
  - Reachability seed anchor lookup now uses `gridCellForPoint(...)`.
  - Greedy floor grouping now uses `gridFootprintForAlignedBounds(...)`.
  - Removed RoomBake-local `cellCoordFor`, reachability footprint struct,
    greedy floor footprint struct, and aligned-cell rounding helper.
- Behavior preserved:
  - Existing RoomBake output counts remained green.
  - The committed near-boundary floor regression remains pinned:
    `{0.99995,0,0}..{1.99995,0.25,1}` bakes as one 1m floor mesh at cell
    `[1,2)`, not `[0,2)`.
  - Negative, exact-boundary, invalid-cell-size, non-finite, empty, out-of-range,
    and misaligned cases are covered by core tests.
- Tests/checks run:
  - `cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d grid_footprint_tests creative_document_room_bake_tests render_room_mesh_geometry_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(grid_footprint_tests|creative_document_room_bake_tests|render_room_mesh_geometry_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Concerns/deferred:
  - The core helper intentionally does not own max-grid-cell policy. RoomBake
    still applies its local `GridTooLarge` caps after receiving valid cell
    coordinates.
