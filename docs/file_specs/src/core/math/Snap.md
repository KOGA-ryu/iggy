# File Spec

Files: `src/core/math/Snap.hpp`, `src/core/math/Snap.cpp`

Verified at: `3b59f65c`

## Owns

- Core deterministic snap math.
- Scalar grid snapping for `float` and `double`.
- Per-axis `Vec3` grid snapping.
- Containing-cell-center snap for scalars and `Vec3`.
- AABB base-to-height alignment.

## Does Not Own

- Creative snap settings, UI receipts, ghost placement, or document storage.
- Occupancy/surface search for target heights.
- Save/hash policy beyond deterministic math behavior.
- Renderer, app window, or physics solver behavior.

## Reads

- Scalar values, steps, origins, axis masks, `Vec3` values, and `Aabb3` boxes.
- AABB validity from `Aabb3.*`.

## Writes / Mutates

- Returns snapped scalar/vector/AABB values.
- Does not mutate caller-owned objects.

## Calls Out To / Wires Out To

- Used by creative 2D snap and creative document 3D snap wrappers.
- Used by facade anchor/grid helpers and tests.

## Called By / Entry Points

- `snapScalarToGrid(...)`.
- `snapVec3ToGrid(...)`.
- `snapToCellCenter(...)`.
- `snapVec3ToCellCenter(...)`.
- `alignAabbBaseToHeight(...)`.
- Grep proof: `rg -n "snapScalarToGrid|snapVec3ToGrid|snapToCellCenter|alignAabbBaseToHeight|snap_kernel_tests" src/app src/core tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Invalid step/cell size or non-finite inputs pass through unchanged.
- Scalar grid snapping uses nearest grid point relative to origin.
- Cell-center snapping uses containing half-open cell, not nearest grid point.
- Axis masks gate per-axis vector changes.
- AABB base alignment preserves box size and rejects invalid boxes, non-finite targets, and invalid axes.
- FP contraction stays disabled in this translation unit to preserve deterministic scalar snapping across toolchains.

## Tests / Proof Commands

- `snap_kernel_tests`.
- `creative_snap_tests`.
- `creative_document_snap_tests`.
- `rg -n "snap_kernel_tests|creative_snap_tests|creative_document_snap_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/spatial/Snap.*` unless creative 2D wrapper behavior changes.
- `src/app/iggy3d/creative/document/DocumentSnap.*` unless document 3D snap behavior changes.
- `src/core/math/Aabb3.*` unless AABB validity semantics change.
- `src/core/math/Vec3.*` unless vector storage changes.

## Update When

- Core snap rounding, invalid-input guards, axis-mask semantics, cell-center policy, AABB base alignment, or determinism constraints change.

## Do Not Update When

- Only UI/facade/document wrappers change around unchanged core snap math.
