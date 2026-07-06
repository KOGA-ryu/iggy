# E84: Standalone Capture Scenario Orchestration Extraction

## Objective

Extract deterministic standalone `--capture` scenario orchestration out of
`apps/iggy3d_creative/main.cpp` without changing the proof scene or interactive
behavior.

## Problem

E77-E83 extracted several standalone helper modules, and `main.cpp` is now about
2,515 lines. The remaining capture proof still owns large frame-gated scenario
blocks inline:

- scripted placements,
- delete no-selection proof,
- create undo proof,
- selected delete/undo proof,
- move proof,
- Point move/undo proof,
- Line move/undo proof,
- Path whole-route move/undo proof,
- Path point-handle move/undo proof,
- save/clear/load round-trip sequencing.

This is deterministic proof orchestration, not the interactive frame loop
itself. The underlying helper operations can stay where they are for now.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E78-standalone-section-map-dependency-audit.md`
- `docs/creative_mode/builder_tasks/done/E83-standalone-persistence-proof-extraction.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/StandalonePlacement.hpp`
- `apps/iggy3d_creative/StandalonePersistenceProof.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `CMakeLists.txt`

## Scope

Create one of:

- `apps/iggy3d_creative/StandaloneCaptureScenario.hpp`
- `apps/iggy3d_creative/StandaloneCaptureScenario.cpp`

or, if the dependencies are smaller than expected, extend
`StandaloneCaptureScript.hpp` with a companion `.cpp`.

Move only capture-scenario orchestration:

- the loop over `captureScript.placements`,
- the frame-gated blocks for delete/create-undo/delete-undo/move/Point/Line/Path
  proof steps,
- the frame-gated save/clear/load round-trip sequence.

Keep existing helper functions as dependencies; do not fold their behavior into
the new module unless doing so is required to avoid impossible dependencies.

Expected main-loop shape after extraction:

```cpp
runStandaloneCaptureScenarioStep(request);
```

where the request carries the current frame index, app state, undo stack,
capture script state, brush/place mode fields, save root/id, and the current
camera/frame data needed for scripted picking/move proof.

Choose a practical request/result struct shape that keeps `main.cpp` readable.

## Do Not

- Do not change `StandaloneCaptureScript` frame numbers.
- Do not change object placements, proof object ids, scripted move
  destinations, undo sequencing, save/load sequencing, or final scene contents.
- Do not change interactive Place/Select/Move behavior.
- Do not change gizmo/path editing helper implementations in this card unless a
  tiny declaration move is required to compile.
- Do not change RoomBake, preview proxy, persistence, renderer, brush palette,
  or placement policy.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` no longer owns the large deterministic capture scenario blocks.
- `main.cpp` still owns frame-loop ordering and calls one named capture-scenario
  step helper.
- Final standalone capture still reports `package_room_meshes_presented`.
- RoomBake final counts remain unchanged.
- Round-trip proof remains unchanged:
  `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
- The proof logs for Point/Line/Path move/undo and path-point edit/undo remain
  present.
- Baseline/final capture hashes should match unless the task exposes existing
  nondeterministic framing drift; if they differ, scalar receipts and visual
  inspection must be reported.

## Suggested Verification

Baseline before edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e84_baseline.png --frames 32 > /tmp/iggy3d_creative_e84_baseline.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e84_baseline.png
rg "DELETE|UNDO applied|POINT|LINE|PATH|PATH_HANDLE|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e84_baseline.log
```

Final after edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e84_final.png --frames 32 > /tmp/iggy3d_creative_e84_final.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e84_final.png
rg "DELETE|UNDO applied|POINT|LINE|PATH|PATH_HANDLE|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e84_final.log
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Baseline capture hash / submit reason:
- Final capture hash / submit reason:
- Proof log lines preserved:
- RoomBake final counts before/after:
- Round-trip proof before/after:
- `main.cpp` line-count delta:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `apps/iggy3d_creative/main.cpp`
  - `apps/iggy3d_creative/StandaloneCaptureScenario.hpp`
  - `apps/iggy3d_creative/StandaloneCaptureScenario.cpp`
- Behavior changed: refactor only. Deterministic `--capture` scenario orchestration moved behind `runStandaloneCaptureScenarioStep(...)`; interactive Place/Select/Move and helper implementations were left in `main.cpp`.
- Baseline capture hash / submit reason: `/tmp/iggy3d_creative_e84_baseline.png`, `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`, `package_room_meshes_presented`.
- Final capture hash / submit reason: `/tmp/iggy3d_creative_e84_final.png`, `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`, `package_room_meshes_presented`.
- Proof log lines preserved: `DELETE no selection`, `DELETE removed`, create/delete/move `UNDO applied`, `POINT` move/undo, `LINE` move/undo, `PATH` move/undo, `PATH_HANDLE` move/undo, `ROUNDTRIP`, `ROOM_BAKE final`, and `FINAL frame`.
- RoomBake final counts before/after: unchanged: `staticMeshes=6`, `spatialSurfaces=11`, `skippedUnsupported=1`, `standalonePreviewMeshes=3`, `sceneMeshes=1690`.
- Round-trip proof before/after: unchanged: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
- `main.cpp` line-count delta: `2515 -> 1985` (`-530` lines).
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e84_final.png --frames 32 > /tmp/iggy3d_creative_e84_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e84_final.png`
  - `rg "DELETE|UNDO applied|POINT|LINE|PATH|PATH_HANDLE|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e84_final.log`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[ \t]+$" /Users/kogaryu/iggy3d/CMakeLists.txt /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneCaptureScenario.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneCaptureScenario.cpp`
- Concerns/deferred: the scenario module still depends on callbacks for main-local move/path/delete helper operations. That keeps E84 narrow, but E85 should extract gizmo/path-editing helpers so this request surface can shrink further.
