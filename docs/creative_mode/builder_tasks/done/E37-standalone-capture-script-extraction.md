# E37: Standalone Capture Script Extraction

## Objective

Move the deterministic standalone capture proof script out of the 2,000-line
`main()` body without changing capture output.

## Problem

`apps/iggy3d_creative/main.cpp` is still 3,648 lines, and `main()` is about
2,042 lines. The capture-only proof code for create/delete/move/point/line/path
operations is interleaved with frame building, selection, gizmo hit testing,
RoomBake preview, save/load, and interactive input.

This is the worst standalone feature-add bottleneck: changing a proof step
requires parsing unrelated rendering and interaction code.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`

## Scope

- Extract capture schedule/state/step execution into a small standalone helper
  header or `.cpp` if the app build rules are updated cleanly.
- Preserve the exact scene and final capture hash unless logging-only changes
  make that impossible.
- Keep interactive behavior unchanged.

## Acceptance

- `main.cpp` loses a coherent capture-script section.
- Baseline and final captures match visually and ideally by SHA-256.
- Final log keeps `package_room_meshes_presented`, round-trip match, and the
  expected RoomBake counts.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e37_baseline.png --frames 32 > /tmp/iggy3d_creative_e37_baseline.log 2>&1`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e37_final.png --frames 32 > /tmp/iggy3d_creative_e37_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change brush eligibility, RoomBake behavior, or path-handle behavior.
- Do not remove visual verification from the standalone proof.
- Do not turn this into a new feature slice.

## Completion Brief

- Modified `apps/iggy3d_creative/main.cpp`.
- Created `apps/iggy3d_creative/StandaloneCaptureScript.hpp`.
- Extracted standalone capture proof schedule/state into
  `StandaloneCaptureScript`: placement script rows, fixed proof frame numbers,
  proof flags, proof target ids, move destinations, and round-trip snapshot
  state.
- Moved `ObjectSnapshotEntry` into the same helper header so save/load
  round-trip snapshot state is no longer locally declared inside `main.cpp`.
- Kept interactive behavior, brush eligibility, RoomBake behavior, path-handle
  behavior, save/load behavior, and capture proof step actions unchanged.
- `main.cpp` line count after extraction: 3551; helper header line count: 119.

Baseline capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e37_baseline.png --frames 32 > /tmp/iggy3d_creative_e37_baseline.log 2>&1`
- PNG: `/tmp/iggy3d_creative_e37_baseline.png`
- Log: `/tmp/iggy3d_creative_e37_baseline.log`
- SHA-256: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Receipt: final submit reason `package_room_meshes_presented`; `ROUNDTRIP`
  match `1`; `ROOM_BAKE final` staticMeshes `6`, spatialSurfaces `11`,
  skippedUnsupported `1`, standalonePreviewMeshes `3`, sceneMeshes `1690`.

Final capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e37_final.png --frames 32 > /tmp/iggy3d_creative_e37_final.log 2>&1`
- PNG: `/tmp/iggy3d_creative_e37_final.png`
- Log: `/tmp/iggy3d_creative_e37_final.log`
- SHA-256: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Baseline/final PNGs are byte-identical.
- Visual inspection: unchanged standalone proof scene with floor/wall/crates,
  Point marker, Beam, PatrolRoute, green ghost, sane grid/camera, and no
  doubled/thick artifact.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- Baseline and final capture commands above.
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused whitespace scan over `apps/iggy3d_creative/main.cpp`,
  `apps/iggy3d_creative/StandaloneCaptureScript.hpp`, and this task card.
