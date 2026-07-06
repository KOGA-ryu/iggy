# E65: Standalone Capture Proof Step Tags

## Objective

Replace raw boolean proof-target tuples in the standalone capture script with
named step tags or a small enum/set so new capture proof rows are reviewable.

## Problem

E37 moved the standalone capture schedule out of `main.cpp`, which reduced the
main-file bottleneck. The extracted schedule now stores proof behavior in
`StandaloneCapturePlacement` as several adjacent booleans:

- `deleteProofTarget`
- `moveProofTarget`
- `createUndoProofTarget`
- `pointProofTarget`
- `lineProofTarget`
- `pathProofTarget`

The placement table then contains rows of `false, true, false, ...`. That is a
smaller version of the descriptor raw-boolean problem: a new proof row can look
right while one flag is shifted or misread.

## Required Reads

- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/main.cpp`
- `docs/creative_mode/builder_tasks/done/E37-standalone-capture-script-extraction.md`

## Evidence

- `StandaloneCaptureScript.hpp:27-38` defines `StandaloneCapturePlacement` with
  six boolean proof flags.
- `StandaloneCaptureScript.hpp:63-78` initializes the placement schedule with
  raw boolean tuples.
- `main.cpp` still consumes those flags to assign proof target ids.

## Scope

- Replace adjacent proof booleans with a named representation, for example:
  - `enum class StandaloneCaptureProofRole`,
  - `std::array<StandaloneCaptureProofRole, N>`,
  - or a small bitmask helper with named constructors.
- Keep frame numbers, object kinds, placement positions, proof behavior, and
  final capture output unchanged.
- Keep the helper lightweight; this is a readability/correctness cleanup, not a
  new capture feature.

## Acceptance

- Placement rows read like named proof intent, not positional boolean tuples.
- Adding a new proof role does not require appending another bool to every row.
- Capture proof behavior remains identical, including target id assignment and
  final capture hash if no logging-only differences are introduced.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e65_final.png --frames 32 > /tmp/iggy3d_creative_e65_final.log 2>&1`
- `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e65_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change capture proof sequencing.
- Do not change brush eligibility, RoomBake behavior, or path-handle behavior.
- Do not widen into main-file extraction beyond the proof-role representation.

## Completion Brief

- Files modified:
  - `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
  - `apps/iggy3d_creative/main.cpp`
- Replaced the six adjacent raw proof booleans on
  `StandaloneCapturePlacement` with named `StandaloneCaptureProofRole` values:
  - `None`
  - `Delete`
  - `Move`
  - `CreateUndo`
  - `Point`
  - `Line`
  - `Path`
- Added `capturePlacementHasProofRole(...)` and updated `main.cpp` to derive
  the same local proof-target booleans from the named role.
- Placement rows now read as named proof intent rather than positional boolean
  tuples.
- Frame numbers, object kinds, placement positions, proof target assignment,
  brush behavior, RoomBake behavior, and path-handle behavior were unchanged.

Capture proof:

- Command:
  `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e65_final.png --frames 32 > /tmp/iggy3d_creative_e65_final.log 2>&1`
- PNG: `/tmp/iggy3d_creative_e65_final.png`
- Log: `/tmp/iggy3d_creative_e65_final.log`
- SHA-256:
  `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
- Receipt evidence:
  - `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
  - `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 skippedUnsupported=1 standalonePreviewMeshes=3 sceneMeshes=1690`
  - `FINAL frame 32 submit outcome=0 reason='package_room_meshes_presented' ... objectCount=8`

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
- Capture command above
- `shasum -a 256 /tmp/iggy3d_creative_e65_final.png`
- `rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e65_final.log`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over `apps/iggy3d_creative/main.cpp` and
  `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
