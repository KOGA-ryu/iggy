# File Spec

Files: `src/app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp`

Verified at: `e9c2b0d9`

## Owns

- Product creative UI command diagnostics packets.
- Nested diagnostics for mutation, create, delete, undo, room shell, and baked-room refresh command outcomes.
- Default not-requested and unknown status values consumed by receipt fields.

## Does Not Own

- Command routing from semantic ids.
- Command execution behavior.
- Creative document mutation, undo stack, room shell generation, or active-room bake refresh.
- Receipt field emission.

## Reads

- This header defines packets only.
- Readers include creative command frame logic, receipt appenders, input frame tests, and command receipt tests.

## Writes / Mutates

- No functions in this header mutate state.
- Command frame and receipt recording code populate instances after command execution.

## Calls Out To / Wires Out To

- Embedded by creative authoring app/window state surfaces.
- `UiCommandFrame.*` writes these diagnostics.
- `CreativeUiFields.*` and command receipt tests consume the fields.

## Called By / Entry Points

- `ProductCreativeUiCommandDiagnostics`.
- `ProductCreativeUiCommandMutationDiagnostics`.
- `ProductCreativeUiCommandCreateDiagnostics`.
- `ProductCreativeUiCommandDeleteDiagnostics`.
- `ProductCreativeUiCommandUndoDiagnostics`.
- `ProductCreativeUiCommandRoomShellDiagnostics`.
- `ProductCreativeBakedRoomRefreshDiagnostics`.
- Focused proof: `rg -n "ProductCreativeUiCommandDiagnostics|creativeUiCommand|creative_ui_command_" src/app tests`.

## Invariants

- Defaults must represent not-requested or unknown command state without implying execution.
- Nested diagnostics mirror source command receipts; they do not own the underlying command semantics.
- Revision, dirty flag, object id, and object kind fields are observability mirrors.
- Baked-room refresh diagnostics intentionally mirror only product-facing bake/active-room proof fields.

## Tests / Proof Commands

- `rg -n "product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests|product_creative_ui_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_ui_command_status|creative_ui_command_create_status|creative_ui_command_undo_status|creative_ui_command_baked_room_refresh_status" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless diagnostic population changes.
- `src/app/iggy3d/receipt/CreativeUiFields.*` unless receipt keys change.
- `src/app/iggy3d/creative/CreativeAppState.hpp` unless undo receipt source changes.
- `src/app/iggy3d/ProductCreativeBakedRoomRefresh.hpp` unless baked refresh packet fields change.

## Update When

- Any command diagnostics field, default value, writer ownership, nested receipt mapping, or receipt-facing semantic changes.

## Do Not Update When

- Only UI row labels, command catalog entries, or creative document internals change without changing diagnostic packet contracts.
