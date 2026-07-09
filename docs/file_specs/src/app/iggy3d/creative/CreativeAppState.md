# File Spec

File: `src/app/iggy3d/creative/CreativeAppState.hpp`

Verified at: `cd6c9663`

## Owns

- App-scoped creative facade container.
- Active creative world identity and save-status mirror.
- Creative undo snapshot stack and inline undo helpers.
- Undo apply receipt and identity clear/world-active helpers.

## Does Not Own

- CreativeDocument mutation internals.
- Product window god-struct mirror fields.
- Save file serialization.
- UI command row dispatch.
- Runtime room/session state.

## Reads

- `Facade` document state and install receipts.
- `CreativeDocument` validity, id, revision, and object count.
- Undo stack depth and max depth.
- Active identity strings and document id.

## Writes / Mutates

- Mutates `CreativeActiveIdentity` through `clear()`.
- Pushes, discards, clears, and pops undo snapshots.
- Applies undo snapshots by installing a document through the facade.
- Writes `CreativeDocumentUndoApplyReceipt`.

## Calls Out To / Wires Out To

- Used by app kernel, operations, save flow, frontend routing, window input, UI command frame, viewport pick, wireframe, and receipt paths.
- `CreativeWorldOperations.*` clears identity/undo during create/open/save flows.
- `UiCommandFrame.*` uses undo helpers for creative undo commands.

## Called By / Entry Points

- `CreativeActiveIdentity::clear()` and `worldActive()`.
- `creativeUndoAvailable(...)`, `creativeUndoDepth(...)`, `clearCreativeUndoStack(...)`.
- `pushCreativeUndoSnapshot(...)`, `discardCreativeUndoSnapshot(...)`, `applyLastCreativeUndoSnapshot(...)`.
- Grep proof: `rg -n "CreativeActiveIdentity|CreativeAppState|pushCreativeUndoSnapshot|applyLastCreativeUndoSnapshot|creativeUndoAvailable|worldActive\\(" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Identity clear returns all fields to the no-live-creative-world baseline.
- `worldActive()` is true when save id, world id, or document id indicates a live creative world.
- Invalid documents are not pushed onto the undo stack.
- Undo stack enforces max depth by dropping the oldest snapshot.
- Undo apply pops only after facade install is accepted.

## Tests / Proof Commands

- `product_creative_world_launch_tests`.
- `product_creative_ui_command_frame_tests`.
- `product_creative_ui_projection_tests`.
- `product_creative_no_window_bake_scenario_tests`.
- `rg -n "product_creative_world_launch_tests|product_creative_ui_command_frame_tests|creative_undo_applied|CreativeAppState" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/Facade.*` unless document install or facade ownership changes.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless launch/open/save identity policy changes.
- `src/app/iggy3d/save/Flow.*` unless save flow needs creative identity changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless undo command behavior changes.

## Update When

- Active identity fields, world-active rules, undo stack behavior, undo receipt semantics, or app-scoped creative ownership changes.

## Do Not Update When

- Only document mutation mechanics, UI row labels, or product receipt formatting changes without changing app state or undo/identity rules.
