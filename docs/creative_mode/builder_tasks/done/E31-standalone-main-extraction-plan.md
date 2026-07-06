# E31: Standalone Creative Main Extraction Plan

## Objective

Prepare the standalone creative app for more work by extracting the highest-risk
logic out of `apps/iggy3d_creative/main.cpp`.

## Problem

`apps/iggy3d_creative/main.cpp` is now over 3,700 lines and owns brush palette
policy, visual proxies, undo, path handles, RoomBake preview consumption,
capture scripting, UI projection, save/load proof, and interaction glue. It was
useful as a lab, but it should not keep absorbing features.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `CMakeLists.txt`

## Scope

- Make the smallest behavior-preserving extraction.
- Recommended first target: standalone visual proxy helpers or standalone undo
  helpers, because those have clear boundaries and can keep capture output
  identical.
- Keep capture PNG/hash/log proof as the gate.

## Acceptance

- `main.cpp` loses a coherent helper section without behavior changes.
- Baseline and final capture hashes match unless the extraction intentionally
  changes only logging.
- No product app behavior changes.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_extract_check.png --frames 32 > /tmp/iggy3d_creative_extract_check.log 2>&1`
- Inspect the capture PNG and log for `package_room_meshes_presented`.
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change brush eligibility, RoomBake policy, path-point behavior, or
  capture script semantics in the extraction.
- Do not use this task as a feature slice.

## Completion Brief

- Files changed:
  - `apps/iggy3d_creative/main.cpp`
  - `apps/iggy3d_creative/StandaloneUndo.hpp`
- Behavior changed:
  - Extracted the standalone app-local undo stack core into
    `StandaloneUndo.hpp`.
  - Kept app-specific create/delete/move/path undo wrappers in `main.cpp`.
  - No brush eligibility, RoomBake policy, path-point behavior, capture
    scripting, product app code, or CMake behavior changed.
  - `main.cpp` line count dropped from 3723 to 3643 lines.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e31_baseline.png --frames 32 > /tmp/iggy3d_creative_e31_baseline.log 2>&1`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e31_final.png --frames 32 > /tmp/iggy3d_creative_e31_final.log 2>&1`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneUndo.hpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E31-standalone-main-extraction-plan.md`
- Evidence:
  - Baseline PNG hash:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Final PNG hash:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Final log kept `package_room_meshes_presented`.
  - Final log kept `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
  - Final log kept `ROOM_BAKE final status='baked' ... staticMeshes=6 spatialSurfaces=11 ... standalonePreviewMeshes=3 sceneMeshes=1690`.
  - Visual inspection matched baseline: same grid/camera, Floor/Crates/Wall,
    Point marker, Beam, PatrolRoute, green ghost, and UI.
- Concerns/deferred:
  - This uses a header-only extraction to avoid a CMake edit. A later larger
    extraction can move standalone helpers into compiled `.cpp` files if the
    app rules are updated to allow CMake changes.
