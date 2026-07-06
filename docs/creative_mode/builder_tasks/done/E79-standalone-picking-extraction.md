# E79: Standalone Picking Extraction

## Objective

Extract standalone world-space picking and hit proxy helpers from
`apps/iggy3d_creative/main.cpp` into a named standalone module.

## Problem

Standalone picking was repaired to use world ray/AABB hits, but the helper types
and hit-test logic still live inside `main.cpp` alongside rendering,
RoomBake preview, capture scripting, and interaction orchestration.

The next feature that touches hit testing should not have to parse the whole
standalone app loop.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/builder_tasks/done/E42-standalone-world-space-pick-helper.md`
- `docs/creative_mode/builder_tasks/ready/E78-standalone-section-map-dependency-audit.md`
- `docs/creative_mode/builder_tasks/ready/E77-standalone-visual-proxy-extraction.md`

## Dependencies

- Prefer doing this after E78.
- If E77 is still ready, coordinate with it: picking must consume the same visual
  proxy policy instead of duplicating marker/proxy dimensions.

## Scope

- Suggested files:
  - `apps/iggy3d_creative/StandalonePicking.hpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
- Move only picking/hit-testing ownership:
  - screen/world ray types,
  - ray/AABB entry math,
  - object visual pick result types,
  - nearest object hit scan,
  - path point handle hit proxy selection if it depends on the same picking
    primitives.
- Keep current O(n) scan. Do not add a spatial index.
- Use visual proxy bounds from the standalone preview/proxy module if E77 has
  landed; otherwise keep the API shaped so that E77 can provide those bounds
  later without duplicating policy.

## Acceptance

- `main.cpp` loses the picking/hit-testing helper section without behavior
  changes.
- Deterministic standalone capture still selects and moves Point, Line, Path,
  and path point handles as before.
- Final capture remains valid and visually sane, with `package_room_meshes_presented`.
- Round-trip proof still passes.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e79_baseline.png --frames 32 > /tmp/iggy3d_creative_e79_baseline.log 2>&1`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e79_final.png --frames 32 > /tmp/iggy3d_creative_e79_final.log 2>&1`
- `rg "WORLD_PICK_PROOF|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e79_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

## Do Not

- Do not change visual proxy dimensions or brush eligibility.
- Do not add BVH/spatial indexing.
- Do not widen into gizmo movement, RoomBake policy, or capture script changes.

## Completion Brief

Status: done.

Files modified or created:

- `apps/iggy3d_creative/StandalonePicking.hpp`
- `apps/iggy3d_creative/StandalonePicking.cpp`
- `apps/iggy3d_creative/main.cpp`
- `CMakeLists.txt`

Extraction result:

- Moved the standalone picking types and helpers out of `main.cpp`:
  - `ScreenAabb`
  - `ScreenPoint`
  - `WorldRay`
  - `ObjectVisualPickBounds`
  - `ObjectVisualPickResult`
  - `PathPointHandleHit`
  - `clipW`
  - `projectBoxToScreen`
  - `projectPointToScreen`
  - `pointToSegmentDistancePx`
  - `worldRayFromPixel`
  - `rayEntryDistanceForAabb`
  - `pickNearestVisualBoundsObject`
  - `buildPathPointHandleHits`
  - `pickPathPointHandle`
- `main.cpp` now consumes the picking module and keeps only the integration
  calls for object candidates, gizmo picking, path handle picking, and capture
  proofs.
- `StandalonePicking.cpp` consumes the E77 visual proxy module for path handle
  bounds, visual centers, and path-point validation, so picking does not
  duplicate visual proxy dimensions.
- No BVH/spatial indexing was added; the current O(n) scan is preserved.
- `main.cpp` line count after extraction: 3108.

Baseline capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e79_baseline.png --frames 32 > /tmp/iggy3d_creative_e79_baseline.log 2>&1`
- Hash: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Submit reason: `package_room_meshes_presented`
- Pick proofs all matched: `floor_overlap`, `point_proxy`, `line_proxy`,
  `path_proxy`.

Final capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e79_final.png --frames 32 > /tmp/iggy3d_creative_e79_final.log 2>&1`
- Hash: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Submit reason: `package_room_meshes_presented`
- Hash matched baseline.
- Visual inspection: Floor, Crates, Wall, Point marker, Beam, PatrolRoute,
  green placement ghost, grid, and camera framing remain sane.
- Final receipt evidence:
  - `WORLD_PICK_PROOF floor_overlap ... pickedObjectId=1 matched=1`
  - `WORLD_PICK_PROOF point_proxy ... pickedObjectId=6 matched=1`
  - `WORLD_PICK_PROOF line_proxy ... pickedObjectId=7 matched=1`
  - `WORLD_PICK_PROOF path_proxy ... pickedObjectId=8 matched=1`
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 skippedUnsupported=1 standalonePreviewMeshes=3 sceneMeshes=1690`

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  passed.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing whitespace scan over touched files passed.

Concerns:

- None for E79 behavior. The remaining standalone app size is still high, but
  the next queued extraction should remove brush/palette/placement policy.
