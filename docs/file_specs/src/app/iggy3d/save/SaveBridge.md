# File Spec

Files: `src/app/iggy3d/save/SaveBridge.hpp`, `src/app/iggy3d/save/SaveBridge.cpp`

Verified at: `6c79462b`

## Owns

- Product-facing save bridge result/request packets for scan, product session write/load, creative document write/load, soft delete, recover, and mutation refresh.
- Product save catalog scanning from active and deleted save roots.
- Catalog entry projection from runtime save envelope/file facts.
- World ID minting and UTC timestamp label creation.
- Durable product session and creative document save/load orchestration over runtime save APIs.

## Does Not Own

- Runtime save text format, durable file-store primitives, or state hash calculation.
- Product menu selection flow or confirmation UI.
- Creative document mutation rules.
- Snapshot image production.

## Reads

- Save root paths, package/scenario filters, runtime save envelopes, session state, creative document save data, and existing save metadata.
- Runtime save file-store and codec/load results.

## Writes / Mutates

- Writes product session and creative document save files through runtime save file-store APIs.
- Soft-deletes and recovers save files through runtime save file-store APIs.
- Returns refreshed `ProductSaveBridgeResult` catalog snapshots after mutation.
- Does not mutate frontend/window state directly.

## Calls Out To / Wires Out To

- `runtime/save/SaveFileStore.*`, `SaveCodec.*`, `SaveLoad.*`, and save compatibility helpers.
- `src/app/iggy3d/save/Catalog.*` for product catalog construction/selection facts.
- Creative document save-section conversion and runtime session load/write seams.

## Called By / Entry Points

- `AppKernel.cpp`, product world creation/launch code, creative world service, menu action handlers, current-session save, save slot operations, automation, and tests.
- Grep proof: `rg -n "scanProductSaves|writeProductSessionSaveDurably|loadProductSessionSave|writeCreativeDocumentSaveDurably|softDeleteProductSaveAndRefresh|recoverProductSaveAndRefresh" src tests cmake`.

## Invariants

- Product catalog compatibility and runtime load compatibility are separate checks.
- Soft delete and recover must refresh active/deleted catalog snapshots.
- Creative and product session save paths share bridge scanning but keep content-kind eligibility distinct.
- Bridge code may perform filesystem I/O; callers/tests should use explicit save roots.
- Save bridge must not become a UI flow owner.

## Tests / Proof Commands

- `rg -n "product_save_bridge_tests|product_save_delete_executor_tests|product_save_catalog_tests|creative_world_service_tests|save_slot_model_tests" cmake tests`.
- `rg -n "writeProductSessionSaveDurably|writeCreativeDocumentSaveDurably|softDeleteProductSaveAndRefresh|recoverProductSaveAndRefresh" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/save/*` unless runtime save format/file-store contracts change.
- `src/app/iggy3d/save/Catalog.*` unless catalog projection/eligibility changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless menu save/delete execution changes.
- `src/app/iggy3d/creative/world/WorldService.*` unless creative save/open behavior changes.

## Update When

- Save bridge request/result packets, scan filters, catalog projection, durable write/load, soft-delete/recover, or world-id/timestamp contracts change.

## Do Not Update When

- Only frontend UI labels or confirmation screens change without bridge API/behavior changes.
- Runtime save codec internals change without changing bridge-visible behavior.
