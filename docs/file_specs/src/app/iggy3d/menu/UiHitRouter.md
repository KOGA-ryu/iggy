# File Spec

Files: `src/app/iggy3d/menu/UiHitRouter.hpp`, `src/app/iggy3d/menu/UiHitRouter.cpp`

Verified at: `0996a5ee`

## Owns

- Shared layered hit routing over `ProductUiDrawList` hit regions.
- `ProductUiHitSurface`, `ProductUiHitLayer`, `ProductUiHitRouteRequest`, and `ProductUiHitRouteReceipt`.
- Hit receipt fields for surface, action, hit kind, layer index, region index, semantic id, consumed/enabled state, and message.

## Does Not Own

- Draw-list construction for starter, pause, creative overlay, or notebook surfaces.
- Input polling, coordinate conversion, mouse capture, or focus policy.
- Frontend action execution or creative command execution.

## Reads

- Request layer span, pointer coordinates, layer enablement, draw-list readiness, and draw-list hit regions.
- `UiHitRegion` rect, action, kind, enabled flag, and semantic id.

## Writes / Mutates

- Returns a `ProductUiHitRouteReceipt`.
- Does not mutate layers, draw lists, or hit regions.

## Calls Out To / Wires Out To

- Does not call into frontend/action systems.
- Consumers use the receipt in creative UI input, opening-menu hit tests, and receipt recording.

## Called By / Entry Points

- `routeProductUiHit`.
- `src/app/iggy3d/creative/bridge/UiInputFrame.cpp` routes creative overlay clicks through this seam.
- `src/app/iggy3d/view/OpeningMenuHitTest.cpp` routes menu hit tests through this seam.
- Focused proof: `rg -n "routeProductUiHit|ProductUiHitRoute|ProductUiHitLayer|ProductUiHitSurface|ui_hit_" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Null or empty layers return requested receipt status `ui_hit_no_layers`.
- Disabled, null, or not-ready layers are skipped.
- Layers are scanned in request order; hit regions inside a draw list are scanned in reverse region order.
- Rect containment is half-open on right and bottom edges.
- Disabled hit regions still report a hit but set `consumed=false` and message `ui_hit_disabled`.
- Misses after all eligible layers return `ui_hit_miss`.

## Tests / Proof Commands

- `product_ui_hit_router_tests` covers no-layer, layer order, miss, consumed hit, disabled region, disabled layer, half-open bounds, and creative overlay receipt behavior.
- `product_creative_ui_input_frame_tests` covers the creative overlay consumer path.
- `rg -n "product_ui_hit_router_tests|product_creative_ui_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/DrawList.*` unless hit region schema changes.
- `src/app/iggy3d/creative/bridge/UiInputFrame.*` unless creative click consumption changes.
- `src/app/iggy3d/view/OpeningMenuHitTest.*` unless menu coordinate routing changes.
- Window input and mouse capture files unless input ownership changes.

## Update When

- Hit layering, rect containment, disabled-region policy, receipt fields, or surface enum values change.

## Do Not Update When

- Only a specific UI surface adds, removes, or repositions its own hit regions.
