# E102: Kernel W5 - RoomBake Reachability Receipt

## Objective

Add a post-bake playability validation receipt that uses the shipped
`core/grid/Reachability` kernel to detect disconnected walkable cells from
runtime anchor seeds.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W5.

## Scope

- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- focused RoomBake tests only.
- Add receipt data to `CreativeRoomBakeResult` or an adjacent receipt struct.

## Required Behavior

- Add a reachability validation receipt with at least:
  - requested/checked or equivalent boolean;
  - `hasIslands`;
  - `strandedCellCount`;
  - status/reason code.
- Project walkable output into a 2D XZ grid using the existing baked room or
  existing creative spatial projection. Prefer reusing existing projection
  helpers over inventing a second geometry classifier.
- Use runtime anchor seeds when available. If no usable seeds exist, report a
  deterministic no-seed status rather than pretending the room is playable.
- Call `floodFillReachability(...)` from `src/core/grid/Reachability.hpp`.

## Hazards

- This is a validation receipt, not a bake rejection, unless tests and policy
  explicitly justify rejection.
- Cell size must be explicit and deterministic. If it becomes a request field,
  default it to current RoomBake behavior without changing existing callers.
- Seeds on blocked cells need a deterministic receipt reason/count.
- v1 is 2D XZ only. Do not solve multi-level stairs/links here.

## Do Not

- Do not alter static mesh or spatial surface bake policy.
- Do not add navmesh, patrol path gameplay, session seed mapping, or rendering.
- Do not make reachability silently fail-open without receipt evidence.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `src/core/grid/Reachability.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- existing RoomBake tests.

## Acceptance

- A room with two disconnected floor islands and one spawn/anchor seed reports
  stranded cells.
- A connected walkable layout reports zero stranded cells.
- Existing RoomBake mesh/anchor/surface counts remain unchanged.
- Existing no-renderable behavior remains unchanged.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Build `iggy3d` too if public headers or compile coverage require it.

## Completion Brief

Append:

- Files changed:
- Receipt shape:
- Reachability grid/seeds policy:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E102

- Files changed:
  - `src/app/iggy3d/creative/adapters/RoomBake.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
  - builder queue bookkeeping files for claim/done movement
- Receipt shape:
  - Added `CreativeRoomBakeReachabilityStatus`:
    - `Unknown`
    - `NotRequested`
    - `NotChecked`
    - `InvalidCellSize`
    - `NoWalkableCells`
    - `NoUsableSeeds`
    - `Reachable`
    - `IslandsFound`
  - Added `CreativeRoomBakeReachabilityReceipt` on `CreativeRoomBakeResult`
    with `requested`, `checked`, `hasIslands`, `walkableCellCount`,
    `reachedCellCount`, `strandedCellCount`, `seedAnchorCount`,
    `usableSeedCount`, `blockedSeedCount`, `cellSizeMeters`, `status`,
    `reasonCode`, and `message`.
  - Added request fields `validateReachability=true` and
    `reachabilityCellSizeMeters=1.0F`.
  - Added `toString(CreativeRoomBakeReachabilityStatus)`.
- Reachability grid/seeds policy:
  - Validation runs after RoomBake has produced static meshes, anchors, and
    spatial surfaces. It does not reject or change the main bake result.
  - Walkable cells are projected from baked `RoomSpatialSurfaceRole::Walkable`
    surfaces into a 2D XZ grid at the request cell size.
  - A floor covering `[0, 4]` at 1 m cell size marks four cells on that axis:
    cell centers `[0.5, 1.5, 2.5, 3.5]`.
  - Seeds come from runtime anchors whose kind is `spawn`, `npc`, or `monster`.
    Other anchor kinds remain baked metadata but do not seed reachability.
  - Seeds outside the walkable grid or on blocked cells increment
    `blockedSeedCount` and produce `NoUsableSeeds` when none are usable.
  - Valid seeded grids call `floodFillReachability(..., FourWay)`.
  - `NoUsableSeeds` reports all walkable cells as stranded so the room does not
    silently fail open.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/adapters/RoomBake.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/adapters/RoomBake.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_room_bake_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E102-kernel-w5-room-bake-reachability-receipt.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md`
- Concerns/deferred:
  - v1 is 2D XZ only, with four-way connectivity. Multi-level stairs, links,
    patrol paths, and navmesh semantics remain deferred.
  - Reachability uses baked room walkable spatial surfaces, so it inherits the
    current RoomBake floor surface policy rather than introducing a second
    creative geometry classifier.
  - The grid is capped at 1,000,000 cells. Oversized projections report the
    explicit `GridTooLarge` reachability status instead of attempting an
    expensive validation pass.

## Planner Review Repair

- Added explicit `CreativeRoomBakeReachabilityStatus::GridTooLarge` so a room
  with real walkable surfaces that exceeds the validation cell cap does not get
  mislabeled as `NoWalkableCells`.
- Added `oversizedWalkableProjectionReportsGridTooLarge()` to
  `creative_document_room_bake_tests`.
- Re-ran:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
