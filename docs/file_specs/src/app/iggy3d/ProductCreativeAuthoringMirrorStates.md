# File Spec

Files: `src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp`, `src/app/iggy3d/ProductCreativeUiInputState.hpp`, `src/app/iggy3d/ProductCreativeUiLastState.hpp`, `src/app/iggy3d/ProductCreativeUiProjectionState.hpp`, `src/app/iggy3d/ProductCreativeUndoState.hpp`

Verified at: `0996a5ee`

## Owns

- App-side creative authoring mirror packets embedded by `CreativeAuthoringStore`.
- Document revision observation fields for before/after frame proof.
- Creative UI input routing, hit, consumed, downstream-click, and semantic-id proof fields.
- Last creative UI click/command proof fields.
- Creative UI projection readiness, virtual size, theme, row/count, and hit-region proof fields.
- Creative undo availability/depth mirror fields.

## Does Not Own

- Creative document truth or undo stack implementation.
- Creative UI model/projection/input/command execution behavior.
- Window input routing policy.
- Receipt field emission.
- Runtime session, active room, or render backend state.

## Reads

- These headers define packets only.
- Readers include creative authoring store, input frame, creative UI bridge, receipt appenders, and focused creative tests.

## Writes / Mutates

- No functions in these headers mutate state.
- Producers write instances in window/input/projection/creative world operation paths.

## Calls Out To / Wires Out To

- Included by `CreativeAuthoringStore.hpp`.
- `InputFrame.cpp`, creative UI bridge code, creative world operations, and receipt recording write or read packet fields.
- `CreativeUiFields.cpp` emits the mirrored fields into receipts.

## Called By / Entry Points

- `ProductCreativeDocumentRevisionState`.
- `ProductCreativeUiInputState`.
- `ProductCreativeUiLastState`.
- `ProductCreativeUiProjectionState`.
- `ProductCreativeUndoState`.
- Focused proof: `rg -n "creativeDocumentRevision|creativeUiInput|creativeUiLast|creativeUiProjection|creativeUndo" src/app tests`.
- Coverage proof: `rg -n "ProductCreativeDocumentRevisionState|ProductCreativeUiInputState|ProductCreativeUiLastState|ProductCreativeUiProjectionState|ProductCreativeUndoState" src/app docs/file_specs`.

## Invariants

- These packets are app/window observability mirrors, not creative model truth.
- Default status strings must stay receipt-friendly and distinguish not-requested states.
- Field additions must identify the producer and the receipt consumer.
- Undo depth and revision fields must mirror creative app/document state without becoming the owner of that state.
- UI input and projection mirrors must not suppress downstream click or hit-region ownership rules.

## Tests / Proof Commands

- `rg -n "product_creative_ui_input_frame_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_window_frame_tests|product_creative_world_launch_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_ui_input_status|creative_ui_projection_status|creative_ui_last_click_seen|creative_undo_depth" src/app/iggy3d/receipt tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp` unless embedding changes.
- `src/app/iggy3d/window/InputFrame.*` unless input mirror production changes.
- `src/app/iggy3d/creative/bridge/*` unless creative UI bridge packet production changes.
- `src/app/iggy3d/receipt/CreativeUiFields.*` unless receipt keys change.
- `src/app/iggy3d/creative/CreativeAppState.hpp` unless creative undo source truth changes.

## Update When

- Any mirrored packet field, default value, writer ownership, receipt-facing status, or app-side mirror boundary changes.

## Do Not Update When

- Only creative document internals, UI layout, or command execution behavior changes without changing these mirror packet contracts.
