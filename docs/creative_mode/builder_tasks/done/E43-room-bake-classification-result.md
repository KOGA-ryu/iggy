# E43: RoomBake Classification Result

## Objective

Make RoomBake include/skip decisions explicit through one classification helper
instead of spreading shape, bounds, role, and skip-count logic through the bake
loop.

## Problem

`RoomBake.cpp` currently classifies each object through a sequence of local
checks: hidden, editor-only, Room metadata, Point anchor, static mesh support,
bounds validity, role derivation, then spatial surface emission.

The behavior is mostly correct, but feature-add cost is high. Adding a new
descriptor family requires understanding several helpers and the main loop to
know which count/reason it will produce.

## Required Reads

- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp/.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Introduce a local classification result for one object, e.g.
  `CreativeRoomBakeObjectClassification`, that captures decision type,
  role/anchor kind, validated bounds, and skip counter/reason.
- Keep public `RoomAsset` and `CreativeRoomBakeResult` behavior unchanged.
- Keep existing source sidecars and receipt counts.

## Acceptance

- The main bake loop reads as classify -> emit/skip rather than a long sequence
  of policy checks.
- Tests pin representative classifications: hidden, editor-only, Room metadata,
  Point anchor, unsupported Point, safe Line, endpoint Line, BoxVolume, Surface
  floor/wall, MeshProxy prop.
- Current RoomBake counts and sidecars remain unchanged.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests creative_object_descriptor_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|creative_object_descriptor_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change which objects bake.
- Do not add runtime anchor session-seed mapping.
- Do not re-enable BoxVolume static geometry.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
- Implementation:
  - Added a local `RoomBakeObjectDecision` and
    `RoomBakeObjectClassification` in `RoomBake.cpp`.
  - Centralized per-object bake policy in `classifyRoomBakeObject(...)`.
  - Main bake loop now reads as classify, count/emit/skip.
  - Kept public `RoomAsset`, `CreativeRoomBakeResult`, source sidecars, and
    receipt semantics unchanged.
- Tests:
  - Added `representativeBakeClassificationsRemainStable()` to pin hidden,
    editor-only, Room metadata, Point anchor, unsupported Point, safe Line,
    endpoint Line, BoxVolume, Surface floor/wall, and MeshProxy prop behavior.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests creative_object_descriptor_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|creative_object_descriptor_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
