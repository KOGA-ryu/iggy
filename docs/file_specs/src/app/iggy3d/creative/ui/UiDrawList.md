# File Spec

Files: `src/app/iggy3d/creative/ui/UiDrawList.hpp`, `src/app/iggy3d/creative/ui/UiDrawList.cpp`

Verified at: `7c9e712f`

## Owns

- Conversion from `creative::CreativeUiModel` into `ProductUiDrawList`.
- Creative overlay panel layout, row text formatting, semantic ids, row hit regions, and draw-list counters.
- Null-model rejection status for creative UI draw-list requests.

## Does Not Own

- Creative UI model construction.
- Hit routing, input consumption, or command execution.
- SDL/Vulkan presentation and receipt-field emission.

## Reads

- `ProductCreativeUiDrawListRequest` model pointer, virtual size, and theme.
- `CreativeUiPanel` visibility/enabled spans and `CreativeUiRow` kinds, labels, flags, targets, values, and object facts.
- `ProductUiDrawList` and `UiHitRegion` contracts.

## Writes / Mutates

- Builds and returns a `ProductUiDrawList`.
- Populates primitives, hit regions, virtual size, theme, status, row counts, disabled row counts, and hit-region counts.
- Does not mutate the source model.

## Calls Out To / Wires Out To

- Produces semantic ids consumed by `routeProductUiHit`, creative UI input, world-launch tests, and receipts.
- Supplies row hit regions to product UI hit routing and creative command dispatch.

## Called By / Entry Points

- `buildProductCreativeUiDrawList`.
- `buildProductCreativeUiProjection`.
- Focused proof: `rg -n "buildProductCreativeUiDrawList|ProductCreativeUiDrawListRequest|creative\\.row|product_creative_ui_draw_list" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Null model returns a not-ready draw list with zero rows and hit regions.
- Ready draw lists use stable `creative.row.<panel>.<row>` semantic ids.
- Visible rows produce matching text primitives and hit regions.
- Disabled rows remain hittable but are marked disabled for downstream routing.
- `hitRegionCount` mirrors the hit-region vector size.

## Tests / Proof Commands

- `product_creative_ui_draw_list_tests` covers null model, ready status, semantic ids, hit regions, disabled rows, inspector rows, and deterministic repeated output.
- `product_ui_hit_router_tests` covers routing against draw lists from this builder.
- `rg -n "product_creative_ui_draw_list_tests|product_ui_hit_router_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/Ui.*` unless model row contracts change.
- `src/app/iggy3d/menu/DrawList.*` unless draw-list schema changes.
- `src/app/iggy3d/menu/UiHitRouter.*` unless hit-routing policy changes.

## Update When

- Creative row text, panel layout, semantic-id generation, hit-region policy, draw-list status, or counter ownership changes.

## Do Not Update When

- Only creative model input values or command execution effects change.
