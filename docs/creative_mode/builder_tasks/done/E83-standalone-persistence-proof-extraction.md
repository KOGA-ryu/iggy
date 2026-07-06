# E83: Standalone Persistence Proof Extraction

## Objective

Extract standalone save/open/round-trip proof helpers out of
`apps/iggy3d_creative/main.cpp` into a narrow app-local module with no behavior
changes.

## Problem

E77-E82 reduced standalone `main.cpp` by extracting visual proxies, picking,
brush/placement policy, renderer bootstrap, and RoomBake preview scene building.
The file still owns a coherent persistence-proof section:

- `ObjectSnapshotEntry`
- `snapshotDocument`
- `logDocumentSnapshot`
- `snapshotsMatch`
- `saveStandaloneScene`
- `loadStandaloneScene`
- `clearToBlankScene`

Those helpers are save/open proof plumbing, not main-loop orchestration. They
also anchor the capture script's round-trip proof and should live in one module.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E78-standalone-section-map-dependency-audit.md`
- `docs/creative_mode/builder_tasks/done/E82-standalone-room-bake-preview-scene-extraction.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/StandaloneBrushPalette.hpp`
- `CMakeLists.txt`

## Scope

Create:

- `apps/iggy3d_creative/StandalonePersistenceProof.hpp`
- `apps/iggy3d_creative/StandalonePersistenceProof.cpp`

Move only persistence-proof ownership:

- `ObjectSnapshotEntry`
- `snapshotDocument`
- `logDocumentSnapshot`
- `snapshotsMatch`
- `saveStandaloneScene`
- `loadStandaloneScene`
- `clearToBlankScene`

If needed, update `StandaloneCaptureScript.hpp` to include the new persistence
proof header for `ObjectSnapshotEntry`.

Update `CMakeLists.txt` only as needed to compile the new `.cpp`.

`main.cpp` may only:

- include the new header,
- remove the moved definitions,
- keep existing call sites compiling,
- keep the round-trip frame sequencing exactly where it is.

## Do Not

- Do not change save/open kernel calls.
- Do not change the "save drains dirty on a document copy" behavior.
- Do not change snapshot equality semantics, including path point comparison.
- Do not change capture script frame numbers or round-trip sequencing.
- Do not move placement, delete, undo, move/gizmo, path editing, RoomBake
  preview, or renderer bootstrap logic.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` no longer defines the persistence-proof helpers.
- `ObjectSnapshotEntry` has a single app-local owner.
- Round-trip proof remains unchanged:
  `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
- Final standalone capture still reports `package_room_meshes_presented`.
- RoomBake final counts remain unchanged.
- Baseline/final capture hashes should match unless the task exposes existing
  nondeterministic framing drift; if they differ, scalar receipts and visual
  inspection must be reported.

## Suggested Verification

Baseline before edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e83_baseline.png --frames 32 > /tmp/iggy3d_creative_e83_baseline.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e83_baseline.png
rg "ROUNDTRIP|SNAPSHOT BEFORE|SNAPSHOT AFTER|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e83_baseline.log
```

Final after edits:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e83_final.png --frames 32 > /tmp/iggy3d_creative_e83_final.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e83_final.png
rg "ROUNDTRIP|SNAPSHOT BEFORE|SNAPSHOT AFTER|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e83_final.log
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Baseline capture hash / submit reason:
- Final capture hash / submit reason:
- Round-trip proof before/after:
- Snapshot log proof before/after:
- RoomBake final counts before/after:
- `main.cpp` line-count delta:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/StandalonePersistenceProof.hpp`
  - `apps/iggy3d_creative/StandalonePersistenceProof.cpp`
  - `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
  - `apps/iggy3d_creative/main.cpp`
  - `CMakeLists.txt`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Behavior changed: none intended. The persistence proof value type and helper
  functions moved from `main.cpp` / `StandaloneCaptureScript.hpp` into
  `StandalonePersistenceProof.*`. `main.cpp` still owns the exact save, clear,
  load, snapshot, and round-trip frame sequencing.
- Baseline capture hash / submit reason:
  - `/tmp/iggy3d_creative_e83_baseline.png`
  - `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - final frame reason `package_room_meshes_presented`
- Final capture hash / submit reason:
  - `/tmp/iggy3d_creative_e83_final.png`
  - `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - final frame reason `package_room_meshes_presented`
- Round-trip proof before/after:
  - Before: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - After: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
- Snapshot log proof before/after:
  - Before: `SNAPSHOT BEFORE objectCount=8`, `SNAPSHOT AFTER objectCount=8`;
    the PatrolRoute entry preserved `pathPointCount=3` and ordered points
    `(9.500,0.000,1.000)->(11.500,0.000,1.000)->(11.500,0.000,3.000)`.
  - After: `SNAPSHOT BEFORE objectCount=8`, `SNAPSHOT AFTER objectCount=8`;
    the PatrolRoute entry preserved `pathPointCount=3` and the same ordered
    points.
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
- `main.cpp` line-count delta:
  - Before E83 edits: 2653 lines
  - After E83 edits: 2515 lines
  - Delta: -138 lines
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e83_baseline.png --frames 32 > /tmp/iggy3d_creative_e83_baseline.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e83_baseline.png`
  - `rg "ROUNDTRIP|SNAPSHOT BEFORE|SNAPSHOT AFTER|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e83_baseline.log`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e83_final.png --frames 32 > /tmp/iggy3d_creative_e83_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e83_final.png`
  - `rg "ROUNDTRIP|SNAPSHOT BEFORE|SNAPSHOT AFTER|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e83_final.log`
  - Visual inspection of `/tmp/iggy3d_creative_e83_final.png`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - No live/in-game window verification was run. Verification stayed to the
    standalone headless capture path.
  - Remaining standalone extraction candidates are capture scenario
    orchestration, gizmo/path editing, and final main-loop cleanup.
