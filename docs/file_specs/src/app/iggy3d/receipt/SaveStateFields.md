# File Spec

Files: `src/app/iggy3d/receipt/SaveStateFields.cpp`

Verified at: `cd02b03e`

## Owns

- Receipt field emission for product save state, load state, save-slot ring/action state, delete/recover state, room editor save-adjacent state, and active creative save identity.
- Table-driven mapping from save/creative/window state packets to receipt keys.

## Does Not Own

- Save/load/delete/recover execution.
- Creative document save behavior.
- Save catalog scanning or durable save mutation.
- Top-level receipt build order.

## Reads

- `FrontendState`, `ProductAppWindowState.saveSession`, creative active identity, creative authoring room editor state, room editor cursor/overlay state, and related app/window proof packets.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate frontend, window, save bridge, or creative identity state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `floatReceiptValue(...)` for float-valued room editor fields.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductSaveStateFields(...)`.
- Focused proof: `rg -n "appendProductSaveStateFields|product_save_status|active_creative_save_id" src/app tests`.

## Invariants

- Receipt emission is a straight read of already-produced save and creative proof state.
- Empty ids should be normalized by producers, not patched in this appender.
- Save/load/delete/recover fields remain observable even when not requested.
- This appender must not trigger save catalog scans or filesystem I/O.

## Tests / Proof Commands

- `rg -n "product_save_status|active_product_save_id|save_delete_status|save_recover_status" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_receipt_key_order_tests|product_save_bridge_tests|product_save_delete_executor_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/*` unless save proof packet fields change.
- `src/app/iggy3d/creative/*` unless creative identity/save proof fields change.
- `src/app/iggy3d/ReceiptBuilder.*` unless receipt ordering changes.

## Update When

- Save-state receipt keys, source state packets, creative identity fields, or save-related receipt ordering changes.

## Do Not Update When

- Only durable save internals or save UI labels change without changing receipt fields.
