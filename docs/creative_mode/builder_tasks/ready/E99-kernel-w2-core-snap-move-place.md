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
