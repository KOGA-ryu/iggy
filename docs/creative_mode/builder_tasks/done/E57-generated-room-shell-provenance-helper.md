# E57: Generated Room Shell Provenance Helper

## Objective

Centralize generated Room Shell provenance so lifecycle commands do not grow more
free-form string-tag matching.

## Problem

Generated Room Shell children are linked back to their source Room through two
plain tags:

- `generated_room_shell`
- `source_room_<id>`

Evidence:

- `src/app/iggy3d/creative/tools/RoomShell.cpp:12` defines the generated-shell
  tag literal.
- `src/app/iggy3d/creative/tools/RoomShell.cpp:14` formats `source_room_<id>`.
- `src/app/iggy3d/creative/tools/RoomShell.cpp:94` detects existing shell
  children by scanning `CreativeObject::tags`.
- `tests/unit/creative_room_shell_tests.cpp:102` and `:398` duplicate the same
  tag literals in assertions and setup.
- `CreativeObject` only has generic `std::vector<std::string> tags` plus
  optional `parentId` today (`src/app/iggy3d/creative/document/Object.hpp:180`).

The first Generate Shell command can get away with this. The next lifecycle
features, such as remove shell, regenerate shell, duplicate detection, or parent
delete repair, will be fragile if they each parse tags by hand.

## Dependencies

- Coordinate with E39. If E39 is already implementing shell lifecycle commands,
  this can be folded into that work only if the diff stays small.

## Required Reads

- `src/app/iggy3d/creative/tools/RoomShell.hpp`
- `src/app/iggy3d/creative/tools/RoomShell.cpp`
- `src/app/iggy3d/creative/document/Object.hpp`
- `tests/unit/creative_room_shell_tests.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Add a small Room Shell provenance helper API in or adjacent to
  `creative/tools/RoomShell.*`.
- Expose helper functions for:
  - generated-shell tag value;
  - source-room tag value;
  - whether an object/request is a generated shell child for a room;
  - collecting generated shell child ids for a room from a document.
- Replace duplicate string-tag literals in RoomShell tests with the helper.
- Keep the persisted authored representation as tags for now unless a narrower
  typed provenance field already exists by the time this card is picked.

## Acceptance

- No Room Shell caller or test needs to spell `"generated_room_shell"` or
  `"source_room_"` directly.
- Duplicate detection and future lifecycle work can call one helper to find
  generated children for a source Room.
- Existing Generate Room Shell behavior, save/load behavior, and RoomBake output
  are unchanged.
- Tests prove provenance helper behavior for:
  generated create requests, existing document objects, hidden generated
  objects, and unrelated tagged/non-parented objects.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add cascade delete/regenerate behavior in this card.
- Do not change the generated shell bounds, object kinds, parent ids, or names.
- Do not change save schema for a provenance field unless explicitly scoped by a
  later card.
- Do not let RoomBake start baking the Room metadata object directly.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/tools/RoomShell.hpp`
  - `src/app/iggy3d/creative/tools/RoomShell.cpp`
  - `tests/unit/creative_room_shell_tests.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
- Added Room Shell provenance helpers:
  - `generatedRoomShellTag()`
  - `sourceRoomShellTag(...)`
  - `creativeRoomShellCreateRequestHasProvenance(...)`
  - `creativeRoomShellObjectHasProvenance(...)`
  - `collectCreativeRoomShellChildIds(...)`
- Replaced duplicate Room Shell provenance string checks in command-frame and
  room-shell tests with helper calls.
- Duplicate detection, `creativeRoomHasGeneratedShellChildren(...)`, and
  remove-shell child lookup now use the centralized provenance helper surface.
- Added helper coverage for generated create requests, hidden generated shell
  objects, collection by source Room, and tagged-but-not-parented objects being
  ignored.
- Persisted representation remains generic `tags`; no save schema or RoomBake
  policy changed.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_shell_tests creative_document_room_bake_tests product_creative_ui_command_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_shell_tests|creative_document_room_bake_tests|product_creative_ui_command_frame_tests)$' --output-on-failure`
- Result: all checks passed.
