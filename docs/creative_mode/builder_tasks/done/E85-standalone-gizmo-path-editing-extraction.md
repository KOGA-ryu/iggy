# E85: Standalone Gizmo And Path Editing Extraction

## Objective

Extract standalone gizmo/move/path-editing helpers out of
`apps/iggy3d_creative/main.cpp` without changing interactive or capture
behavior.

## Dependency

Do this after E84 is complete. E84 moves capture scenario orchestration and may
change the exact call sites for move/path proof steps.

## Problem

The standalone app still owns transform-gizmo and path-editing implementation
inside `main.cpp`. The code is no longer core app bootstrap; it is an app-local
interaction subsystem:

- gizmo axis names and held-axis mapping,
- screen-space axis hit math,
- move dispatch logging,
- move release with undo,
- whole-path move mutation through `SetPatrolRoute`,
- path point mutation through `SetPatrolRoute`,
- path point handle support glue that is not purely picking/rendering.

This makes `main.cpp` harder to review and keeps path-editing behavior mixed
with the frame loop.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E78-standalone-section-map-dependency-audit.md`
- `docs/creative_mode/builder_tasks/done/E79-standalone-picking-extraction.md`
- `docs/creative_mode/builder_tasks/done/E84-standalone-capture-scenario-orchestration-extraction.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandalonePicking.hpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `CMakeLists.txt`

## Scope

Create one or both, depending on dependency shape:

- `apps/iggy3d_creative/StandaloneGizmo.hpp/.cpp`
- `apps/iggy3d_creative/StandalonePathEditing.hpp/.cpp`

Move only app-local interaction helpers, such as:

- `GizmoAxis`
- `GizmoAxisShaft`
- `gizmoAxisName`
- `heldAxisForGrabbedAxis`
- axis/pixel distance helpers if still in `main.cpp`
- `logMoveDispatch`
- `dispatchMoveReleaseWithUndo`
- `samePathPoints`
- `translatePathPoints`
- `movePathPoint`
- `movePathObjectWithUndo`
- `movePathPointWithUndo`

If E84 leaves capture scenario code calling these helpers, expose a narrow
public API from the new module. Do not duplicate helper logic in the capture
module.

Update `CMakeLists.txt` only as needed.

## Do Not

- Do not change Move dispatch semantics.
- Do not change undo push/discard semantics.
- Do not change Path mutation payloads or dirty/revision behavior.
- Do not change path point handle size, visual rendering, or picking policy
  unless a shared declaration move is required to compile.
- Do not change capture frame numbers, scripted destinations, or final scene.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` no longer defines the moved gizmo/path-edit helpers.
- Capture proof logs for Move, Point, Line, Path, and Path Handle still appear.
- Round-trip proof remains unchanged:
  `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
- Final standalone capture still reports `package_room_meshes_presented`.
- RoomBake final counts remain unchanged.
- Baseline/final capture hashes should match unless existing capture drift is
  observed and explained with stable scalar receipts.

## Suggested Verification

Baseline and final capture commands should use `--frames 32` and grep for:

```sh
rg "MOVE|POINT|LINE|PATH|PATH_HANDLE|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e85_final.log
```

Also run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
git -C /Users/kogaryu/iggy3d diff --check
```

and a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Baseline/final capture hashes:
- Move/Point/Line/Path proof lines preserved:
- RoomBake and round-trip proof before/after:
- `main.cpp` line-count delta:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `apps/iggy3d_creative/main.cpp`
  - `apps/iggy3d_creative/StandaloneCaptureScenario.hpp`
  - `apps/iggy3d_creative/StandaloneCaptureScenario.cpp`
  - `apps/iggy3d_creative/StandaloneGizmo.hpp`
  - `apps/iggy3d_creative/StandaloneGizmo.cpp`
  - `apps/iggy3d_creative/StandalonePathEditing.hpp`
  - `apps/iggy3d_creative/StandalonePathEditing.cpp`
- Behavior changed: refactor only. Gizmo axis types, held-axis mapping, gizmo hit choice, move dispatch logging, move-release undo wrapping, and whole-path/path-point mutation helpers moved out of `main.cpp`.
- Baseline/final capture hashes:
  - Baseline `/tmp/iggy3d_creative_e85_baseline.png`: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Final `/tmp/iggy3d_creative_e85_final.png`: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Submit reason remained `package_room_meshes_presented`.
- Move/Point/Line/Path proof lines preserved: `MOVE dispatch CAPTURE_MOVE_*`, `POINT before/after move/undo`, `LINE before/after move/undo`, `PATH move commit`, `PATH mutation receipt`, `PATH_HANDLE move commit`, `PATH_HANDLE mutation receipt`, and `PATH_HANDLE after undo`.
- RoomBake and round-trip proof before/after:
  - `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 skippedUnsupported=1 standalonePreviewMeshes=3 sceneMeshes=1690`
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
- `main.cpp` line-count delta: `1985 -> 1639` (`-346` lines).
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e85_baseline.png --frames 32 > /tmp/iggy3d_creative_e85_baseline.log 2>&1`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e85_final.png --frames 32 > /tmp/iggy3d_creative_e85_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e85_final.png`
  - `rg "MOVE|POINT|LINE|PATH|PATH_HANDLE|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e85_final.log`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[ \t]+$" /Users/kogaryu/iggy3d/CMakeLists.txt /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneCaptureScenario.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneCaptureScenario.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneGizmo.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneGizmo.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePathEditing.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePathEditing.cpp`
- Concerns/deferred: `main.cpp` still owns the interactive state-machine branches and UI/wireframe assembly. E86 should be a cleanup pass over that orchestration rather than another behavior extraction.
