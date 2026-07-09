# File Spec

Files: `src/app/iggy3d/receipt/ReceiptFields.hpp`, `src/app/iggy3d/receipt/ReceiptFields.cpp`

Verified at: `cd02b03e`

## Owns

- Public declaration surface for product receipt domain appenders.
- Shared fixed precision float-to-receipt string formatting helper.
- Receipt-domain dependency boundary used by `ReceiptBuilder.cpp`.

## Does Not Own

- The top-level receipt build order.
- Field key tables inside each domain appender.
- Runtime, save, creative, or rendering state ownership.
- Receipt parsing in tests and smokes.

## Reads

- Float values passed to `floatReceiptValue(...)`.
- Domain appender inputs declared in this header: frontend/settings/window/save/creative/debug/render readiness packets.

## Writes / Mutates

- Appenders declared here mutate caller-provided `RenderReceipt`.
- `floatReceiptValue(...)` returns a formatted string and does not mutate external state.

## Calls Out To / Wires Out To

- Domain appenders implemented by the sibling files under `src/app/iggy3d/receipt/`.
- `std::to_chars(...)` for float formatting.

## Called By / Entry Points

- `src/app/iggy3d/ReceiptBuilder.cpp` includes this header and calls the domain appenders.
- Receipt domain files include this header to define their appender implementations.
- Focused proof: `rg -n "appendProduct.*Fields|floatReceiptValue|ReceiptFields\\.hpp" src/app tests/unit`.

## Invariants

- This header is a declaration seam; field-table ownership stays in domain `.cpp` files.
- Float receipt formatting stays deterministic and fixed precision.
- Appenders append to an existing receipt and must not allocate ownership of product state.
- Adding a new domain appender requires updating the top-level receipt builder deliberately.

## Tests / Proof Commands

- `rg -n "product_receipt_key_order_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_command_receipt_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "floatReceiptValue|appendProduct.*Fields" src/app/iggy3d/receipt src/app/iggy3d/ReceiptBuilder.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ReceiptBuilder.*` unless receipt build order changes.
- Domain receipt appender files unless their field ownership changes.
- Smoke receipt parsers unless output contract changes.

## Update When

- Appender declarations, shared receipt formatting behavior, or receipt domain boundary changes.

## Do Not Update When

- Only a field table inside one domain appender changes without changing this shared declaration surface.
