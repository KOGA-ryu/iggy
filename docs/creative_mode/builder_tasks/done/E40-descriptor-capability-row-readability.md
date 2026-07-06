# E40: Descriptor Capability Row Readability

## Objective

Make `ObjectDescriptor.cpp` descriptor rows reviewable enough that adding a new
object kind does not require decoding trailing boolean tuples by position.

## Problem

`kDescriptors` is a long table of one-line `descriptor(...)` calls. The last
six arguments are raw booleans for `hasTransform`, `hasBounds`,
`canHaveParent`, `canOwnChildren`, `isRuntimeMeaningful`, and `isEditorOnly`.

Example:

```cpp
descriptor(..., boxDefaults(...), true, true, true, false, true, false)
```

This is high-risk review surface: tests can be green while a future descriptor
row swaps parent/runtime/editor semantics silently.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Replace trailing raw boolean capability arguments with a named capabilities
  helper/struct or clear named factory helpers.
- Preserve all current descriptor facts.
- Keep compile-time descriptor construction if practical.
- Do not reclassify object kinds in this slice.

## Acceptance

- Descriptor rows no longer end in ambiguous boolean tuples.
- Tests prove representative rows still have the same transform/bounds/parent,
  runtime, and editor-only facts.
- Existing descriptor, create, save, and RoomBake behavior remains unchanged.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_room_bake_tests creative_document_create_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests|creative_document_create_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change descriptor semantics while making rows readable.
- Do not add per-kind bake or palette policy here.
- Do not weaken descriptor invariant tests.

## Completion Brief

Status: done.

Files changed:
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`

Behavior changed:
- Replaced the descriptor helper's six trailing positional capability booleans
  with named local capability flags:
  - `kHasTransform`
  - `kHasBounds`
  - `kCanHaveParent`
  - `kCanOwnChildren`
  - `kRuntimeMeaningful`
  - `kEditorOnly`
  - `kNoCapabilities`
- Rewrote all 108 descriptor rows to pass named capability expressions instead
  of ambiguous raw bool tuples.
- Preserved descriptor facts; no object kind was reclassified.

Tests/checks run:
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_room_bake_tests creative_document_create_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests|creative_document_create_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- focused trailing-whitespace scan over touched source/test files

Evidence:
- Build passed.
- Focused CTest passed: 3/3 tests.
- `git diff --check` passed.
- Focused trailing-whitespace scan found no matches.
- `creative_object_descriptor_tests` now pins representative capability facts
  for Wall, Crate, PointLight, Note, NavLink, PatrolRoute, Group,
  PrefabInstance, and TestLane.

Concerns/deferred:
- Descriptor rows are still long one-line records. This slice removed the
  high-risk trailing bool tuples without reformatting the whole table.
