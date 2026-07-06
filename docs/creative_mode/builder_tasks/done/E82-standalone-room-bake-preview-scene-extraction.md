# E82: Standalone RoomBake Preview Scene Extraction

## Objective

Extract the standalone app's RoomBake preview scene-build plumbing out of
`apps/iggy3d_creative/main.cpp` without changing capture output.

## Problem

E77-E81 moved visual proxies, picking, brush/placement policy, and renderer
bootstrap out of the standalone app file. `main.cpp` still owns a coherent
preview-scene block:

- grid-dot mesh append helpers,
- construction of the fixed standalone `CreativeRoomBakeRequest`,
- `CreativeDocument -> RoomAsset` bake call,
- product scene projection of the baked `RoomAsset`,
- appending grid dots and standalone-only preview proxies,
- tracking `standalonePreviewMeshCount`,
- final `ROOM_BAKE` scalar log line.

That is RoomBake preview consumption, not main-loop orchestration.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E77-standalone-visual-proxy-extraction.md`
- `docs/creative_mode/builder_tasks/done/E78-standalone-section-map-dependency-audit.md`
- `docs/creative_mode/builder_tasks/done/E81-standalone-renderer-bootstrap-extraction.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
- `CMakeLists.txt`

## Scope

Create:

- `apps/iggy3d_creative/StandaloneRoomBakePreview.hpp`
- `apps/iggy3d_creative/StandaloneRoomBakePreview.cpp`

Move only preview scene-build ownership:

- `gridDotSizeFor`
- `appendGridDotsToScene`
- fixed standalone bake request construction:
  - `roomId = "iggy3d_creative_preview"`
  - `sourceName = "apps/iggy3d_creative"`
  - `sourceSubset = "standalone_preview"`
- the call to `creative::buildRoomAssetFromCreativeDocument(...)`
- the call to `buildSceneProjection(emptyRuntimeState, &roomBake.room)`
- grid append and `appendStandalonePreviewProxiesToScene(...)`
- the `scene.room.loaded/staticMeshCount` finalization that belongs to the
  preview scene build
- the final `ROOM_BAKE` scalar log helper if it can move without pulling in
  unrelated final-frame logging

Suggested API shape:

```cpp
struct StandaloneRoomBakePreviewScene {
  SceneProjectionResult scene;
  creative::CreativeRoomBakeResult roomBake;
  std::size_t standalonePreviewMeshCount = 0;
};

StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const creative::CreativeDocument& document,
    const ProductMapMakerGridSnapshot& gridSnapshot);

void logStandaloneRoomBakeFinal(const StandaloneRoomBakePreviewScene& preview);
```

Use a different name if it fits local style better. Keep the API narrow and
standalone-owned.

Update `CMakeLists.txt` only as needed to compile the new `.cpp`.

## Do Not

- Do not change `RoomBake` adapter policy.
- Do not change `StandalonePreviewProxies` policy.
- Do not change grid dot size, id, material, Y-plane filtering, or loaded/count
  semantics.
- Do not change placement, picking, gizmo/path editing, save/load proof,
  capture script sequencing, renderer bootstrap, or the main frame loop order.
- Do not move final selection/frame logging except the `ROOM_BAKE final` line.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` no longer owns the RoomBake preview scene-build block or grid-dot
  append helpers.
- `main.cpp` still owns the frame loop and calls one named helper to produce
  the preview scene/result for the frame.
- Proxy suppression still uses `CreativeRoomBakeResult.staticMeshSources`
  through the existing `appendStandalonePreviewProxiesToScene(...)` API.
- RoomBake final counts remain unchanged.
- Round-trip proof remains unchanged.
- Final standalone capture reports `package_room_meshes_presented`.
- Baseline/final capture hashes should match unless the task exposes existing
  nondeterministic framing drift; if they differ, scalar receipts and visual
  inspection must be reported.

## Suggested Verification

Baseline before edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e82_baseline.png --frames 32 > /tmp/iggy3d_creative_e82_baseline.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e82_baseline.png
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e82_baseline.log
```

Final after edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e82_final.png --frames 32 > /tmp/iggy3d_creative_e82_final.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e82_final.png
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e82_final.log
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Baseline capture hash / submit reason:
- Final capture hash / submit reason:
- RoomBake final counts before/after:
- Round-trip proof before/after:
- `main.cpp` line-count delta:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/StandaloneRoomBakePreview.hpp`
  - `apps/iggy3d_creative/StandaloneRoomBakePreview.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `CMakeLists.txt`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Behavior changed: none intended. The fixed standalone RoomBake preview scene
  assembly moved out of `main.cpp` into `StandaloneRoomBakePreview.*`.
  `main.cpp` still owns frame-loop ordering and now calls
  `buildStandaloneRoomBakePreviewScene(...)` plus
  `logStandaloneRoomBakeFinal(...)`.
- Baseline capture hash / submit reason:
  - `/tmp/iggy3d_creative_e82_baseline.png`
  - `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - final frame reason `package_room_meshes_presented`
- Final capture hash / submit reason:
  - `/tmp/iggy3d_creative_e82_final.png`
  - `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - final frame reason `package_room_meshes_presented`
- RoomBake final counts before/after:
  - Before: `status='baked' reasonCode='creative_room_baked' accepted=1
    objectCount=8 considered=8 staticMeshes=6 spatialSurfaces=11
    skippedHidden=0 skippedEditorOnly=0 skippedNoBounds=0
    skippedUnsupported=1 skippedRoomMetadata=0 standalonePreviewMeshes=3
    sceneMeshes=1690`
  - After: `status='baked' reasonCode='creative_room_baked' accepted=1
    objectCount=8 considered=8 staticMeshes=6 spatialSurfaces=11
    skippedHidden=0 skippedEditorOnly=0 skippedNoBounds=0
    skippedUnsupported=1 skippedRoomMetadata=0 standalonePreviewMeshes=3
    sceneMeshes=1690`
- Round-trip proof before/after:
  - Before: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - After: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
- `main.cpp` line-count delta:
  - Before E82 edits: 2733 lines
  - After E82 edits: 2653 lines
  - Delta: -80 lines
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e82_baseline.png --frames 32 > /tmp/iggy3d_creative_e82_baseline.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e82_baseline.png`
  - `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e82_baseline.log`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e82_final.png --frames 32 > /tmp/iggy3d_creative_e82_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e82_final.png`
  - `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e82_final.log`
  - Visual inspection of `/tmp/iggy3d_creative_e82_final.png`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - No live/in-game window verification was run. Verification stayed to the
    standalone headless capture path.
  - Remaining standalone extraction candidates are still capture scenario
    orchestration, gizmo/path editing, and final main-loop cleanup.
