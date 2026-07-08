# E219 - Snap/Grid Math Dedupe Preflight

## Status

Ready.

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
