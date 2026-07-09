# File Spec

Files: `src/app/iggy3d/save/SaveSlotOperations.hpp`, `src/app/iggy3d/save/SaveSlotOperations.cpp`

Verified at: `158627a3`

## Owns

- Product save-slot selection recording and movement.
- Deleted-save browser open, selection recording, and recover flow.
- Delete confirmation open/cancel and soft-delete flow execution.
- Product save flow request/result proof packets.

## Does Not Own

- Save catalog scanning implementation, save file mutation implementation, save browser model construction internals, frontend draw-list layout, or runtime session launch.

## Reads

- `SaveSlotList`, selected ids in `ProductAppWindowState.saveSession`, frontend save browser mode, app save options, and product world template ids.
- Mutation results from active/deleted save scan, recover, and soft-delete APIs.

## Writes / Mutates

- Mutates `ProductAppWindowState.saveSession` selection, ring, deleted-save, recover, delete, action, and flow proof fields.
- Mutates `FrontendState` child screen, selected action, save browser mode, and status.
- Updates caller-owned `ProductSaveBridgeResult& saves` after successful soft delete.

## Calls Out To / Wires Out To

- `buildSaveSlotRingModel(...)` and `nextSaveSlotRingSelection(...)`.
- `scanDeletedProductSaves(...)`, `recoverProductSaveAndRefresh(...)`, and `softDeleteProductSaveAndRefresh(...)`.
- `productWorldTemplateFromOptions(...)` for package/scenario context.

## Called By / Entry Points

- Product action handlers, automation save browser commands, product session launch, and window input frame call selection/delete/recover helpers.
- Grep proof: `rg -n "initializeSelectedProductSaveSlot|moveSelectedProductSaveSlot|selectProductSaveSlotById|openDeletedProductSaveBrowser|executeProductSaveRecover|openProductSaveDeleteConfirmation|cancelProductSaveDeleteConfirmation|executeProductSaveSoftDelete" src/app tests/unit`.

## Invariants

- Selection recording must update both selected slot proof and ring proof.
- Empty, missing, disabled, and selected slot states must stay distinguishable.
- Delete confirmation must open before `executeProductSaveSoftDelete(...)` accepts a candidate.
- Successful soft delete refreshes active/deleted catalogs and reclamps selection.
- Recover and delete flows update frontend status and save browser mode proof.

## Tests / Proof Commands

- `rg -n "product_save_delete_executor_tests|save_browser_tests|product_save_bridge_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "executeProductSaveSoftDelete|openProductSaveDeleteConfirmation|executeProductSaveRecover|selectProductSaveSlotById" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/frontend/SaveBrowser.*` unless ring/action model changes.
- `src/app/iggy3d/save/SaveBridge.*` unless scan/mutation result APIs change.
- `src/app/iggy3d/menu/ActionHandlers.*` unless user-facing route wiring changes.

## Update When

- Selection recording, delete/recover flow fields, frontend status transitions, catalog refresh behavior, or save flow proof contracts change.

## Do Not Update When

- Only lower-level save file mutation internals change without changing operation contracts.
