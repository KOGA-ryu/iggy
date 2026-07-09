# File Spec

Files: `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`

Verified at: `cd02b03e`

## Owns

- Receipt field emission for world setup, world creation, ASCII room preview, ASCII activation, room editor state, and room editing operation proof.
- Table-driven mapping from creative authoring state packets to receipt keys.

## Does Not Own

- World setup route handling.
- ASCII room parsing/authoring/activation.
- Room editor command execution.
- Save or load execution.

## Reads

- `ProductAppWindowState.creativeAuthoring` world setup, world creation, ASCII preview, ASCII activation, room editor, room editing, and operation proof fields.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate app/window authoring state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `appendProductStartupWorldBuildoutFields(...)` calls this appender as part of startup/world buildout receipt grouping.

## Called By / Entry Points

- `src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp`.
- Focused proof: `rg -n "appendProductWorldAuthoringFields|world_setup_status|ascii_room_preview_status|room_editing_last_operation" src/app tests`.

## Invariants

- World setup and world creation proof stay observable separately.
- ASCII preview/activation fields are receipt views over producer state only.
- Room editor receipt fields must not execute edit commands.
- Receipt field order is table order and should remain intentional.

## Tests / Proof Commands

- `rg -n "world_setup_status|ascii_room_preview_status|room_editing_last_operation" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_receipt_key_order_tests|product_world_setup_smoke|product_ascii_authoring_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/*` unless world setup/creation proof fields change.
- `src/app/iggy3d/ascii_room/*` unless ASCII proof fields change.
- `src/app/iggy3d/room_editor/*` unless room editor proof fields change.

## Update When

- World authoring receipt keys, source proof packets, or startup/world buildout receipt grouping changes.

## Do Not Update When

- Only ASCII parser internals, room editor UI layout, or save behavior changes without changing emitted receipt fields.
