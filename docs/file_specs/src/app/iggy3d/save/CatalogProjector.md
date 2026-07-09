# File Spec

Files: `src/app/iggy3d/save/CatalogProjector.hpp`, `src/app/iggy3d/save/CatalogProjector.cpp`

Verified at: `158627a3`

## Owns

- Projection from product save catalog entries to frontend save slot preview/list packets.
- Compatibility classification from catalog-entry facts.
- Deleted-entry recoverability/loadability rules for slot enablement.

## Does Not Own

- Filesystem save scanning, runtime save decode/load, save browser ring modeling, delete/recover execution, or UI drawing.

## Reads

- `ProductSaveCatalogEntry` fields: save id, path, package/scenario, tick, hash, authored-room counts, corrupt/loadable/compatible flags, disabled reason, deleted/recoverable flags, display title, timestamp, and snapshot fields.

## Writes / Mutates

- No mutation; returns `SaveSlotCompatibility`, `SaveSlotPreview`, and `SaveSlotList`.

## Calls Out To / Wires Out To

- Uses `canLoadProductSave(...)`, `productSaveDisplayTitle(...)`, and `saveSlotCompatibilityName(...)`.
- `SaveBridge.*` calls `buildSaveSlotListFromCatalog(...)`.
- Save slot operations and frontend save browser consume the projected list.

## Called By / Entry Points

- `saveSlotCompatibilityFromCatalogEntry(...)`, `saveSlotPreviewFromCatalogEntry(...)`, and `buildSaveSlotListFromCatalog(...)`.
- Grep proof: `rg -n "saveSlotCompatibilityFromCatalogEntry|saveSlotPreviewFromCatalogEntry|buildSaveSlotListFromCatalog|CatalogProjector" src/app tests/unit`.

## Invariants

- Corrupt/read/decode failures classify as decode failed.
- Incompatible package and scenario reasons have explicit compatibility values.
- Active entries are enabled through `canLoadProductSave(...)`; deleted entries are enabled when recoverable and not corrupt.
- Empty title/timestamp/snapshot fields must be normalized for frontend display.

## Tests / Proof Commands

- `rg -n "save_slot_model_tests|product_save_bridge_tests|save_browser_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "saveSlotPreviewFromCatalogEntry|buildSaveSlotListFromCatalog|saveSlotCompatibilityFromCatalogEntry" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/Catalog.*` unless catalog entry fields change.
- `src/app/frontend/SaveSlotModel.*` unless preview/list packet contracts change.
- `src/app/frontend/SaveBrowser.*` unless browser model changes.

## Update When

- Catalog-to-preview field mapping, compatibility classification, deleted/recoverable enablement, or display normalization changes.

## Do Not Update When

- Only save file scan/decode internals change without changing catalog entry or preview contracts.
