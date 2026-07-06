# E81: Standalone Renderer Bootstrap Extraction

## Objective

Reduce `apps/iggy3d_creative/main.cpp` by extracting existing renderer/capture
bootstrap helpers into a narrow standalone module with no behavior changes.

## Problem

The normalized architecture tally ranks `apps/iggy3d_creative/main.cpp` as a
god integration file. E77-E80 extracted preview proxies, picking, brush palette,
and placement policy, but the file still owns renderer bootstrap and capture
helpers at the top of the source.

These helpers are coherent and low-risk to move:

- `makeCreativeVulkanRendererConfig`
- `createCreativeRenderer`
- `captureFrameToPng`

They are app bootstrap/capture plumbing, not placement, tool, RoomBake, or
CreativeDocument policy.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E78-standalone-section-map-dependency-audit.md`
- `apps/iggy3d_creative/main.cpp`
- `CMakeLists.txt`

## Scope

Create:

- `apps/iggy3d_creative/CreativeRendererBootstrap.hpp`
- `apps/iggy3d_creative/CreativeRendererBootstrap.cpp`

Move exactly these existing helper definitions out of `main.cpp`:

- `makeCreativeVulkanRendererConfig`
- `createCreativeRenderer`
- `captureFrameToPng`

Update `CMakeLists.txt` only as needed to compile the new `.cpp`.

`main.cpp` may only:

- include the new header,
- remove the moved definitions,
- keep the existing call sites compiling.

## Do Not

- Do not touch placement, brush palette, gizmo, path editing, picking,
  RoomBake preview, save/load proof, capture script sequencing, or the main
  frame loop.
- Do not change renderer config values, log strings, capture artifact paths, or
  capture result semantics.
- Do not alter Product renderer lifecycle code.
- Do not fold unrelated grid/scene helpers into this module.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` loses only the renderer/capture bootstrap helper definitions.
- No duplicate definitions remain for the three moved helpers.
- Existing call sites still use the same helper names or an equivalently narrow
  namespace-qualified API.
- Behavior is unchanged.
- Final standalone capture still reports `package_room_meshes_presented`.
- RoomBake final counts and round-trip proof remain stable.

## Suggested Verification

Baseline before edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e81_baseline.png --frames 32 > /tmp/iggy3d_creative_e81_baseline.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e81_baseline.png
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e81_baseline.log
```

Final after edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e81_final.png --frames 32 > /tmp/iggy3d_creative_e81_final.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e81_final.png
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e81_final.log
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeRendererBootstrap.hpp`
- `apps/iggy3d_creative/CreativeRendererBootstrap.cpp`
- `CMakeLists.txt`

If baseline/final hashes differ, inspect the images and explain the scalar
receipt deltas. A hash mismatch is acceptable only when the visual scene and
RoomBake/round-trip/final receipt counts remain stable.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Baseline capture hash / submit reason:
- Final capture hash / submit reason:
- RoomBake final counts before/after:
- Round-trip proof before/after:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/CreativeRendererBootstrap.hpp`
  - `apps/iggy3d_creative/CreativeRendererBootstrap.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `CMakeLists.txt`
- Behavior changed: none intended. The renderer config, Vulkan backend creation,
  and frame capture helper bodies were moved out of `main.cpp` into the
  standalone renderer bootstrap module. Existing call sites still call the same
  helper names through the narrow app namespace import.
- Baseline capture hash / submit reason:
  - `/tmp/iggy3d_creative_e81_baseline.png`
  - `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - final frame reason `package_room_meshes_presented`
- Final capture hash / submit reason:
  - `/tmp/iggy3d_creative_e81_final.png`
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
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e81_baseline.png --frames 32 > /tmp/iggy3d_creative_e81_baseline.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e81_baseline.png`
  - `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e81_baseline.log`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e81_final.png --frames 32 > /tmp/iggy3d_creative_e81_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e81_final.png`
  - `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e81_final.log`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - No live/in-game window verification was run. The verification for this card
    was limited to the standalone headless capture/build path.
  - Further standalone extraction candidates remain parked in `PRIORITY.md`.
