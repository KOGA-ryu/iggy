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
