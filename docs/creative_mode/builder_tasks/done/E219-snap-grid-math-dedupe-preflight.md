# E219 - Snap/Grid Math Dedupe Preflight

## Status

Done.

## Context

`docs/complexity_audit_v0_1.md` item #7 flags duplicate snap/grid math with
diverged guards:

- `creative/spatial/Snap::snapScalar(...)`
- `creative/document/DocumentSnap::snapCreativeDocumentScalar(...)`
- `creative/spatial/SpatialProjection.cpp` grid coordinate helpers
- core `Snap` and `GridFootprint` helpers

Do not start from stale roadmap assumptions. The audit's object-kind switch item
is already stale in the current tree: `CreativeObjectKind::toString(...)`
delegates to `describeObject(kind).name`, and `allowedMutations(...)` already
builds from descriptor/profile helpers. This card is specifically for the
snap/grid math item.

## Objective

Read-only preflight the snap/grid duplicate-math seam and decide the safest next
implementation slice.

Produce a concrete follow-up recommendation, but do not edit source, tests,
CMake, fixtures, receipt golden, or production docs.

## Files To Inspect

Production:

- `src/core/math/Snap.hpp`
- `src/core/math/Snap.cpp`
- `src/core/grid/GridFootprint.hpp`
- `src/core/grid/GridFootprint.cpp`
- `src/app/iggy3d/creative/spatial/Snap.hpp`
- `src/app/iggy3d/creative/spatial/Snap.cpp`
- `src/app/iggy3d/creative/document/DocumentSnap.hpp`
- `src/app/iggy3d/creative/document/DocumentSnap.cpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`

Tests:

- `tests/unit/snap_kernel_tests.cpp`
- `tests/unit/creative_snap_tests.cpp`
- `tests/unit/creative_document_snap_tests.cpp`
- `tests/unit/grid_footprint_tests.cpp`
- any focused spatial projection tests found by grep

Reference docs:

- `docs/complexity_audit_v0_1.md` item #7 only
- `docs/creative_mode/builder_tasks/done/E104-kernel-w7-grid-footprint-primitive.md`

## Questions To Answer

1. Which current functions implement the same rounding formula, and which are
   intentionally different?
2. Which functions have non-finite, invalid-step, overflow, or int32 range
   guards?
3. Would delegating creative `double` snap wrappers to core `float` snap helpers
   change precision or receipt-visible values?
4. Is the right implementation a core `double` scalar snap helper, a templated
   core helper, or a narrowly shared private helper?
5. Should `SpatialProjection.cpp` delegate to an extended `GridFootprint` 3D
   helper, or should the next slice only add guard parity without a new core API?
6. What tests already pin current behavior, and what exact new guard tests are
   required before changing implementation?

## Required Classification Table

In the completion brief, include a table with one row per function:

- file/function
- value type (`float`, `double`, or int32 grid)
- rounding mode (`round`, `floor`, `ceil`, containing-cell, aligned-boundary)
- invalid step behavior
- non-finite input behavior
- overflow/out-of-range behavior
- current tests
- recommended action (`delegate`, `add guard`, `leave local`, `needs owner`)

## Candidate Follow-Up Shapes

The completion brief should recommend one of these, or explain why none is safe:

1. **Scalar guard parity only**: make the two creative scalar wrappers use the
   same invalid-step/non-finite/result-finite semantics while preserving their
   `double` API and current receipt messages.
2. **Core double snap primitive**: add a shared core `double` scalar snap helper
   and delegate creative 2D/3D wrappers to it.
3. **Grid projection guard slice**: extend core grid helpers or add a small
   guarded 3D grid conversion API, then route `SpatialProjection.cpp` through
   it.
4. **Split into two implementation cards**: scalar snap first, grid projection
   second.

## Non-Scope

Do not edit:

- source code
- tests
- CMake
- receipt golden
- fixtures
- production docs

Do not change:

- snap behavior
- receipt messages/statuses
- save/load format
- creative spatial projection behavior
- core grid API
- renderer/window code

No staging, commit, push, or window launch.

## Required Commands

Run and report:

```sh
rg -n "snapScalar|snapCreativeDocumentScalar|snapScalarToGrid|snapVec3ToGrid|snapToCellCenter|worldToGridCoord|worldBoundsToGridBounds|gridFootprintFor|gridCellForPoint|validStep|std::round|std::floor|std::ceil|static_cast<std::int32_t>" /Users/kogaryu/iggy3d/src/core /Users/kogaryu/iggy3d/src/app/iggy3d/creative /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "snapCreativeDocument|snapScalar\\(|snapPoint\\(|snapVec3ToGrid|snapScalarToGrid|GridFootprint|gridFootprint|gridCellForPoint|worldToGridCoord|worldBoundsToGridBounds" /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp'
git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required unless source/test files are unexpectedly edited.

## Self-Blockers

Stop and report instead of drafting an implementation card if:

- current behavior is already fully delegated to one owner
- a precision change would be required to share helpers
- the grid projection change needs a cross-lane owner decision
- tests reveal receipt-visible or save-visible snap output that the audit did
  not mention

## Completion Brief

When done, report:

- files inspected
- duplicate/intentional-local classification table
- exact current guard divergence
- test coverage gaps
- recommended next card title and scope, or "no implementation card"
- commands run
- confirmation that no source/test/CMake/receipt files were edited

## Completed

- Files inspected:
  - `src/core/math/Snap.hpp`
  - `src/core/math/Snap.cpp`
  - `src/core/grid/GridFootprint.hpp`
  - `src/core/grid/GridFootprint.cpp`
  - `src/app/iggy3d/creative/spatial/Snap.hpp`
  - `src/app/iggy3d/creative/spatial/Snap.cpp`
  - `src/app/iggy3d/creative/document/DocumentSnap.hpp`
  - `src/app/iggy3d/creative/document/DocumentSnap.cpp`
  - `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`
  - `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
  - `tests/unit/snap_kernel_tests.cpp`
  - `tests/unit/creative_snap_tests.cpp`
  - `tests/unit/creative_document_snap_tests.cpp`
  - `tests/unit/grid_footprint_tests.cpp`
  - `tests/unit/creative_spatial_projection_tests.cpp`
  - `docs/complexity_audit_v0_1.md`
  - `docs/creative_mode/builder_tasks/done/E104-kernel-w7-grid-footprint-primitive.md`
- Duplicate/intentional-local classification table:

| File/function | Value type | Rounding mode | Invalid step behavior | Non-finite input behavior | Overflow/out-of-range behavior | Current tests | Recommended action |
|---|---:|---|---|---|---|---|---|
| `core/math/Snap.cpp::snapScalarToGrid` | `float` | nearest grid point: `origin + round((value - origin) / step) * step` | pass through on non-positive or non-finite step | pass through on non-finite value/origin/step | pass through if computed result is non-finite | `snap_kernel_tests`: rounding, origin, bad step, non-finite value, overflow | leave current API; add `double` sibling or shared checked scalar before creative delegation |
| `core/math/Snap.cpp::snapVec3ToGrid` | `float` vec3 | per-axis nearest grid point via `snapScalarToGrid` | per-axis scalar pass-through | per-axis scalar pass-through | per-axis scalar pass-through | `snap_kernel_tests`: all axes, axis mask, per-axis step/origin, delta origin | leave; use as model, not direct target for creative `double` APIs |
| `core/math/Snap.cpp::snapToCellCenter` | `float` | containing cell center: `floor((value - origin) / cellSize)` | pass through on non-positive or non-finite cell size | pass through on non-finite value/origin/cell size | pass through if computed center is non-finite | `snap_kernel_tests`: containing-cell boundaries, origin, bad cell | leave local to place/cell-center semantics; intentionally not same as round snap |
| `core/math/Snap.cpp::snapVec3ToCellCenter` | `float` vec3 | per-axis containing-cell center | per-axis scalar pass-through | per-axis scalar pass-through | per-axis scalar pass-through | `snap_kernel_tests`: held Y axis | leave local to cell-center semantics |
| `creative/spatial/Snap.cpp::isValidSnapSettings` | `double` settings | no rounding | rejects only `stepX <= 0` or `stepY <= 0` in Grid mode | `NaN` step rejects by comparison; `infinity` step is currently accepted; origins/points are not checked | no result guard | `creative_snap_tests`: zero step only | add guard parity test; likely reject non-finite active steps before scalar routing |
| `creative/spatial/Snap.cpp::snapScalar` | `double` | same nearest-grid formula as core scalar | pass through only when `step <= 0`; `NaN` step is not caught in direct scalar call | value/origin/non-finite step can produce non-finite result | no result-finite guard; double overflow can emit non-finite | `creative_snap_tests`: formula, origin, negatives | delegate to new checked `double` core scalar after tests pin non-finite/value/origin/result behavior |
| `creative/spatial/Snap.cpp::snapPoint` | `double` 2D | per-axis `snapScalar` when axis enabled | setting-level zero/negative rejection; disabled mode bypasses validation | `infinity` step accepted today; non-finite point/origin not guarded | no result guard | `creative_snap_tests`: default, disabled, invalid zero step, axes, already snapped | add receipt guard tests before changing non-finite step policy; then delegate through checked scalar |
| `creative/document/DocumentSnap.cpp::validStep` | `double` | no rounding | finite and positive only | rejects non-finite step | no result guard | `creative_document_snap_tests`: disabled invalid steps, inactive invalid axes, active infinity step invalid | keep policy; share with scalar helper or call new core `validSnapStep` only if introduced |
| `creative/document/DocumentSnap.cpp::isValidCreativeDocumentSnapSettings` | `double` settings | no rounding | rejects active-axis non-finite/non-positive steps; ignores inactive-axis invalid steps; rejects unknown axes | does not validate origins or point values | no result guard | `creative_document_snap_tests`: active vs inactive invalid steps, none axes, unknown axes indirectly absent | leave local policy; only route scalar math |
| `creative/document/DocumentSnap.cpp::snapCreativeDocumentScalar` | `double` | same nearest-grid formula as core scalar | pass through on non-finite/non-positive step | value/origin are not checked; can emit non-finite | no result-finite guard; double overflow can emit non-finite | `creative_document_snap_tests`: formula/origin/negative through point/bounds | delegate to new checked `double` core scalar after direct scalar guard tests |
| `creative/document/DocumentSnap.cpp::snapPointValues` | `double` 3D | per-axis `snapCreativeDocumentScalar` | setting-level active-axis guard | no point/origin finite guard | no result guard | `creative_document_snap_tests`: axes, bounds normalization | delegate indirectly once scalar is shared |
| `core/grid/GridFootprint.cpp::gridFootprintForContainingBounds` | `float` XZ grid | half-open containing bounds: `floor(min/cell)`, `ceil(max/cell)` | status `InvalidCellSize` for non-finite or non-positive cell | status `NonFiniteInput` for non-finite bounds | `checkedInt32` gives `OutOfRange`; empty footprint detected | `grid_footprint_tests`: containing, negative/exact boundary, invalid, non-finite, out-of-range, empty | leave; existing 2D XZ kernel is good owner for RoomBake-style footprints |
| `core/grid/GridFootprint.cpp::gridFootprintForAlignedBounds` | `float` XZ grid | aligned boundary: `round(value/cell)` within tolerance | status `InvalidCellSize` | status `NonFiniteInput` for bounds or tolerance | `checkedInt32`; misaligned status when not near grid line | `grid_footprint_tests`: near-boundary, misaligned, statuses | leave; intentionally different from scalar snap and containing projection |
| `core/grid/GridFootprint.cpp::gridCellForPoint` | `float` XZ grid | containing point cell: `floor(point/cell)` | status `InvalidCellSize` | status `NonFiniteInput` for point | `checkedInt32` gives `OutOfRange` | `grid_footprint_tests`: zero/negative point cells, statuses | candidate model for future 3D projection helper, but not enough by itself |
| `creative/spatial/SpatialProjection.cpp::isValidRequest` | `double` grid request | no rounding | rejects only `cellSize <= 0` | `NaN` cell rejects by comparison; `infinity` cell is currently accepted; object bounds mostly unchecked | no int32 range guard | `creative_spatial_projection_tests`: normal invalid/out-of-bounds projection, not non-finite cell | add guard tests before any routing |
| `creative/spatial/SpatialProjection.cpp::worldToGridCoord` | `double` 3D grid | containing point cell: `floor(position/cellSize)` on X/Y/Z | returns `{}` only for `cellSize <= 0` | `NaN` cell or non-finite position can reach `floor` and int cast; `infinity` cell maps finite positions to zero | direct `static_cast<int32_t>` after floor has no checked range behavior | indirectly covered by projection tests for finite values; no direct invalid/range tests | needs guard slice or new 3D checked helper; do not delegate to 2D `GridFootprint` as-is |
| `creative/spatial/SpatialProjection.cpp::worldBoundsToGridBounds` | `double` 3D grid | containing half-open bounds: `floor(min/cell)`, `ceil(max/cell)` | returns `{}` only for `cellSize <= 0` | non-finite bounds/cell can reach floor/ceil and int casts | no checked int32 range behavior | indirectly covered by finite box/volume projection tests | needs guard slice or new 3D checked helper |
| `creative/spatial/SpatialProjection.cpp::appendSampledLineCells` | `int32` grid line | line sampling: `round(delta * t)` per axis | not step-based | no non-finite because inputs are already grid coords | possible `int32` arithmetic overflow on extreme coords, but callers normally gate by grid size | `creative_spatial_projection_tests`: path/link stable order and dedupe | leave local; not scalar snap duplicate |
| `creative/spatial/SpatialProjection.cpp::toGridIndex` / `toGridCoord` | integer grid | row-major conversion | not step-based | not applicable | assumes valid coord/size; `toGridCoord` guards only non-positive width/height | `creative_spatial_projection_tests`: row-major round trip | leave for now; outside snap duplicate unless future projection guard exposes invalid index policy |

- Exact current guard divergence:
  - Core `snapScalarToGrid(float)` is the most defensive scalar path: finite
    `value`, `origin`, and `step`; positive step; finite result fallback.
  - Creative 2D `snapScalar(double)` duplicates the same round formula but only
    checks `step <= 0.0`. Direct calls with `NaN` step, non-finite value/origin,
    or overflowing intermediate results can emit non-finite values.
  - Creative 2D settings validation rejects zero/negative and `NaN` steps by
    comparison, but accepts positive infinity as a valid grid step.
  - Creative document `snapCreativeDocumentScalar(double)` has finite positive
    step validation, but does not guard non-finite value/origin or non-finite
    computed results.
  - Directly delegating the creative `double` APIs to current core `float`
    helpers would be a precision change. The safe shared primitive is a checked
    `double` scalar overload or a type-generic implementation that preserves
    the public `double` arithmetic path.
  - Core `GridFootprint` has checked status results for finite cell sizes,
    finite XZ bounds/points, empty footprints, and int32 out-of-range values.
  - Creative `SpatialProjection` uses public 3D `double` helpers with direct
    `floor`/`ceil` to `int32_t` casts. They reject only `cellSize <= 0.0`;
    non-finite coordinates, `NaN` cell sizes, and int32 range overflow are not
    status-modeled there.
- Test coverage gaps:
  - Add creative 2D tests for `infinity` and `NaN` active step behavior at both
    settings/receipt level and direct `snapScalar(...)`.
  - Add creative 2D and document direct scalar tests for non-finite value,
    non-finite origin, and overflow/result-finite fallback.
  - Add document snap tests confirming active-axis non-finite steps stay invalid
    and inactive-axis invalid steps still do not reject.
  - Add precision guard tests proving `double` inputs that would round
    differently through `float` remain `double`-preserving.
  - Add spatial projection tests for direct `worldToGridCoord(...)` and
    `worldBoundsToGridBounds(...)` finite boundary semantics before changing
    implementation.
  - Add spatial projection invalid guard tests for non-finite position/bounds,
    non-finite cell size, and out-of-int32-range coordinates before introducing
    any checked 3D projection helper.
- Recommended next implementation card:
  - **E220: Core Double Snap Scalar And Creative Guard Parity**
  - Scope:
    - Add a checked core `double` scalar snap primitive, preferably an overload
      such as `snapScalarToGrid(double value, double step, double origin)`.
    - Preserve the existing `float` API and tests.
    - Route `creative/spatial/Snap::snapScalar(...)` and
      `creative/document/DocumentSnap::snapCreativeDocumentScalar(...)` through
      the checked `double` primitive.
    - Add the guard and precision tests listed above in
      `creative_snap_tests`, `creative_document_snap_tests`, and
      `snap_kernel_tests`.
    - Keep receipt/status strings stable for valid and invalid cases; if
      `CreativeSnapSettings` infinity-step handling is intentionally tightened,
      pin the new `invalid_snap_settings` behavior explicitly.
    - Do not touch `SpatialProjection` or `GridFootprint` in this first slice.
  - Follow-on card after E220:
    - **Spatial Projection 3D Grid Guard Slice**
    - Add tests for finite 3D projection boundaries and invalid/range behavior,
      then either add a small core 3D checked grid-coordinate helper or extend
      `GridFootprint` with an explicit 3D result type. Existing 2D XZ
      `GridFootprint` should not be forced onto `SpatialProjection` without a
      3D owner decision and status policy.
- Commands run:
  - `rg -n "snapScalar|snapCreativeDocumentScalar|snapScalarToGrid|snapVec3ToGrid|snapToCellCenter|worldToGridCoord|worldBoundsToGridBounds|gridFootprintFor|gridCellForPoint|validStep|std::round|std::floor|std::ceil|static_cast<std::int32_t>|static_cast&lt;std::int32_t&gt;" /Users/kogaryu/iggy3d/src/core /Users/kogaryu/iggy3d/src/app/iggy3d/creative /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'`
  - `rg -n "snapCreativeDocument|snapScalar\\(|snapPoint\\(|snapVec3ToGrid|snapScalarToGrid|GridFootprint|gridFootprint|gridCellForPoint|worldToGridCoord|worldBoundsToGridBounds" /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp'`
  - `rg -n "SpatialProjection|worldToGridCoord|worldBoundsToGridBounds|projectObjectToGrid|projectObjectsToGrid|CreativeSpatial" tests/unit --glob '*.cpp'`
  - `rg -n "complexity.*#7|snap|grid|GridFootprint|duplicate" docs/complexity_audit_v0_1.md docs/creative_mode/builder_tasks/done/E104-kernel-w7-grid-footprint-primitive.md`
  - `git -C /Users/kogaryu/iggy3d diff --check`
- Confirmation:
  - No source, test, CMake, fixture, receipt golden, staging, commit, push, or
    window launch changes were performed. Only this task card was moved and
    appended.
