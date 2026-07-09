# File Spec

Files: `src/app/iggy3d/receipt/TailFields.cpp`

Verified at: `0fe74f9c`

## Owns

- Receipt field emission for trailing frame/window/save/template summary fields.
- Tail fields for event polling, requested/presented frames, window status, save root/counts, selected package/scenario, world template source, and dev package override flag.

## Does Not Own

- Save catalog scanning.
- Window frame presentation.
- Product world template resolution.
- Top-level final `result` and `reason_code` fields.

## Reads

- `ProductAppOptions`, `ProductWorldTemplate`, `ProductAppWindowState.frontendShell`, and `ProductSaveBridgeResult`.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate options, world template, window state, or save bridge results.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductTailFields(...)` before final result fields.
- Focused proof: `rg -n "appendProductTailFields|frames_presented|compatible_save_count|selected_package_id|normal_package_cli" src/app tests`.

## Invariants

- Tail fields summarize already-produced state and must not perform scanning or frame presentation.
- Save root and compatible save count come from the save bridge result.
- Final product receipt result fields remain in `ReceiptBuilder.cpp`, not this tail appender.
- `normal_package_cli` remains a fixed receipt fact until the CLI path actually becomes distinct.

## Tests / Proof Commands

- `rg -n "compatible_save_count|selected_package_id|frames_presented|normal_package_cli" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_receipt_key_order_tests|product_ascii_package_smoke|product_save_load_smoke|product_startup_lifecycle_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless save bridge summary fields change.
- `src/app/iggy3d/world/DefaultWorldTemplate.*` unless world template fields change.
- `src/app/iggy3d/window/*` unless frontend shell frame/window proof fields change.

## Update When

- Tail receipt keys, source summary fields, or final receipt ordering around tail/result changes.

## Do Not Update When

- Only save scan internals, frame rendering internals, or package loading behavior changes without changing tail receipt fields.
