# E110: Kernel W9 - AabbGridIndex Validation And Boundary Policy

## Objective

Harden and document the core `AabbGridIndex` validation contract without
changing broadphase behavior.

This is a core-kernel primitive card. The goal is to make the shared spatial
index safer as more systems consume it, not to tune standalone picking or
runtime gameplay behavior.

## Current Seam

Files to inspect first:

- `src/core/spatial/AabbGridIndex.hpp`
- `src/core/spatial/AabbGridIndex.cpp`
- `tests/unit/aabb_grid_index_tests.cpp`

Observed current behavior:

- Constructor falls back to `8.0F` for `cellSizeMeters <= 0.0F`, but it does
  not explicitly reject `NaN` or `infinity`.
- `Aabb3::isValid(...)` allows zero-extent bounds because min <= max is valid.
- `AabbGridIndex::cellRange(...)` uses inclusive max-cell coverage, so an exact
  grid-line max boundary conservatively includes the next cell.

## Required Work

1. Treat non-finite cell sizes as invalid input in the constructor.
   - Keep public API unchanged.
   - Preserve the current invalid-cell-size fallback model by normalizing
     non-finite, zero, and negative sizes to the default `8.0F`.
   - Add tests for `NaN`, `infinity`, zero, and negative constructor input.
2. Pin zero-extent bounds policy.
   - If current behavior accepts zero-extent valid AABBs, keep it and add a
     focused test that insert/query remains deterministic.
   - Do not change `Aabb3::isValid(...)` in this card.
3. Pin exact-boundary cell policy.
   - Add a focused test showing a box ending exactly on a cell boundary is
     conservatively indexed/queryable from both adjacent cell regions.
   - The broadphase must remain a superset; do not optimize away conservative
     boundary hits in this card.
4. Improve comments in `AabbGridIndex.hpp` or near `cellRange(...)` so future
   users understand the zero-extent and exact-boundary policies.

## Do Not

- Do not change `Aabb3` validity semantics.
- Do not change `AabbGridIndex` public API.
- Do not alter standalone picking, RoomBake, physics, render, save/load,
  descriptors, or Creative systems.
- Do not introduce a cache or broadphase lifetime owner.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Behavior

- `AabbGridIndex(std::numeric_limits<float>::infinity()).cellSizeMeters()`
  returns `8.0F`.
- `AabbGridIndex(NaN).cellSizeMeters()` returns `8.0F`.
- Existing insertion/query/rebuild/status tests continue to pass.
- Zero-extent and exact-boundary behavior is explicitly covered and documented.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d aabb_grid_index_tests standalone_picking_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(aabb_grid_index_tests|standalone_picking_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Validation policy:
- Boundary/zero-extent policy:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/core/spatial/AabbGridIndex.hpp`
  - `src/core/spatial/AabbGridIndex.cpp`
  - `tests/unit/aabb_grid_index_tests.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E110-kernel-w9-aabb-grid-index-validation-policy.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Validation policy:
  - Constructor now treats non-finite, zero, and negative cell sizes as invalid
    constructor input and normalizes them to the existing default `8.0F`.
  - Public API remains unchanged.
  - Added coverage for `NaN`, `infinity`, zero, negative, and positive finite
    constructor input.
- Boundary/zero-extent policy:
  - Kept `Aabb3` validity semantics unchanged: zero-extent AABBs are valid.
  - Added coverage proving zero-extent bounds insert and query
    deterministically.
  - Kept conservative inclusive max-cell coverage. Added coverage proving a box
    ending exactly on a cell boundary remains queryable from the min cell and
    conservatively queryable from the adjacent max-side cell region.
  - Added comments in `AabbGridIndex.hpp` documenting zero-extent validity,
    constructor fallback, and exact-boundary conservative broadphase policy.
- Behavior preserved:
  - Existing insertion, query, rebuild, bounds-status, and checked-query tests
    remain green.
  - `standalone_picking_tests` remains green as the known `AabbGridIndex`
    consumer.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d aabb_grid_index_tests standalone_picking_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(aabb_grid_index_tests|standalone_picking_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - This intentionally does not optimize away exact-boundary conservative
    candidates. The broadphase remains a superset and exact intersection stays
    the caller's responsibility.
