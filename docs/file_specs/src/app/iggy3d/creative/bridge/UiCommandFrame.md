# File Spec

Files: `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`, `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`

Verified at: `d40a476b`

## Owns

- Creative UI semantic-command dispatch from an input hit receipt to facade/document operations.
- `ProductCreativeUiCommandFrameReceipt` aggregation for tool selection, visibility/lock mutation, create, delete, undo, and room-shell generate/remove commands.
- Command kind handler availability.

## Does Not Own

- UI hit testing or click coordinate conversion.
- Creative UI projection/draw-list construction.
- Durable save/load, launch/open policy, or renderer presentation.
- Core document validation beyond invoking facade/document APIs.

## Reads

- `creative::CreativeAppState`, input receipt fields, semantic id, command catalog rows, facade document/tool/undo state, and selected object facts.

## Writes / Mutates

- Facade active tool, selected object visibility/lock, document create/remove/install, undo stack state, and room-shell generated document content through facade/document APIs.
- Returns a receipt with command-specific proof fields.

## Calls Out To / Wires Out To

- `creativeUiCommandForSemanticId(...)`.
- `Facade::setActiveTool(...)`, toggle/create/remove/install APIs, undo helpers, and room-shell build/remove helpers.

## Called By / Entry Points

- `window/InputFrame.cpp` calls `routeProductCreativeUiCommandFrame(...)` after creative UI input routing.
- Receipt builders and tests consume command receipt fields.
- Grep proof: `rg -n "routeProductCreativeUiCommandFrame|productCreativeUiCommandKindHasHandler|creativeUiCommandForSemanticId" src tests cmake`.

## Invariants

- Command routing requires a facade and a consumed enabled input hit.
- Unknown or unhandled semantic ids return receipt status instead of mutating state.
- Command receipts preserve before/after tool, mutation, revision, object, undo, and room-shell facts for no-window proof.
- Room-shell batch changes use staged document/install behavior rather than partial live mutation.

## Tests / Proof Commands

- `rg -n "product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests|product_creative_pick_flow_tests|product_creative_wireframe_frame_tests" cmake tests`.
- `rg -n "GenerateSelectedRoomShell|RemoveSelectedRoomShell|creative_undo" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/UiCommandCatalog.*` unless semantic command mapping changes.
- `src/app/iggy3d/creative/Facade.*` unless mutation wrappers change.
- `src/app/iggy3d/creative/tools/RoomShell.*` unless room-shell command behavior changes.

## Update When

- UI command receipt fields, semantic command dispatch, handler availability, facade mutation calls, undo behavior, or room-shell command wiring changes.

## Do Not Update When

- Only hit testing or window coordinate conversion changes before command dispatch.
