# File Spec

Files: `src/app/iggy3d/automation/AutomationSaveBrowser.hpp`, `src/app/iggy3d/automation/AutomationSaveBrowser.cpp`

Verified at: `49d04797`

## Owns

- Save-browser automation handler for selecting a save slot, opening delete confirmation, opening deleted-save browser, selecting a deleted save, and executing recover.
- Load-save child-screen guard for save browser automation commands.
- Bool parsing policy for save-browser commands and invalid-value status recording for save selection ids.

## Does Not Own

- Save catalog scanning implementation.
- Save delete executor semantics.
- Save recovery filesystem behavior.
- Frontend load/save/delete confirmation UI rendering.
- Product continue/load selection rules.

## Reads

- `FrontendState.childScreen`.
- `ProductAppOptions` for deleted-save scan/recover operations.
- `ProductSaveBridgeResult` slots and deleted-save scan results.
- `ProductAppWindowState.saveSession` selection, deleted-browser, delete, recover, and automation-control state.

## Writes / Mutates

- Mutates save slot selection through `selectProductSaveSlotById(...)`.
- Mutates delete confirmation state through `openProductSaveDeleteConfirmation(...)`.
- Mutates deleted-save browser state through `openDeletedProductSaveBrowser(...)`, `recordDeletedProductSaveSlots(...)`, and `selectDeletedProductSaveSlotById(...)`.
- Mutates save recover state through `executeProductSaveRecover(...)`.
- Mutates automation-control status and last command proof through `markAutomationApplied(...)`.

## Calls Out To / Wires Out To

- `resolveProductSaveBrowserBoolAutomation(...)` and `resolveProductSaveSelectionAutomation(...)`.
- `selectProductSaveSlotById(...)`.
- `openProductSaveDeleteConfirmation(...)`.
- `openDeletedProductSaveBrowser(...)`.
- `scanDeletedProductSavesForOptions(...)`.
- `recordDeletedProductSaveSlots(...)`.
- `selectDeletedProductSaveSlotById(...)`.
- `executeProductSaveRecover(...)`.

## Called By / Entry Points

- `AutomationDispatch.cpp` calls `applyProductSaveBrowserAutomationCommand(...)`.
- Save/delete/recover smokes drive this handler through automation command files.
- Focused proof: `rg -n "applyProductSaveBrowserAutomationCommand|save.select|save.delete|save.show_deleted|save.deleted_select|save.recover" src/app tests`.

## Invariants

- Save-browser automation only runs when `frontend.childScreen` is `LoadSave`.
- Deleted-save select and recover require the deleted-save browser to be open.
- False bool values are ignored, not executed.
- Empty save ids are invalid values.
- Delete automation opens confirmation; confirmation execution is owned by the frontend/menu route, not this file.
- Recover automation records success/failure from `window.saveSession.saveRecover.executed`.

## Tests / Proof Commands

- `rg -n "product_save_delete_recover_smoke|product_save_delete_executor_tests|product_automation_command_registry_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "save.select=|save.delete=true|save.show_deleted=true|save.deleted_select=|save.recover=true" tests/smoke/product_save_delete_recover_smoke.cpp`.
- `rg -n "save_delete_status|save_recover_status|automationCommandFailed" tests/smoke/product_save_delete_recover_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless save delete/recover execution changes.
- `src/app/iggy3d/save/SaveSlotOperations.*` unless selection or confirmation state changes.
- `src/app/frontend/SaveBrowser.*` unless load-save child-screen model changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless confirmation execution routing changes.

## Update When

- Save browser automation commands, child-screen guards, selection semantics, deleted-browser behavior, delete confirmation opening, or recover execution routing changes.

## Do Not Update When

- Only save codec, catalog ordering, or filesystem store internals change without changing save-browser automation behavior.
