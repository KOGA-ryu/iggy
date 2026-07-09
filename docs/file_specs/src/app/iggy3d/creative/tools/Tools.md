# File Spec

Files: `src/app/iggy3d/creative/tools/Tools.hpp`, `src/app/iggy3d/creative/tools/Tools.cpp`

Verified at: `ff909047`

## Owns

- Creative tool input packet schema and dispatch receipts.
- Pointer/button/modifier/world-destination packets for creative tools.
- Active tool state, measurement-active state, and move-drag state.
- Dispatch from pointer/cancel input kinds into tool intents.

## Does Not Own

- Document mutation execution.
- Selection state mutation.
- Measurement, ghost, snap, viewport pick, or placement math.
- Product window/input coordinate conversion.
- UI command rows or keyboard shortcut policy.

## Reads

- Caller-supplied `CreativeToolInputPacket`.
- Current `CreativeToolState`.
- `Tool` values from creative core.

## Writes / Mutates

- Mutates caller-owned `CreativeToolState`.
- Emits `CreativeToolIntent` rows in dispatch receipts.
- Clears measurement and move-drag state on tool switch, release, or cancel.

## Calls Out To / Wires Out To

- Consumed by `creative::Facade::dispatchToolInput(...)`.
- Product creative input bridge builds packets consumed here.
- Downstream facade dispatch routes emitted intents into selection, ghost, measurement, and move mutation handling.

## Called By / Entry Points

- `makeDefaultCreativeToolState()`.
- `setActiveTool(...)`.
- `dispatchToolInput(...)`.
- Grep proof: `rg -n "dispatchToolInput|setActiveTool|CreativeToolInputPacket|CreativeToolIntent" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Unknown input remains unsupported and emits no intent.
- Navigate pointer input is accepted but inert for document mutation.
- Move press selects and begins a drag; held move emits previews; release commits one move intent; cancel discards the drag.
- Tool switches abandon in-flight measurement and move drag state.
- Receipt `emittedIntentCount` must match the intent vector size.

## Tests / Proof Commands

- `creative_tools_tests`.
- `creative_facade_tests`.
- `product_creative_input_frame_tests`.
- `product_creative_move_drag_frame_tests`.
- `rg -n "creative_tools_tests|creative_facade_tests|product_creative_input_frame_tests|product_creative_move_drag_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/Facade.*` unless intent routing changes.
- `src/app/iggy3d/creative/bridge/InputFrame.*` unless packet construction changes.
- `src/app/iggy3d/creative/tools/Select.*` unless selection-intent semantics change.
- `src/app/iggy3d/creative/tools/Measure.*` unless measurement intent lifecycle changes.

## Update When

- Tool state fields, input/intent enums, move-drag lifecycle, receipt semantics, or active-tool switching behavior changes.

## Do Not Update When

- Only facade mutation behavior or UI command presentation changes after intents are emitted.
