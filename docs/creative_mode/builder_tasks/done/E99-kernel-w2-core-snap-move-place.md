# E99: Kernel W2 - Core Snap For Move Finalization And Standalone Place

## Objective

Wire the shipped `core/math/Snap` kernel into Move finalization and standalone
Place cell-center math without touching the existing creative UI snap policy
layers.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W2.
- Standing rule: do not touch `creative/spatial/Snap` or
  `creative/document/DocumentSnap`.

## Scope

- `src/app/iggy3d/creative/Facade.cpp`
- `apps/iggy3d_creative/StandalonePlacement.*`
- focused tests only.

## Required Move Change

- Keep the current pre-snap held-axis anchoring.
- Replace Move's final position snap with `iggy3d::snapVec3ToGrid(...)`.
- Build the axis mask from the held axis:
  - held X -> mask `0x6`
  - held Y -> mask `0x5`
  - held Z -> mask `0x3`
- Convert document snap step/origin from `double` creative types to core
  `float` types at the boundary, then convert the result back.
- Delete the second post-snap `holdMoveAxis` pass; the core snap axis mask owns
  that responsibility.

## Required Place Change

- Replace `snapGroundToCellCenter(...)` hand math with `snapVec3ToGrid(...)`.
- Use X/Z snap only and leave Y at 0:
  - value = ground world position
  - step = `{cellSize, cellSize, cellSize}` or equivalent
  - origin = `{cellSize * 0.5, cellSize * 0.5, cellSize * 0.5}` or equivalent
  - axis mask = X|Z (`0x5`)

## Do Not

- Do not alter UI pointer snap policy.
- Do not touch `creative/spatial/Snap` or `creative/document/DocumentSnap`.
- Do not alter RoomBake, descriptors, save/load, renderer/Vulkan, or product
  launch behavior.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `src/core/math/Snap.hpp`
- `src/app/iggy3d/creative/Facade.cpp`
- `apps/iggy3d_creative/StandalonePlacement.hpp`
- `apps/iggy3d_creative/StandalonePlacement.cpp`
- nearby creative tool/move tests.

## Acceptance

- Held-axis Move preserves the held axis exactly while snapping the other axes.
- Existing creative tool tests stay green.
- Standalone capture still places objects at the same cell centers as before.
- No observable capture regression unless the old hand math had a proven bug.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative creative_tools_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_tools_tests$' --output-on-failure
/Users/kogaryu/iggy3d/build/iggy3d_creative --capture /tmp/iggy3d_kernel_w2_final.png --frames 32 > /tmp/iggy3d_kernel_w2_final.log 2>&1
git -C /Users/kogaryu/iggy3d diff --check
```

If `creative_tools_tests` is not the exact focused target in this checkout, use
the closest existing Move/facade tool target and report it.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Held-axis snap proof:
- Place snap proof:
- Capture artifact:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `src/app/iggy3d/creative/Facade.cpp`
  - `apps/iggy3d_creative/StandalonePlacement.cpp`
  - `tests/unit/creative_facade_mutation_tests.cpp`
  - `tests/unit/standalone_placement_tests.cpp`
  - `cmake/iggy3d_tests.cmake`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Behavior changed:
  - Move preview/commit still resolves the held axis into the requested anchor
    first.
  - Move final snap now uses `iggy3d::snapVec3ToGrid(...)` with a core axis
    mask that excludes the held axis: held X -> `0x6`, held Y -> `0x5`,
    held Z -> `0x3`.
  - The old post-snap `holdMoveAxis(...)` pass was removed because the core
    mask now owns that responsibility.
  - Standalone `snapGroundToCellCenter(...)` now uses `snapVec3ToGrid(...)`
    with X/Z mask `0x5`, cell-size steps, half-cell origins, and Y forced
    back to ground `0`.
- Held-axis snap proof:
  - Added `dragCommitHeldYAxisSnapsOnlyXZWithCoreMask()` in
    `creative_facade_mutation_tests`.
  - The test moves a Room from start anchor `(0,5,0)` toward
    `(10.4,100,10.6)` with held Y.
  - Preview and commit both produce snapped anchor `(10,5,11)`, proving Y is
    preserved exactly while X/Z snap through the core grid.
- Place snap proof:
  - Added `standalone_placement_tests`.
  - Tests compare `snapGroundToCellCenter(...)` to direct
    `snapVec3ToGrid(...)` for 1m and 2m cells, including negative Z, and pin
    Y at `0`.
- Capture artifact:
  - No capture run for this slice. The user explicitly cut out the test script
    path that opens the standalone app and places objects. The standalone place
    proof is covered by `standalone_placement_tests`.
- Tests/checks run:
  - `cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative creative_facade_mutation_tests creative_tools_tests standalone_placement_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_facade_mutation_tests|creative_tools_tests|standalone_placement_tests)$' --output-on-failure`
- Concerns/deferred:
  - `creative/spatial/Snap` and `creative/document/DocumentSnap` were not
    modified.
  - Standalone capture hash was not refreshed because the script path was
    intentionally skipped.
