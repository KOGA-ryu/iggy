# E105: RoomBake Reachability Extraction

## Objective

After E104 lands, extract the post-bake reachability validation pass out of
`RoomBake.cpp` into a narrow RoomBake-adjacent helper while preserving behavior.

This is an organization card, not a policy card. `RoomBake.cpp` is now one of
the largest files in the Creative path. Reachability is already conceptually a
post-bake validator; it should not keep bloating the main bake orchestration
file.

## Dependency

Do this only after E104 is complete. If the core grid-footprint primitive is not
present yet, move this card to blocked or leave it unclaimed.

## Audit Evidence

- `src/app/iggy3d/creative/adapters/RoomBake.cpp` is about 1,356 LOC.
- Reachability owns a coherent responsibility:
  - project walkable room surfaces to a 2D grid;
  - select usable seed anchors;
  - call `core/grid/Reachability`;
  - fill `CreativeRoomBakeReachabilityReceipt`.
- That responsibility is separate from object classification, mesh emission,
  source attribution, and status of the main bake.

## Scope

Expected files:

- Add an internal helper near RoomBake, likely:
  - `src/app/iggy3d/creative/adapters/RoomBakeReachability.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBakeReachability.cpp`
- Update CMake if needed.
- Update `RoomBake.cpp` so it calls the helper and no longer contains the
  reachability projection/flood-fill implementation.
- Tests should remain focused on existing RoomBake/reachability tests unless
  the extraction exposes a missing focused target.

## API Shape

Keep the helper narrow and adapter-local. A reasonable shape:

```cpp
CreativeRoomBakeReachabilityReceipt validateCreativeRoomBakeReachability(
    const RoomAsset& room,
    const CreativeRoomBakeRequest& request);
```

If naming or namespace conventions suggest a better local name, follow the
repo. Do not make this a global product/runtime feature.

## Required Behavior

- Main RoomBake receipt behavior is unchanged.
- Reachability statuses/reason codes are unchanged.
- No-renderable, no-walkable, no-seed, grid-too-large, connected, and island
  cases remain pinned by existing tests.
- The helper should use the E104 grid-footprint primitive rather than
  reintroducing local floor/ceil/round math.

## Do Not

- Do not change RoomBake object classification.
- Do not change walkable surface generation.
- Do not change GreedyMesh floor grouping.
- Do not change session seed mapping, anchors, gameplay, renderer/Vulkan,
  save/load, input, or standalone code.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- E104 grid-footprint helper
- `src/core/grid/Reachability.hpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Acceptance

- `RoomBake.cpp` loses the reachability projection/flood-fill implementation
  while keeping the main bake orchestration readable.
- No status/reason/count changes in existing tests.
- New helper is small and cohesive; it should not become a dumping ground for
  static mesh or anchor bake policy.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Extraction shape:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `src/app/iggy3d/creative/adapters/RoomBakeReachability.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBakeReachability.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/done/E105-roombake-reachability-extraction.md`
- Extraction shape:
  - Added `validateCreativeRoomBakeReachability(const RoomAsset&, const
    CreativeRoomBakeRequest&)` as the post-bake validator entrypoint.
  - Added `initialCreativeRoomBakeReachabilityReceipt(...)` so early RoomBake
    failure returns keep the existing reachability default/not-requested state.
  - Moved walkable-grid projection, anchor seed filtering, blocked seed
    accounting, `floodFillReachability(...)` invocation, and reachability
    status/reason filling into `RoomBakeReachability.cpp`.
- Behavior preserved:
  - Main RoomBake object classification, mesh emission, greedy floor grouping,
    source sidecars, anchors, and walkable surface generation are unchanged.
  - Reachability statuses/reason codes/counts remain covered by existing
    `creative_document_room_bake_tests`.
  - The extracted helper continues using the E104 `GridFootprint` primitive.
- Tests/checks run:
  - `cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure`
  - final combined focused build/ctest listed in the slice brief
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Concerns/deferred:
  - No new focused helper test target was added because existing RoomBake tests
    already pin no-walkable, no-seed, grid-too-large, connected, and island
    reachability behavior through the public adapter output.
