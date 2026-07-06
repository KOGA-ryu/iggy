# E80: Standalone Brush Palette And Placement Extraction

## Objective

Extract standalone descriptor-driven brush palette and placement request policy
from `apps/iggy3d_creative/main.cpp`.

## Problem

The standalone app correctly moved away from per-kind brush tables, but the
descriptor placement predicates and request-building helpers still live in
`main.cpp`. That keeps palette policy, placement shape support, path initial
route construction, and frame orchestration coupled in one file.

This is a feature-add cost problem: adding another shape affordance should not
require editing the standalone loop directly.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/builder_tasks/done/E72-authoring-palette-visibility-as-descriptor-intent.md`
- `docs/creative_mode/builder_tasks/ready/E78-standalone-section-map-dependency-audit.md`

## Dependencies

- Prefer doing this after E78 so destination boundaries are confirmed.

## Scope

- Suggested files:
  - `apps/iggy3d_creative/StandaloneBrushPalette.hpp`
  - `apps/iggy3d_creative/StandaloneBrushPalette.cpp`
  - optionally `apps/iggy3d_creative/StandalonePlacement.hpp/.cpp` if request
    construction is too large for the palette module.
- Move:
  - `BrushFootprint`,
  - descriptor support predicates for Box/Surface/MeshProxy, Point, Line, Path,
  - standalone brush palette filtering,
  - footprint derivation from descriptor defaults,
  - generic placement request construction,
  - initial Path route construction if it is currently embedded in placement.
- Preserve descriptor-driven policy. Deterministic capture representatives may
  still name `PointLight`, `Beam`, or `PatrolRoute`, but core palette policy
  must not become a kind table.

## Acceptance

- `main.cpp` loses the standalone brush/palette/placement helper section without
  behavior changes.
- Palette counts and capture proof remain stable unless the extraction exposes a
  real bug; any change must be directly explained in the completion brief.
- No per-kind brush tables are introduced.
- Final capture remains valid with `package_room_meshes_presented`, stable
  RoomBake counts, and round-trip match.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e80_baseline.png --frames 32 > /tmp/iggy3d_creative_e80_baseline.log 2>&1`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e80_final.png --frames 32 > /tmp/iggy3d_creative_e80_final.log 2>&1`
- `rg "brush palette|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e80_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

## Do Not

- Do not add per-kind palette/footprint/render-role tables.
- Do not change creative descriptor truth or RoomBake policy.
- Do not fold picking, gizmo, or capture scripting into this extraction.

## Completion Brief

Status: done.

Files modified or created:

- `apps/iggy3d_creative/StandaloneBrushPalette.hpp`
- `apps/iggy3d_creative/StandaloneBrushPalette.cpp`
- `apps/iggy3d_creative/StandalonePlacement.hpp`
- `apps/iggy3d_creative/StandalonePlacement.cpp`
- `apps/iggy3d_creative/main.cpp`
- `CMakeLists.txt`

Extraction result:

- Moved standalone descriptor brush policy into `StandaloneBrushPalette`:
  - `BrushFootprint`
  - descriptor support predicates for Box/Surface/MeshProxy, Point, Line, Path
  - `descriptorShowsInAuthoringBrushPalette` filtering
  - descriptor-derived footprint policy
  - palette construction and brush cycling
  - initial Path route construction from the aimed cell
- Moved placement request and create execution into `StandalonePlacement`:
  - `snapGroundToCellCenter`
  - `pathPointsSummary`
  - `buildBrushCreateRequest`
  - `placeBrushObject`
  - `placeBrushObjectWithUndo`
- `main.cpp` now keeps orchestration: it asks the palette module for brushes,
  asks placement for snapped cells/creates, and still owns the frame loop,
  capture schedule, and interaction order.
- No per-kind palette or footprint table was added.
- Descriptor truth and RoomBake policy were not changed.
- `main.cpp` line count after extraction: 2818.

Baseline capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e80_baseline.png --frames 32 > /tmp/iggy3d_creative_e80_baseline.log 2>&1`
- Hash: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Palette log: `before=79 after=66 removed=13`
- Submit reason: `package_room_meshes_presented`

Final capture:

- Command: `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e80_final.png --frames 32 > /tmp/iggy3d_creative_e80_final.log 2>&1`
- Hash: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Hash matched baseline.
- Palette log remained `before=79 after=66 removed=13`.
- Final receipt evidence:
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 skippedUnsupported=1 standalonePreviewMeshes=3 sceneMeshes=1690`
  - `FINAL frame 32 ... reason='package_room_meshes_presented' ... brush='Wall' ghostEdges=12 objectCount=8`
- Visual inspection: stable Floor/Crates/Wall/Point marker/Beam/PatrolRoute scene,
  green Wall ghost, sane grid/camera, no placement or palette regression.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  passed.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing whitespace scan over touched files passed.

Concerns:

- None for E80 behavior. Remaining standalone bulk is mostly frame-loop
  orchestration, capture scenario execution, move/gizmo/path interaction,
  RoomBake preview consumption, and persistence proof.
