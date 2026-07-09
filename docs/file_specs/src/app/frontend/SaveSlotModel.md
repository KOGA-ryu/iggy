# File Spec

Files: `src/app/frontend/SaveSlotModel.hpp`, `src/app/frontend/SaveSlotModel.cpp`

Verified at: `d9665a0c`

## Owns

- Frontend save slot preview packet.
- Save slot compatibility enum and stable compatibility names.
- Save slot list packet with compatible and corrupt counts.

## Does Not Own

- Save catalog scanning, runtime save decode/load, save browser ring/action model, delete execution, snapshot file generation, or UI drawing.

## Reads

- `SaveSlotCompatibility` values for naming.
- Callers populate `SaveSlotPreview` from save catalog/runtime data.

## Writes / Mutates

- No mutation; `saveSlotCompatibilityName(...)` returns stable names.

## Calls Out To / Wires Out To

- `CatalogProjector.*` builds previews from product save catalog entries.
- `SaveBrowser.*`, save slot operations, product session launch, and menu/draw-list paths consume the list and preview packets.

## Called By / Entry Points

- `saveSlotCompatibilityName(...)` and direct `SaveSlotPreview`/`SaveSlotList` construction.
- Grep proof: `rg -n "SaveSlotPreview|SaveSlotList|saveSlotCompatibilityName" src/app tests/unit`.

## Invariants

- Preview packets are frontend catalog presentation facts, not runtime save truth.
- Compatibility names are receipt/UI-facing strings.
- Corrupt/disabled state must be represented explicitly with a reason.
- Snapshot availability/fallback/status are presentation facts and do not imply save validity.

## Tests / Proof Commands

- `rg -n "save_slot_model_tests|save_browser_tests|product_save_bridge_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "saveSlotCompatibilityName|SaveSlotPreview|SaveSlotList" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/frontend/SaveBrowser.*` unless browser/ring model changes.
- `src/app/iggy3d/save/CatalogProjector.*` unless preview projection changes.
- `src/app/iggy3d/save/SaveSlotOperations.*` unless selection/delete operations change.

## Update When

- Preview fields, list fields, compatibility values, or compatibility names change.

## Do Not Update When

- Only save catalog scan/load implementation changes without changing preview/list contracts.
