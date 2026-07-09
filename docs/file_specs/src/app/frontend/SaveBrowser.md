# File Spec

Files: `src/app/frontend/SaveBrowser.hpp`, `src/app/frontend/SaveBrowser.cpp`

Verified at: `158627a3`

## Owns

- Frontend save browser ring/carousel model.
- Save slot action specs for load, delete, and back.
- Selected save presentation facts: title, timestamp, snapshot availability, fallback, and status.
- Ring selection movement through `nextSaveSlotRingSelection(...)`.

## Does Not Own

- Save catalog scanning, save slot preview projection, load execution, delete/recover execution, frontend state mutation, or UI drawing.

## Reads

- `SaveSlotList`, selected save id, and `FrontendSaveBrowserMode`.
- Save slot preview fields used to build ring items and selected presentation.

## Writes / Mutates

- No mutation; returns `SaveSlotRingModel`, next selected id, or `SaveBrowserModel`.

## Calls Out To / Wires Out To

- Consumed by product save-slot operations, menu draw-list, and frontend router context.
- Uses `SaveSlotModel.*` packets as input.

## Called By / Entry Points

- `buildSaveSlotRingModel(...)`, `nextSaveSlotRingSelection(...)`, `buildSaveBrowserModel(...)`, and `saveSlotCommandName(...)`.
- Grep proof: `rg -n "SaveSlotCommand|SaveSlotRingModel|SaveSlotActionSpec|SaveBrowserModel|saveSlotCommandName|buildSaveSlotRingModel|nextSaveSlotRingSelection|buildSaveBrowserModel" src/app tests/unit`.

## Invariants

- Empty slot lists produce an empty ring and `none` selection.
- Missing selected ids fall back to the first slot.
- Delete mode exposes delete and back actions only.
- Delete actions require confirmation when a slot is present.
- Load actions require selected slot enabled; delete can target disabled/corrupt visible slots for confirmation flow.

## Tests / Proof Commands

- `rg -n "save_browser_tests|save_slot_model_tests|product_save_delete_executor_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildSaveBrowserModel|nextSaveSlotRingSelection|SaveSlotActionSpec" tests/unit/save_browser_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/frontend/SaveSlotModel.*` unless slot preview fields change.
- `src/app/iggy3d/save/SaveSlotOperations.*` unless selection/delete execution changes.
- `src/app/iggy3d/menu/DrawList.*` unless save browser drawing changes.

## Update When

- Ring selection, browser status, action specs, selected presentation fields, or save browser mode behavior changes.

## Do Not Update When

- Only catalog scanning, save mutation, or UI layout changes without changing the browser model contract.
