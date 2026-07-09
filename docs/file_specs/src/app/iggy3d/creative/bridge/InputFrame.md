# File Spec

Files: `src/app/iggy3d/creative/bridge/InputFrame.hpp`, `src/app/iggy3d/creative/bridge/InputFrame.cpp`

Verified at: `e9c2b0d9`

## Owns

- Product bridge from app input actions, direct creative tool keys, mouse click, and pointer lifecycle events into creative facade tool input.
- Persistent pointer lifecycle state and raw pointer sample resolution.
- Tool-key-to-creative-tool table for direct tool selection.
- Product creative input frame receipt for handled actions, tool changes, pointer dispatch, cancel dispatch, acceptance, change, and emitted intent count.

## Does Not Own

- Window-level input polling.
- Creative tool behavior or facade command execution internals.
- Viewport pick target resolution.
- UI command dispatch from semantic ids.
- Receipt field emission.

## Reads

- Product window active creative editor state.
- `creative::CreativeAppState` and facade tool state.
- `ActionState`, `KeyboardCreativeToolKeyPresses`, `MouseClick`, pointer targets, and lifecycle events.

## Writes / Mutates

- Mutates `ProductCreativePointerLifecycleState` in lifecycle resolver/reset helpers.
- Mutates the creative facade through active tool changes and `dispatchToolInput(...)`.
- Returns `ProductCreativeInputFrameReceipt`.
- Does not mutate window frontend state or raw input samples.

## Calls Out To / Wires Out To

- Calls `productCreativeDocumentEditorActiveForSource(...)`.
- Calls `Facade::setActiveTool(...)` and `Facade::dispatchToolInput(...)`.
- Converts app mouse/click/lifecycle facts into `CreativeToolInputPacket`.
- Window input and creative pick flow paths consume these helpers.

## Called By / Entry Points

- `productCreativeInputActiveForWindow(...)`.
- `productCreativeToolKeyTarget(...)`.
- `productCreativePointerPressPacket(...)`, `productCreativePointerMovePacket(...)`, `productCreativePointerReleasePacket(...)`.
- `resolveProductCreativePointerLifecycle(...)`.
- `resetProductCreativePointerLifecycle(...)`.
- `processProductCreativeInputFrame(...)`.
- `processProductCreativeInputActions(...)`.
- Focused proof: `rg -n "ProductCreativeInputFrame|processProductCreativeInputFrame|resolveProductCreativePointerLifecycle" src/app tests`.

## Invariants

- Creative input is active only on the creative document editor surface.
- Press is owned by the pick chain; lifecycle continuation emits move and release only.
- Held pointer move emits only when position changes.
- Tool key repeats do not fabricate changed receipts for the same active tool.
- Cancel, press, move, and release dispatch through the facade as tool input packets.

## Tests / Proof Commands

- `rg -n "product_creative_input_frame_tests|product_creative_pick_flow_tests|product_creative_ui_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "product_creative_input_processed|pointerMoveDispatched|pointerReleaseDispatched|toolChanged" tests/unit/product_creative_input_frame_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/InputFrame.*` unless window-to-creative input routing changes.
- `src/app/iggy3d/creative/tools/*` unless tool input packet semantics change.
- `src/app/iggy3d/creative/Facade.*` unless facade dispatch contracts change.
- `src/app/iggy3d/creative/bridge/ViewportPickFrame.*` unless pick target handoff changes.

## Update When

- Creative input request/receipt fields, tool key mapping, pointer lifecycle semantics, facade dispatch handoff, or active-surface gating changes.

## Do Not Update When

- Only individual creative tool behavior, UI command semantics, or raw platform input polling changes without changing this bridge contract.
