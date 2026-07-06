# E66: Standalone Source Narrative Cleanup

## Objective

Remove or relocate stale historical slice narrative from
`apps/iggy3d_creative/main.cpp` so the source file describes current
architecture, not the archaeology of earlier slices.

## Problem

The standalone app remains the largest code hotspot, and its first 100+ lines
are a historical narrative for slices 3-6. That text was useful while the app was
being built, but it now competes with current code as documentation and makes
the file harder to scan.

Worse, the file now includes features added long after those comments: descriptor
palette hygiene, Point/Line/Path affordances, RoomBake consumption, proxy
suppression, snapshot undo, save/load round-trip, and generated capture script
state. A reader starting at line 1 gets a fossil record, not a concise current
map.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `docs/creative_mode/standalone_app_handoff.md`

## Evidence

- `main.cpp:1-103` is a long historical `SLICE 3/4/5/6` comment block.
- `main.cpp` currently has about 438 comment-only lines out of 3,551 total
  lines.
- The current app includes RoomBake, Point/Line/Path, path point handles,
  standalone undo, and capture-script helpers that the opening narrative does
  not summarize as the current architecture.

## Scope

- Replace the historical top-of-file narrative with a short current-state module
  map:
  - what the standalone app is for,
  - what helpers own capture schedule and undo,
  - where RoomBake/proxy preview boundaries live,
  - what remains intentionally app-local.
- Move any still-useful historical detail to an appropriate doc only if it is
  worth preserving. Do not preserve stale text by default.
- Keep behavior unchanged.

## Acceptance

- The top of `main.cpp` is concise and current.
- Historical slice labels no longer dominate the source file.
- Any remaining comments explain current non-obvious behavior, not past slice
  implementation notes.
- Capture output and interactive behavior are unchanged.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e66_final.png --frames 32 > /tmp/iggy3d_creative_e66_final.log 2>&1`
- `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e66_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change runtime behavior.
- Do not delete comments that explain active tricky math or current invariants.
- Do not move historical narrative into another source file just to keep it.

## Completion Brief

- Files modified:
  - `apps/iggy3d_creative/main.cpp`
- Replaced the historical `SLICE 3/4/5/6` top-of-file narrative with a concise
  current module map describing:
  - the standalone app as the creative-editor lab,
  - shared creative-kernel ownership,
  - `StandaloneCaptureScript.hpp` as capture schedule/proof state owner,
  - `StandaloneUndo.hpp` as the app-local snapshot undo wrapper,
  - RoomBake-owned runtime room geometry,
  - app-local point/path/ghost preview boundaries.
- Did not move stale historical text elsewhere.
- Did not alter runtime behavior, capture schedule, RoomBake behavior, brush
  eligibility, path-handle behavior, or active tricky math comments.

Capture proof:

- Command:
  `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e66_final.png --frames 32 > /tmp/iggy3d_creative_e66_final.log 2>&1`
- PNG: `/tmp/iggy3d_creative_e66_final.png`
- Log: `/tmp/iggy3d_creative_e66_final.log`
- SHA-256:
  `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Receipt evidence:
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 skippedUnsupported=1 standalonePreviewMeshes=3 sceneMeshes=1690`
  - `FINAL frame 32 submit outcome=0 reason='package_room_meshes_presented' ... objectCount=8`

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- Capture command above
- `shasum -a 256 /tmp/iggy3d_creative_e66_final.png`
- `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e66_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over `apps/iggy3d_creative/main.cpp`
