# E42: Standalone World-Space Pick Helper

## Objective

Replace the standalone app's projected-AABB center-depth picker with a reusable
world-space ray-vs-AABB pick helper.

## Problem

`apps/iggy3d_creative/main.cpp` currently projects every object to a screen AABB
and chooses the hit with the smallest center clip depth. That is simple, but it
can select the wrong object when screen boxes overlap and object centers do not
represent the nearest surface.

This is also organization debt: picking math, screen projection, object scan,
and click routing are all embedded in `main()`.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- Any available math matrix inverse helpers under `src/core/math`

## Scope

- Add a reusable standalone helper for pixel -> world ray and nearest ray/AABB
  slab hit over `visualBoundsForObject(...)`.
- Keep the current visual proxy bounds as the picking primitive.
- Use the O(n) scan as the correctness baseline; no spatial index in this slice.
- Preserve existing select/move behavior and capture proof.

## Acceptance

- Click selection uses nearest ray-entry distance, not projected center depth.
- Floor/Crate overlap and Point/Line/Path proxy selections are covered by
  focused standalone proof logs or tests where practical.
- Final standalone capture remains valid and visually sane.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e42_final.png --frames 32 > /tmp/iggy3d_creative_e42_final.log 2>&1`
- Inspect the PNG and grep the log for `package_room_meshes_presented`,
  `ROUNDTRIP`, and the expected pick/selection proof.
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add a BVH/spatial index here.
- Do not change visual proxy policy.
- Do not use per-kind pick branches.

## Completion Brief

Status: done.

Files changed:
- `apps/iggy3d_creative/main.cpp`

Behavior changed:
- Added app-local reusable world-pick helpers:
  - `worldRayFromPixel(...)`
  - `rayEntryDistanceForAabb(...)`
  - `pickNearestVisualBoundsObject(...)`
- Click selection now picks the visible object with the nearest world-space
  ray/AABB entry distance through `visualBoundsForObject(...)`, replacing the
  old projected-AABB center-depth chooser.
- Kept projected screen AABBs only for diagnostics/proxy logging.
- No BVH/spatial index, per-kind pick dispatch, or visual proxy policy change.

Tests/checks run:
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e42_final.png --frames 32 > /tmp/iggy3d_creative_e42_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`
- focused trailing-whitespace scan over `apps/iggy3d_creative/main.cpp`

Evidence:
- Build passed.
- Capture passed with SHA-256
  `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`.
- Capture submit reason stayed `package_room_meshes_presented`.
- Capture hash stayed unchanged from E41, so the visual scene is stable.
- `WORLD_PICK_PROOF` logs showed:
  - `floor_overlap` matched object id 1
  - `point_proxy` matched object id 6
  - `line_proxy` matched object id 7
  - `path_proxy` matched object id 8
- Final `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  still passed.
- Final `ROOM_BAKE` line stayed accepted with `staticMeshes=6`,
  `spatialSurfaces=11`, `skippedUnsupported=1`, `standalonePreviewMeshes=3`,
  and `sceneMeshes=1690`.
- Visual inspection showed the expected standalone scene with Floor/Wall/Crates,
  Beam, Point marker, PatrolRoute path, grid, UI, and green ghost.

Concerns/deferred:
- The helper is app-local because this slice was scoped to standalone. If product
  or other tools need identical world picking later, this should move behind a
  small shared math/tool seam.
