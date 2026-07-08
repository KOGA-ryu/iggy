# E220 - Core Double Snap Scalar And Creative Guard Parity

## Status

Ready.

## Context

E219 preflighted the snap/grid duplicate-math seam and recommended splitting
work into two implementation slices:

1. scalar snap guard/owner parity first
2. spatial projection 3D grid guard work later

This is the scalar snap slice only.

Current state from E219:

- `core/math/Snap.cpp::snapScalarToGrid(float, float, float)` is the checked
  nearest-grid scalar owner for `float`:
  - requires finite `value`, `origin`, and `step`
  - requires positive `step`
  - falls back to the original `value` if the computed result is non-finite
- `creative/spatial/Snap.cpp::snapScalar(double, double, double)` duplicates
  the same round formula but only checks `step <= 0.0`.
- `creative/document/DocumentSnap.cpp::snapCreativeDocumentScalar(...)` checks
  finite positive `step`, but not finite `value`, finite `origin`, or finite
  result.
- Directly delegating the creative `double` APIs to the current core `float`
  helper would be a precision change. Add a checked `double` primitive instead.

## Objective

Add a checked core `double` nearest-grid scalar helper and route creative 2D/3D
scalar snap wrappers through it while preserving public APIs and receipt/status
strings.

## Implementation Scope

Edit:

- `src/core/math/Snap.hpp`
- `src/core/math/Snap.cpp`
- `src/app/iggy3d/creative/spatial/Snap.cpp`
- `src/app/iggy3d/creative/document/DocumentSnap.cpp`
- `tests/unit/snap_kernel_tests.cpp`
- `tests/unit/creative_snap_tests.cpp`
- `tests/unit/creative_document_snap_tests.cpp`
- this task card

Likely no CMake change is needed.

## Required API Shape

Add an overload or equivalent checked core helper:

```cpp
[[nodiscard]] double snapScalarToGrid(double value,
                                      double step,
                                      double origin) noexcept;
```

Required semantics:

- same formula as the existing scalar snap:
  `origin + round((value - origin) / step) * step`
- pass through `value` unchanged when:
  - `value` is non-finite
  - `origin` is non-finite
  - `step` is non-finite
  - `step <= 0.0`
  - the computed result is non-finite
- preserve the existing `float snapScalarToGrid(float, float, float)` API and
  behavior.

Implementation preference:

- It is acceptable to add a small file-local templated implementation in
  `Snap.cpp` and have both overloads call it, as long as the public `float`
  behavior stays byte-for-byte equivalent for existing tests.
- Keep `#pragma STDC FP_CONTRACT OFF` discipline in `Snap.cpp`.

## Creative Routing

Route:

- `creative::snapScalar(double, double, double)`
- `creative::snapCreativeDocumentScalar(double, double, double)`

through the new checked `double` core helper.

For `creative/spatial/Snap.cpp::isValidSnapSettings(...)`:

- tighten active grid steps to finite positive values, matching document snap
  and core scalar semantics.
- Preserve disabled-mode behavior.
- Preserve `kCreativeSnapAxisNone` behavior.
- If a step is inactive because the axis mask excludes that axis, it should not
  invalidate settings.

For `creative/document/DocumentSnap.cpp`:

- preserve current active-axis finite positive step validation.
- preserve inactive-axis invalid-step tolerance.
- preserve unknown-axis rejection.
- only change scalar result behavior via the checked core double helper.

## Tests To Add/Update

In `tests/unit/snap_kernel_tests.cpp`:

- Add direct coverage for `double snapScalarToGrid(...)`.
- Pin finite normal rounding/origin behavior for the double overload.
- Pin pass-through on non-finite value, non-finite origin, non-finite step,
  non-positive step, and non-finite computed result.
- Add a precision guard proving the double overload does not round through
  `float`.

In `tests/unit/creative_snap_tests.cpp`:

- Add direct `creative::snapScalar(...)` tests for:
  - non-finite value pass-through
  - non-finite origin pass-through
  - non-finite step pass-through
  - overflow/non-finite computed result pass-through
  - double precision retained, not routed through float
- Add receipt/settings tests proving active infinite or NaN grid steps are
  rejected as invalid settings.
- Add a test proving inactive-axis invalid/non-finite steps do not reject when
  their axis is masked out.
- Preserve existing receipt messages for existing valid/invalid cases.

In `tests/unit/creative_document_snap_tests.cpp`:

- Add direct `snapCreativeDocumentScalar(...)` tests for non-finite value,
  non-finite origin, non-finite step, non-positive step, overflow/result-finite
  fallback, and double precision retention.
- Add or preserve tests proving active-axis invalid/non-finite steps reject and
  inactive-axis invalid/non-finite steps do not reject.
- Preserve existing document snap receipt statuses/reason codes for existing
  cases.

## Non-Scope

Do not change:

- `SpatialProjection.cpp`
- `GridFootprint.*`
- `snapToCellCenter(...)`
- `snapVec3ToGrid(...)` behavior beyond routing through preserved scalar logic
  if a shared implementation is introduced
- receipt key/order/value golden files
- creative spatial projection behavior
- save/load format
- renderer/window code
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

Run and report:

```sh
rg -n "snapScalarToGrid\\(|snapScalar\\(|snapCreativeDocumentScalar\\(|isValidSnapSettings|validStep" /Users/kogaryu/iggy3d/src/core/math/Snap.hpp /Users/kogaryu/iggy3d/src/core/math/Snap.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/spatial/Snap.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/document/DocumentSnap.cpp /Users/kogaryu/iggy3d/tests/unit/snap_kernel_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_snap_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_snap_tests.cpp
rg -n "worldToGridCoord|worldBoundsToGridBounds|gridFootprintFor|GridFootprint" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/spatial /Users/kogaryu/iggy3d/src/core/grid --glob '*.cpp' --glob '*.hpp'
```

Expected classification:

- core `Snap` owns both checked scalar overloads.
- creative scalar wrappers remain as public compatibility wrappers but delegate
  to core.
- `SpatialProjection` and `GridFootprint` are unchanged by this slice.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target snap_kernel_tests creative_snap_tests creative_document_snap_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(snap_kernel_tests|creative_snap_tests|creative_document_snap_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- adding the `double` helper changes existing `float` snap behavior
- creative snap receipt statuses/messages change for existing valid/invalid
  cases outside the explicitly tightened active non-finite step behavior
- double precision guard cannot be written without brittle decimal assumptions
- implementation starts pulling in `SpatialProjection` or `GridFootprint`
- receipt golden output changes

## Completion Brief

When done, report:

- files changed
- exact core helper/API added
- how creative 2D/document scalar snap now routes
- active/inactive non-finite step behavior
- precision and guard tests added
- required grep classification
- focused build/CTest result
- receipt golden result
- diff/whitespace checks
- confirmation that `SpatialProjection` and `GridFootprint` were not touched
