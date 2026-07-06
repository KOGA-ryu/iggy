# E86: Standalone Main Loop Cleanup Pass

## Objective

Perform a final behavior-preserving cleanup pass on
`apps/iggy3d_creative/main.cpp` after the standalone helper extractions.

## Dependency

Do this after E85 is complete.

## Problem

After E77-E85, `main.cpp` should be much closer to orchestration. It will still
likely contain stale comments, unused includes, unused `using` aliases, and
overlong comments left behind by extraction.

This card is not another feature extraction. It is a cleanup pass to keep the
remaining file honest.

## Required Reads

- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E84-standalone-capture-scenario-orchestration-extraction.md`
- `docs/creative_mode/builder_tasks/done/E85-standalone-gizmo-path-editing-extraction.md`
- `apps/iggy3d_creative/main.cpp`

## Scope

Allowed changes:

- remove unused includes,
- remove unused `using` aliases,
- update comments that are stale after extraction,
- collapse comments that narrate old slice history instead of current structure,
- move tiny declarations only if the move is necessary for include hygiene,
- add short section comments only where they clarify current orchestration.

Suggested target:

- `main.cpp` should read as app orchestration: argument parsing, window/render
  lifetime, document setup, frame loop, input/event handling, calls into named
  standalone modules, final capture/shutdown.

## Do Not

- Do not change logic.
- Do not move more behavior into new modules in this card.
- Do not change capture sequencing, frame order, input behavior, RoomBake,
  placement, picking, path editing, save/load, or renderer bootstrap.
- Do not stage, commit, push, launch a live window, or run broad CTest.

## Acceptance

- `main.cpp` has no obvious stale slice-era comments.
- `main.cpp` has fewer unused dependencies and less narrative bulk.
- Build and capture behavior remain stable.
- Final standalone capture reports `package_room_meshes_presented`.
- RoomBake final counts and round-trip proof remain unchanged.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
./build/iggy3d_creative --capture /tmp/iggy3d_creative_e86_final.png --frames 32 > /tmp/iggy3d_creative_e86_final.log 2>&1
shasum -a 256 /tmp/iggy3d_creative_e86_final.png
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e86_final.log
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Cleanup performed:
- Final capture hash / submit reason:
- RoomBake and round-trip proof:
- `main.cpp` line-count delta:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/main.cpp`
  - builder queue bookkeeping files for claim/done movement
- Behavior changed:
  - None intended. This was a comment/include/using cleanup only.
- Cleanup performed:
  - Removed unused standalone app dependencies: `<algorithm>`, `<iterator>`,
    `<limits>`, and `app/iggy3d/creative/mutation/Mutation.hpp`.
  - Removed unused local using aliases for `discardUndoSnapshot` and
    `pointToSegmentDistancePx`.
  - Replaced the old slice-era top narrative with current ownership notes for
    `Standalone*` helpers and RoomBake-backed preview geometry.
  - Removed stale `SLICE`/`slice` labels from section comments; `rg -n
    'SLICE|slice' apps/iggy3d_creative/main.cpp` now returns no matches.
- Final capture hash / submit reason:
  - `/tmp/iggy3d_creative_e86_final.png`
  - SHA-256:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Submit reason: `package_room_meshes_presented`
- RoomBake and round-trip proof:
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - `ROOM_BAKE final status='baked' reasonCode='creative_room_baked'
    accepted=1 objectCount=8 considered=8 staticMeshes=6
    spatialSurfaces=11 skippedHidden=0 skippedEditorOnly=0 skippedNoBounds=0
    skippedUnsupported=1 skippedRoomMetadata=0 standalonePreviewMeshes=3
    sceneMeshes=1690`
  - Final frame: `objectCount=8`, `placed=7`, `brush='Wall'`,
    `ghostEdges=12`.
- `main.cpp` line-count delta:
  - E85 final baseline: 1639 lines.
  - E86 final: 1635 lines.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e86_final.png
    --frames 32 > /tmp/iggy3d_creative_e86_final.log 2>&1`
  - `shasum -a 256 /tmp/iggy3d_creative_e86_final.png`
  - `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome"
    /tmp/iggy3d_creative_e86_final.log`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over `main.cpp`, this card, and
    `PRIORITY.md`.
- Concerns/deferred:
  - `main.cpp` is now small enough for orchestration review, but future cards
    should avoid new feature logic in it. Additional cleanup should be driven by
    concrete ownership seams, not more broad extraction.
