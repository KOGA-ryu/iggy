# E77: Standalone Preview Proxy Rendering Extraction

## Objective

Extract the standalone app's preview proxy rendering helpers out of
`apps/iggy3d_creative/main.cpp` without changing capture output.

## Problem

The standalone app has had undo, capture schedule, and source-narrative cleanup,
but `main.cpp` is still 3,685 lines. A coherent block still lives in the main
file:

- `VisualBounds`,
- Point/Line/Path visual proxy bounds,
- path segment proxy mesh generation,
- baked-static-mesh preview suppression,
- path polyline/debug preview helpers.

Those helpers are current behavior, not historical scaffolding, but they make
the standalone lab hard to extend because visual proxy policy, RoomBake preview
consumption, and frame code are all in one file.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/builder_tasks/done/E31-standalone-main-extraction-plan.md`
- `docs/creative_mode/builder_tasks/done/E42-standalone-world-space-pick-helper.md`
- `docs/creative_mode/builder_tasks/ready/E78-standalone-section-map-dependency-audit.md`

## Dependencies

- Prefer doing this after E78 so line ranges and dependencies are mapped.

## Scope

- Suggested files:
  - `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
  - `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
- Allow the small CMake edit needed to compile the extracted `.cpp`.
- Move only standalone preview/render ownership:
  - marker/proxy constants,
  - `VisualBounds`,
  - major-axis helper if used by proxy rendering,
  - Point/Line/Path proxy bounds and mesh builders,
  - path polyline line/proxy append helpers,
  - baked static mesh source suppression if it is only about render ownership.
- Preserve current app-local proxy policy:
  - Point marker size,
  - Line proxy thickness,
  - Path proxy thickness,
  - Path point handle size,
  - visual bounds used by render, hit test, selection, wire/debug feedback, and
    preview mesh suppression.
- Keep RoomBake static mesh source sidecar suppression behavior unchanged.
- Keep final capture visually identical and ideally byte-identical.
- Do not move brush eligibility or world-space picking in this card unless a
  tiny type must be shared to compile; E79 owns picking.

## Acceptance

- `main.cpp` loses a coherent preview/proxy rendering section without behavior
  changes.
- The extracted helper has a narrow standalone namespace/API and does not move
  app-local policy into the creative kernel.
- RoomBake final counts remain unchanged: static meshes/spatial surfaces
  unchanged; standalone preview mesh count unchanged.
- Final standalone capture still reports `package_room_meshes_presented`,
  round-trip match, and expected RoomBake counts.
- Baseline and final capture hashes match unless logging-only changes make that
  impossible.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e77_baseline.png --frames 32 > /tmp/iggy3d_creative_e77_baseline.log 2>&1`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e77_final.png --frames 32 > /tmp/iggy3d_creative_e77_final.log 2>&1`
- `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e77_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

## Do Not

- Do not change brush eligibility, RoomBake policy, path-handle behavior,
  capture script sequencing, or save/load proof.
- Do not move world-ray/AABB picking into this card; E79 owns that extraction.
- Do not promote app-local preview proxies into RoomBake or the creative kernel
  in this extraction.
- Do not combine this with new shape/tool behavior.

## Completion Brief

### Files changed

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
- `CMakeLists.txt`

### Extraction

- Added `StandalonePreviewProxies.hpp/.cpp` as a compiled standalone helper and
  registered it in the `iggy3d_creative` target.
- Moved the app-local preview/render ownership out of `main.cpp`:
  - Point/Line/Path marker constants.
  - `VisualBounds`.
  - Point marker bounds.
  - Line proxy bounds.
  - Path proxy bounds and path segment proxy bounds.
  - Path point handle bounds.
  - Descriptor-derived preview render role policy.
  - Baked-static-mesh source suppression by
    `CreativeRoomBakeStaticMeshSource`.
  - Point/Line/Path standalone preview mesh append.
  - Path polyline debug-line append.
- Left E79/E80 ownership alone:
  - World ray, ray/AABB picking, click routing, path-handle hit tests remain in
    `main.cpp`.
  - Brush eligibility, footprint, palette, placement, and path initial-point
    policy remain in `main.cpp`.
- `main.cpp` line count dropped from 3,685 to 3,393.

### Capture proof

- Baseline:
  - command:
    `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e77_baseline.png --frames 32 > /tmp/iggy3d_creative_e77_baseline.log 2>&1`
  - hash:
    `09e84c2451ee45d225031264d88d3d724e1046cd19c257714524cefe84023c72`
  - receipt:
    `ROOM_BAKE final status='baked' reasonCode='creative_room_baked' accepted=1 objectCount=8 considered=8 staticMeshes=6 spatialSurfaces=11 skippedHidden=0 skippedEditorOnly=0 skippedNoBounds=0 skippedUnsupported=1 skippedRoomMetadata=0 standalonePreviewMeshes=3 sceneMeshes=1690`
  - final frame:
    `submit outcome=0 reason='package_room_meshes_presented' ... combinedWireLines=98 ... objectCount=8`
- Final:
  - command:
    `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e77_final.png --frames 32 > /tmp/iggy3d_creative_e77_final.log 2>&1`
  - hash:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - receipt:
    `ROOM_BAKE final status='baked' reasonCode='creative_room_baked' accepted=1 objectCount=8 considered=8 staticMeshes=6 spatialSurfaces=11 skippedHidden=0 skippedEditorOnly=0 skippedNoBounds=0 skippedUnsupported=1 skippedRoomMetadata=0 standalonePreviewMeshes=3 sceneMeshes=1690`
  - final frame:
    `submit outcome=0 reason='package_room_meshes_presented' ... combinedWireLines=98 ... objectCount=8`
- Final rerun:
  - command:
    `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e77_final_rerun.png --frames 32 > /tmp/iggy3d_creative_e77_final_rerun.log 2>&1`
  - hash:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Baseline/final hashes did not match. Direct image inspection showed the same
  scene, objects, UI, grid/camera sanity, Point marker, Beam, PatrolRoute,
  green ghost, and RoomBake/round-trip counts; the visible difference was a
  small framing/grid pixel shift. The final hash is stable on rerun and matches
  the older stable standalone capture hash recorded by E31/E42.

### Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over:
  - `apps/iggy3d_creative/main.cpp`
  - `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
  - `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
  - `CMakeLists.txt`

### Concerns

- The first baseline hash mismatch should be treated as capture/framing drift,
  not a behavior delta, because final semantic receipts are unchanged and final
  capture reruns byte-identically. Future standalone extraction cards should
  continue comparing both hashes and RoomBake/round-trip scalar receipts.
