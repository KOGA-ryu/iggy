# E96: Extract RoomBake Bounds Predicate

## Objective

Expose RoomBake's bake-survival bounds predicate as a callable shared function
so pre-bake validation and RoomBake use the same logic.

## Problem

`RoomBake.cpp` currently owns `validBakeBounds(...)` in an anonymous namespace.
`validateDocumentPreBake` needs to answer the same question for degenerate
bounds without reimplementing bake policy and drifting from RoomBake.

## Scope

- Move or wrap `validBakeBounds(...)` behind a header-visible predicate.
- Keep the predicate policy unchanged.
- Use the predicate from `RoomBake.cpp`.
- Add focused tests that prove exported predicate behavior matches current bake
  acceptance/rejection for sane, degenerate, non-finite, and non-positive bounds.

## Do Not

- Do not change RoomBake include/skip policy.
- Do not change Room metadata handling.
- Do not introduce a broader pre-bake validator here.
- Do not touch SaveFileStore twins.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Acceptance

- A named header-visible predicate exists for bakeable bounds.
- RoomBake calls that predicate instead of an anonymous duplicate.
- Focused tests pin predicate results and RoomBake skip behavior.
- No RoomBake output policy changes.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files modified:
  - `src/app/iggy3d/creative/adapters/RoomBake.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Predicate API:
  - Added `creative::creativeRoomBakeBoundsAreValid(CreativeBounds) noexcept`
    as the header-visible bake-survival predicate.
  - `RoomBake.cpp` now calls that predicate from its internal
    `validBakeBounds(...)` conversion helper before producing float room
    bounds.
- Policy preserved:
  - Preserved existing requirements: finite min/max, positive extents on every
    axis, and float-safe min/max/size/center values.
  - No RoomBake include/skip, Room metadata, anchor, line, or volume policy was
    changed.
- Tests:
  - Added direct predicate coverage for sane, zero-width, inverted,
    non-finite, and over-float bounds.
  - Added RoomBake skip coverage proving finite invalid bounds on otherwise
    bake-supported Floor/Wall/Crate objects count as `skippedNoBoundsCount` and
    emit no meshes/surfaces/anchors.
  - Updated endpoint-line room-bake fixtures to create `NavLink` with the
    required two endpoint path points after E94.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing whitespace scan over touched E96 files.
  - All passed.
- Concerns/deferred:
  - Non-finite object bounds cannot reach RoomBake through public document
    create/restore validation; E96 pins non-finite rejection at the exported
    predicate and pins RoomBake skip behavior for finite invalid bounds.
