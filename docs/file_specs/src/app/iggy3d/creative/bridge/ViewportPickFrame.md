# File Spec

Files: `src/app/iggy3d/creative/bridge/ViewportPickFrame.hpp`, `src/app/iggy3d/creative/bridge/ViewportPickFrame.cpp`

Verified at: `5ad31d28`

## Owns

- Product bridge request/receipt for routing a creative viewport click into a spatial pick.
- Active creative editor gate for viewport picking.
- Projection of creative document objects to grid cells before pick routing.
- Receipt status and copied pick target facts for object id, kind, occupancy, grid coordinate, grid index, and cell index.

## Does Not Own

- Creative document object storage.
- Spatial projection kernel internals.
- Viewport pick math.
- UI hit-region routing or downstream click priority decisions.
- Receipt field emission.

## Reads

- Product window active creative editor state.
- `creative::CreativeAppState` and its `Facade` document.
- Mouse click packet, viewport request, projection request, fixed-depth options, and downstream suppression flag.

## Writes / Mutates

- Returns `ProductCreativeViewportPickFrameReceipt`.
- Does not mutate window state, creative app state, document objects, or input state.

## Calls Out To / Wires Out To

- Calls `productCreativeDocumentEditorActiveForSource(...)`.
- Calls `creative::projectObjectsToGrid(...)`.
- Calls `creative::pickCreativeViewportCell(...)`.
- Receipt recording copies this frame receipt into product window creative authoring state.

## Called By / Entry Points

- `productCreativeViewportPickActiveForWindow(...)`.
- `routeProductCreativeViewportPickFrame(...)`.
- Focused proof: `rg -n "ProductCreativeViewportPickFrame|routeProductCreativeViewportPickFrame|creative_viewport_pick" src/app tests`.

## Invariants

- Missing window, inactive creative surface, no click, suppressed click, missing facade, empty source, empty projection, and pick miss each produce explicit statuses.
- Click suppression is honored before facade/document access.
- The picker can still run with empty projection cells only for invalid grid or viewport inputs so validation statuses stay observable.
- This bridge reports pick facts; it must not mutate selection or creative document state.

## Tests / Proof Commands

- `rg -n "product_creative_viewport_pick_frame_tests|product_creative_pick_flow_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routeProductCreativeViewportPickFrame|product_creative_viewport_pick_hit|product_creative_viewport_pick_click_suppressed" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/spatial/SpatialProjection.*` unless projection input/output changes.
- `src/app/iggy3d/creative/spatial/ViewportPick.*` unless pick semantics change.
- `src/app/iggy3d/receipt/CreativeReceiptRecording.*` unless receipt recording changes.
- `src/app/iggy3d/window/InputFrame.*` unless click routing order changes.

## Update When

- Viewport pick request/receipt fields, active gating, suppression policy, projection handoff, pick status mapping, or receipt-facing semantics change.

## Do Not Update When

- Only creative selection behavior, document mutation, or lower-level pick math changes without changing this bridge contract.
