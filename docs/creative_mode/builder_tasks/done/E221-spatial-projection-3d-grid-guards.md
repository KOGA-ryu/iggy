# E221 - Spatial Projection 3D Grid Guards

## Status

Done.

## Context

E219 preflighted duplicate snap/grid math and split the work:

1. E220 added checked core `double` scalar snap and routed creative scalar snap
   wrappers through it.
2. This card handles the follow-on spatial projection 3D grid guard slice.

Current `SpatialProjection.cpp` state:

- `isValidRequest(...)` accepts any `cellSize > 0.0`; positive infinity is
  currently accepted.
- `worldToGridCoord(...)` and `worldBoundsToGridBounds(...)` reject only
  `cellSize <= 0.0`.
- Both helpers directly cast `std::floor(...)` / `std::ceil(...)` results to
  `std::int32_t`, so non-finite coordinates/cell sizes and out-of-int32-range
  values are not guarded.
- `GridFootprint` is a checked 2D XZ owner and is intentionally not a direct
  owner for this public 3D creative projection API.

## Objective

Add explicit finite/range guards for creative 3D grid coordinate conversion and
projection request validation while preserving existing finite projection
behavior and public APIs.

## Implementation Scope

Edit:

- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- `tests/unit/creative_spatial_projection_tests.cpp`
- this task card

Likely no CMake change is needed.

## Required Behavior

For direct public helpers:

- `worldToGridCoord(position, cellSize)` preserves existing finite containing
  cell semantics: per-axis `floor(position.axis / cellSize)`.
- `worldBoundsToGridBounds(bounds, cellSize)` preserves existing finite
  half-open containing bounds semantics: min uses `floor`, max uses `ceil`.
- Return default `{}` when:
  - `cellSize` is non-finite
  - `cellSize <= 0.0`
  - any relevant position/bounds coordinate is non-finite
  - any computed cell coordinate is non-finite
  - any computed cell coordinate is outside `std::int32_t` range

For projection requests:

- `isValidRequest(...)` must require finite positive `cellSize`.
- Projection with non-finite positive cell size should reject as
  `CreativeSpatialProjectionStatus::InvalidGrid` with message
  `"invalid_grid"`.

For object projection:

- Preserve existing behavior for finite in-grid, off-grid, clamped,
  unclamped, path, line, link, aggregate, hidden, and authoring-only cases.
- Do not silently turn non-finite or out-of-range point/bounds coordinates into
  an origin cell.
- Use an existing projection status/message instead of adding a new enum. The
  preferred narrow policy is:
  - point/box/volume/line conversion guard failure returns `OutOfBounds` with
    message `"out_of_bounds"` and no cells
  - path/link endpoint non-finite validation remains through the existing
    `invalid_path_points` / `invalid_line_endpoints` paths
- If that policy conflicts with existing tests or forces broad receipt changes,
  stop and report the conflict instead of inventing new status strings.

## Implementation Guidance

Prefer file-local helpers in `SpatialProjection.cpp`, for example:

- `validCellSize(double) noexcept`
- `checkedInt32(double, std::int32_t&) noexcept`
- `tryWorldToGridCoord(CreativeVec3, double, CreativeGridCoord3&) noexcept`
- `tryWorldBoundsToGridBounds(CreativeBounds, double, CreativeGridBounds3&) noexcept`

The public helpers can wrap these and return `{}` on failure. Projection
callers should use the status-returning private helpers so failure is not
confused with the valid origin cell.

Keep `appendSampledLineCells(...)` local. Its `round(delta * t)` integer line
sampling is not scalar snap math and is out of scope.

## Tests To Add/Update

In `tests/unit/creative_spatial_projection_tests.cpp`, add direct helper tests
for:

- finite `worldToGridCoord(...)` boundary behavior, including:
  - positive values
  - negative values
  - fractional values just below/above a cell boundary
- finite `worldBoundsToGridBounds(...)` half-open boundary behavior, including:
  - exact max boundary
  - fractional max boundary
  - negative min boundary
- invalid `worldToGridCoord(...)` fallback to `{}` for:
  - zero, negative, NaN, and infinity cell sizes
  - NaN/infinity coordinates
  - out-of-int32-range coordinates
- invalid `worldBoundsToGridBounds(...)` fallback to `{}` for:
  - NaN/infinity cell sizes
  - non-finite min/max coordinates
  - out-of-int32-range min/max coordinates

Add projection-level guards for:

- point projection with NaN/infinity position does not project an origin cell
  and reports `OutOfBounds` / `"out_of_bounds"` unless a stronger existing
  status is already used.
- volume or box projection with non-finite/out-of-range bounds does not project
  an origin/intersecting cell and reports `OutOfBounds` / `"out_of_bounds"` or
  `EmptyProjection` only if the existing clamped finite path already implies
  empty.
- request with `cellSize = infinity` reports `InvalidGrid` /
  `"invalid_grid"` for single-object and aggregate projection.

Preserve all existing tests unchanged unless a test directly depends on the
unguarded non-finite/int32-cast behavior.

## Non-Scope

Do not change:

- `src/core/grid/GridFootprint.*`
- `src/core/math/Snap.*`
- creative scalar snap behavior from E220
- `SpatialProjection.hpp` public API shape unless a tiny private-source-only
  helper is impossible
- `appendSampledLineCells(...)` line sampling policy
- `toGridIndex(...)` / `toGridCoord(...)` integer conversion policy
- receipt keys/order/value golden files
- save/load format
- renderer/window code
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

Run and report:

```sh
rg -n "worldToGridCoord|worldBoundsToGridBounds|isValidRequest|validCellSize|checkedInt32|tryWorld|std::floor|std::ceil|static_cast<std::int32_t>" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/spatial/SpatialProjection.cpp /Users/kogaryu/iggy3d/tests/unit/creative_spatial_projection_tests.cpp
rg -n "GridFootprint|gridFootprintFor|gridCellForPoint" /Users/kogaryu/iggy3d/src/core/grid /Users/kogaryu/iggy3d/src/app/iggy3d/creative/spatial --glob '*.cpp' --glob '*.hpp'
```

Expected classification:

- `SpatialProjection.cpp` owns this 3D guard slice.
- `GridFootprint.*` is unchanged and remains the checked 2D XZ footprint owner.
- Existing `SpatialProjection` finite projection tests still pass.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_spatial_projection_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_spatial_projection_tests|product_receipt_key_order_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- direct helper guards require a public API result/status type change
- non-finite projection policy cannot be expressed with existing statuses
  without changing broad receipt expectations
- guarding int32 range forces changes in `toGridIndex(...)` / `toGridCoord(...)`
- implementation starts moving logic into `GridFootprint` or `Snap`
- receipt golden output changes

## Completion Brief

When done, report:

- files changed
- exact helper/guard shape added
- direct public helper invalid fallback behavior
- projection-level non-finite/range behavior
- tests added
- required grep classification
- focused build/CTest result
- receipt golden result
- diff/whitespace checks
- confirmation that `GridFootprint` and core `Snap` were not touched

## Completion Brief

- Files changed:
  - `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
  - `tests/unit/creative_spatial_projection_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E221-spatial-projection-3d-grid-guards.md`
- Exact helper/guard shape added:
  - Added file-local `validCellSize(double) noexcept`, requiring finite positive cell sizes.
  - Added file-local `checkedInt32(double, std::int32_t&) noexcept`, rejecting non-finite and outside-`std::int32_t` computed cell coordinates.
  - Added file-local `tryWorldToGridCoord(CreativeVec3, double, CreativeGridCoord3&) noexcept`, preserving per-axis `floor(position.axis / cellSize)` for finite in-range inputs.
  - Added file-local `tryWorldBoundsToGridBounds(CreativeBounds, double, CreativeGridBounds3&) noexcept`, preserving min `floor(...)` and max `ceil(...)` half-open bounds semantics for finite in-range inputs.
  - Added file-local `canExpandCell(...)`, `pointBoundsOrDefault(...)`, and `lineBoundsOrDefault(...)` so out-of-bounds receipts do not overflow half-open projected bounds when a finite converted coordinate is already at `std::int32_t::max()`.
  - `isValidRequest(...)` now uses `validCellSize(...)`, so positive infinity rejects as an invalid grid.
- Direct public helper invalid fallback behavior:
  - `worldToGridCoord(...)` now wraps `tryWorldToGridCoord(...)` and returns default `{}` for invalid/non-finite/non-positive cell size, non-finite coordinates, non-finite computed cells, or out-of-int32-range computed cells.
  - `worldBoundsToGridBounds(...)` now wraps `tryWorldBoundsToGridBounds(...)` and returns default `{}` for the same invalid cell-size/range/finite checks over min and max bounds.
- Projection-level non-finite/range behavior:
  - Point, box/volume, line, path, and link projection code uses the private `tryWorld*` helpers so guard failure is not confused with a valid origin cell.
  - Point/box/volume/line/range conversion guard failures return `CreativeSpatialProjectionStatus::OutOfBounds`, message `"out_of_bounds"`, and no cells.
  - Existing path/link non-finite validation remains on the existing `invalid_path_points` / `invalid_line_endpoints` paths before grid conversion.
  - Single-object and aggregate requests with `cellSize = infinity` reject as `CreativeSpatialProjectionStatus::InvalidGrid`, message `"invalid_grid"`.
- Tests added:
  - `worldToGridCoordUsesContainingCellBoundaries()`
  - `worldBoundsToGridBoundsUsesHalfOpenBoundaries()`
  - `invalidWorldToGridCoordInputsReturnDefault()`
  - `invalidWorldBoundsToGridBoundsInputsReturnDefault()`
  - `nonFinitePointDoesNotProjectOriginCell()`
  - `maxIntPointDoesNotOverflowProjectionBounds()`
  - `invalidVolumeBoundsDoNotProjectOriginCell()`
  - `infiniteCellSizeRejectsProjectionRequests()`
- Required grep classification:
  - `SpatialProjection.cpp` owns this 3D guard slice through `validCellSize`, `checkedInt32`, `tryWorldToGridCoord`, and `tryWorldBoundsToGridBounds`.
  - Existing finite public helper and projection tests still pass.
  - Remaining raw casts in `appendSampledLineCells(...)` are the existing integer line sampling policy and were intentionally left local/out of scope.
  - Existing `toGridCoord(...)` integer index conversion casts were not changed.
  - `GridFootprint.*` grep hits are only in `src/core/grid/GridFootprint.*`; creative spatial projection does not route through it in this slice.
- Focused build/CTest result:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_spatial_projection_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_spatial_projection_tests|product_receipt_key_order_tests)$' --output-on-failure` passed, 2/2.
- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed: `1032 fields match golden (order + values)`.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched source/test/card files returned no hits.
- Confirmation:
  - `src/core/grid/GridFootprint.*` and `src/core/math/Snap.*` were not touched.
  - `SpatialProjection.hpp`, `appendSampledLineCells(...)`, `toGridIndex(...)`, `toGridCoord(...)`, receipt golden files, CMake, save/load, renderer/window code, staging, commit, push, and window launch were not changed.
