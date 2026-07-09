# File Spec

Files: `src/app/iggy3d/receipt/CreativeUiFields.cpp`

Verified at: `cd02b03e`

## Owns

- Receipt field emission for CreativeDocument UI projection, hit/input state, command diagnostics, baked-room refresh diagnostics, document revision proof, undo state, and baked-room stale proof.
- Ordered append sequence for pre-command, command, auto-refresh, and post-refresh creative UI receipt groups.

## Does Not Own

- Creative UI projection generation.
- Creative UI input routing or command execution.
- Creative document mutation rules.
- Baked-room refresh execution.

## Reads

- `ProductAppWindowState.creativeAuthoring` Creative UI projection, input, command, baked-room refresh, document revision, undo, and stale state packets.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate creative authoring state or creative documents.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- Internal helper appenders for creative command and baked-room refresh diagnostic groups.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductCreativeUiFields(...)`.
- Focused proof: `rg -n "appendProductCreativeUiFields|creative_ui_projection_ready|creative_ui_command_create_status|creative_baked_room_auto_refresh_status" src/app tests`.

## Invariants

- Creative UI receipt fields are diagnostics only and must not dispatch commands.
- Command receipt groups preserve requested, accepted, changed, status, object, revision, and reason facts.
- Baked-room refresh fields keep command-triggered and automatic refresh diagnostics distinct.
- Projection readiness and hit-region facts remain visible even when no command is accepted.

## Tests / Proof Commands

- `rg -n "product_creative_ui_projection_receipt_tests|product_creative_ui_command_receipt_tests|product_creative_ui_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_ui_projection_ready|creative_ui_command_create_status|creative_baked_room_auto_refresh_status" tests/unit src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/*` unless projection proof fields change.
- `src/app/iggy3d/creative/bridge/*` unless command/input diagnostics change.
- `src/app/iggy3d/ReceiptBuilder.*` unless receipt ordering changes.

## Update When

- Creative UI receipt keys, command diagnostic packets, baked-room refresh diagnostics, or creative UI receipt grouping changes.

## Do Not Update When

- Only CreativeDocument mutation internals or UI visual layout changes without changing emitted receipt fields.
