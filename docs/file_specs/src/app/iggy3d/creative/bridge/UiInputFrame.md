# File Spec

Files: `src/app/iggy3d/creative/bridge/UiInputFrame.hpp`, `src/app/iggy3d/creative/bridge/UiInputFrame.cpp`

Verified at: `d40a476b`

## Owns

- Creative overlay click-to-hit routing receipt.
- Creative UI downstream click suppression or passthrough decision.
- Copying hit route fields into creative UI input receipts.

## Does Not Own

- Creative UI projection/draw-list construction.
- Semantic command execution.
- Window coordinate conversion.
- Viewport pick, document mutation, or facade state changes.

## Reads

- Optional `ProductUiDrawList` pointer and `MouseClick`.
- Higher-priority and creative-UI consumed flags for downstream click routing.

## Writes / Mutates

- No external state.
- Returns `ProductCreativeUiInputFrameReceipt` and `ProductCreativeUiDownstreamClickReceipt`.

## Calls Out To / Wires Out To

- `routeProductUiHit(...)` with a single `CreativeOverlay` hit layer.
- Downstream click receipts consumed by window input routing and viewport pick.

## Called By / Entry Points

- `window/InputFrame.cpp` routes creative overlay clicks through this file before command execution and viewport pick.
- UI input and pick-flow tests call it directly.
- Grep proof: `rg -n "routeProductCreativeUiInputFrame|routeProductCreativeUiDownstreamClick" src tests cmake`.

## Invariants

- No click returns a no-click receipt and does not route hit regions.
- Missing draw list returns draw-list-missing.
- Not-ready draw lists can still route fields but report not-ready.
- Consumed creative UI clicks suppress downstream viewport clicks.
- Higher-priority UI consumption wins over creative overlay consumption.

## Tests / Proof Commands

- `rg -n "product_creative_ui_input_frame_tests|product_creative_pick_flow_tests" cmake tests`.
- `rg -n "product_creative_ui_input_consumed|product_creative_ui_downstream_click" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/UiHitRouter.*` unless generic hit routing changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless command dispatch consumes different receipt fields.
- `src/app/iggy3d/window/InputFrame.cpp` unless frame ordering changes.

## Update When

- Creative UI input receipt fields, hit-layer routing, consumed/miss/not-ready statuses, or downstream click suppression rules change.

## Do Not Update When

- Only a semantic command handler changes behind the same input receipt.
