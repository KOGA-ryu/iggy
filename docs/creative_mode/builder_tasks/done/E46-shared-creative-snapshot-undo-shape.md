# E46: Shared Creative Snapshot Undo Shape

## Objective

Remove the duplicated snapshot-stack implementation between product Creative
undo and standalone Creative undo, while preserving each app's current UX and
logging behavior.

## Problem

Product live Creative undo stores full `CreativeDocument` snapshots in
`creative::CreativeDocumentUndoStack` on `CreativeAppState`.

The standalone app now has `apps/iggy3d_creative/StandaloneUndo.hpp`, which
defines a separate `StandaloneUndoStack` with the same vector/max-depth/push/pop
shape and restores through the same `Facade::installDocument(...)` seam.

That duplication is small now, but it is exactly the kind of drift that makes
Redo, lifecycle clearing, and snapshot policy hard to add consistently later.

## Required Reads

- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `apps/iggy3d_creative/main.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`

## Scope

- Reuse `creative::CreativeDocumentUndoStack` and its basic push/depth/clear
  helpers in the standalone path, or move the shared stack operations to a
  small creative header if that keeps ownership cleaner.
- Preserve standalone SDL log messages or wrap the shared operations with
  standalone-specific logging.
- Keep product undo receipts and behavior unchanged.
- Keep standalone snapshot depth, discard-on-failure, save/new/load clearing,
  and capture proof behavior unchanged.

## Acceptance

- There is one snapshot stack type/push/depth/clear policy for CreativeDocument
  undo.
- Standalone no longer carries its own duplicate vector/max-depth stack type.
- Product undo tests remain green.
- Standalone capture still proves create/delete/move/path undo and save/open
  round-trip.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative product_creative_ui_command_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e46_final.png --frames 32 > /tmp/iggy3d_creative_e46_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add Redo in this slice.
- Do not change undo selection-clearing behavior.
- Do not move undo history into `CreativeDocument`.
- Do not drop standalone capture proof.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/CreativeAppState.hpp`
  - `apps/iggy3d_creative/StandaloneUndo.hpp`
- Implementation:
  - Added shared `discardCreativeUndoSnapshot(...)`.
  - Added a shared `applyLastCreativeUndoSnapshot(Facade&, CreativeDocumentUndoStack&)`
    overload and kept the existing `CreativeAppState&` overload as a wrapper.
  - Replaced standalone's duplicate `StandaloneUndoStack` struct with an alias to
    `creative::CreativeDocumentUndoStack`.
  - Standalone undo logging wrappers now call shared depth, clear, push, apply,
    and discard helpers while preserving SDL log messages and behavior.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative product_creative_ui_command_frame_tests product_creative_world_launch_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e46_final.png --frames 32 > /tmp/iggy3d_creative_e46_final.log 2>&1`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
- Capture:
  - PNG: `/tmp/iggy3d_creative_e46_final.png`
  - SHA-256:
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Final submit reason: `package_room_meshes_presented`
  - Round-trip: `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`
