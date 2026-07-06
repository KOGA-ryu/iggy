# E28: Parent Relationship Integrity

## Objective

Make CreativeDocument parent relationships truthful now that Generate Room Shell
creates Floor/Wall children parented to a Room object.

## Problem

Room shell children are authored objects with `parentId = Room`, but document
remove and restore do not currently protect that relationship. Deleting a Room
can leave orphan generated shell children, and restore can accept objects whose
parent id does not exist.

## Required Reads

- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/tools/RoomShell.cpp`
- `tests/unit/creative_room_shell_tests.cpp`
- `tests/unit/creative_document_path_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

- Add document-level relationship integrity for `parentId`.
- Add focused tests for parent delete/restore behavior.
- Keep RoomBake policy unchanged: Room remains metadata, generated children bake
  as normal authored Floor/Wall objects.

## Recommended Policy

- Restore rejects an object with `parentId` that does not refer to another
  restored object.
- Delete of a parent Room should not leave generated shell children orphaned.
  Either cascade generated shell children or reject parent delete while generated
  children exist. Prefer cascade for generated shell children only if the receipt
  can report it clearly.

## Acceptance

- A document with generated shell children cannot restore with a missing parent.
- Deleting a generated shell parent cannot leave children with a dangling
  `parentId`.
- Product-live Delete Selected after Generate Room Shell has deterministic
  active-room/undo behavior.
- Existing create/delete/undo/path tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d -j10`
- `cmake --build /Users/kogaryu/iggy3d/build --target creative_room_shell_tests creative_document_path_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|creative_document_path_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add RoomBake direct geometry for Room.
- Do not add app-local cleanup outside document truth.
- Do not silently delete unrelated non-generated children without an explicit
  receipt/test.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/creative/document/Document.hpp`
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `tests/unit/creative_room_shell_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
- Behavior changed:
  - `restoreForLoad(...)` now rejects restored objects whose `parentId` is unsupported, invalid/self-referential, or missing from the restored object set.
  - `removeDocumentObject(...)` now rejects deleting any object that still has children with `ParentHasChildren` / `parent_has_children`, so parent deletes cannot orphan generated shell children.
  - Product-live Delete Selected after Generate Room Shell now deterministically rejects Room deletion, preserves active room/collision, does not push undo, and leaves the existing shell undo path intact.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_room_shell_tests creative_document_path_tests product_creative_world_launch_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|creative_document_path_tests|product_creative_world_launch_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files and this task card
- Evidence:
  - `creative_room_shell_tests` covers restore rejection for generated shell children with a missing parent and parent delete rejection while generated shell children remain.
  - `product_creative_world_launch_tests` covers Generate Room Shell, rejected parent Room delete, unchanged active room/undo state, and subsequent Undo clearing shell geometry.
  - All focused tests passed.
- Concerns/deferred:
  - This slice rejects parent deletion while any children exist rather than cascading. A future cascade path should add explicit child-count receipt fields and tests before deleting generated descendants.
