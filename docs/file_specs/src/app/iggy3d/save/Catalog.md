# File Spec

Files: `src/app/iggy3d/save/Catalog.hpp`, `src/app/iggy3d/save/Catalog.cpp`

Verified at: `9a81866f`

## Owns

- Product-facing save catalog rows, content-kind labels, display-title fallback, active/deleted counting, row sorting, product continue selection, and creative world continue selection.
- Eligibility helpers for product-session load and creative-document open.

## Does Not Own

- Filesystem scanning, save decoding, runtime compatibility checks, durable write/delete/recover, menu confirmation flow, or save-slot rendering.
- Runtime save schema or codec behavior.

## Reads

- `ProductSaveCatalogEntry` fields projected by app save bridge: location, content kind, timestamps, compatibility/loadable/corrupt flags, creative document facts, deleted/recoverable state, snapshot facts, and disabled reason.

## Writes / Mutates

- Returns sorted catalog snapshots and selection result packets.
- Does not mutate files, sessions, frontend state, or creative documents.

## Calls Out To / Wires Out To

- Used by `SaveBridge.*` after runtime save scan/decode.
- Used by frontend save-slot model/tests and product menu/continue flows through save bridge/action handlers.

## Called By / Entry Points

- `productSaveCatalogLocationName(...)`.
- `productSaveContentKindName(...)`.
- `productSaveDisplayTitle(...)`.
- `canLoadProductSave(...)`.
- `canOpenCreativeWorld(...)`.
- `buildProductSaveCatalog(...)`.
- `sortProductSaveCatalogEntries(...)`.
- `selectProductContinueSave(...)`.
- `selectCreativeWorldContinueSave(...)`.
- Grep proof: `rg -n "ProductSaveCatalog|selectProductContinueSave|selectCreativeWorldContinueSave|canLoadProductSave|canOpenCreativeWorld" src tests`.

## Invariants

- Product continue must ignore creative-document saves.
- Creative world continue must ignore product-session saves and deleted saves.
- Deleted/recoverable rows remain representable for recovery surfaces.
- Compatibility/loadability are row facts supplied by scanning/bridge code; this file selects from them, it does not prove them.
- Sorting must remain deterministic for timestamp ties.

## Tests / Proof Commands

- `rg -n "product_save_catalog_tests|save_slot_model_tests|product_save_bridge_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creativeSaveIsIgnoredByProductContinue|deletedSaveIsIgnoredByContinue|selectCreativeWorldContinueSave" tests/unit/product_save_catalog_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless catalog projection/scan inputs change.
- `src/app/frontend/*` unless visible save-slot selection state changes.
- `src/runtime/save/*` unless save record facts exposed to catalog change.

## Update When

- Catalog row fields, content-kind eligibility, sorting, display title fallback, active/deleted counts, or product/creative continue selection changes.

## Do Not Update When

- Only durable file movement, save text format, frontend draw layout, or confirmation dialog behavior changes.
