# E127: ProductPrimitiveDrawKind Presentation Metadata Table

## Objective

Replace duplicated `ProductPrimitiveDrawKind` base presentation data with a
single metadata catalog, while preserving current render behavior.

This is the implementation follow-up to E124. Do not resolve decorative/render
drift silently; either keep current render-local decoration or pin an explicit
owner decision in tests.

## Scope

- `src/app/iggy3d/view/PrimitiveDrawList.hpp`
- `src/app/iggy3d/view/PrimitiveDrawList.cpp`
- focused primitive draw/view tests only, unless a tiny header split is needed.

Possible helper home:

- `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp` as a header-only catalog
  modeled after `creative/bridge/UiCommandCatalog.hpp`.

## Required Work

1. Add a `constexpr` catalog keyed by `ProductPrimitiveDrawKind`.
   Suggested shape:

   ```cpp
   struct ProductPrimitiveDrawKindMetadata {
     ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
     ProductPrimitiveColor baseColor{};
     float markerSize = 14.0F;
   };
   ```

   Current code has no per-kind layer/z-order field. Do not invent render
   ordering behavior in this slice. If a layer field is added for future use,
   all rows should keep the current neutral value and tests must prove no
   ordering behavior changed.

2. Add lookup helpers such as:
   - `productPrimitiveDrawKindMetadataCatalog()`
   - `findProductPrimitiveDrawKindMetadata(...)`
   - `baseColorForProductPrimitiveDrawKind(...)`
   - `markerSizeForProductPrimitiveDrawKind(...)`

3. Route data-carrying construction sites through the catalog:
   - `colorForRoomKind(...)` or its replacement;
   - static default marker sizes for floor/wall/prop/entity/debug/editor/map
     primitive items.

4. Preserve dynamic variants as local policy unless owner explicitly chooses to
   table them:
   - Door open vs closed color/size.
   - Prop ledge/reset-zone color/size.
   - Placement preview wall/object/default size.
   - Physics debug sensor vs solid color/size.
   - Map-maker grid major vs minor color/size.

5. Do not table-ify genuine dispatch:
   - `OpeningMenuView::drawPrimitiveItem(...)`.
   - `OpeningMenuView::drawRoomTile(...)` shape/decorative drawing.
   - `RenderBridge` counting/target classification.
   - `PrimitiveDrawList::updateCounts(...)`.
   - `ViewportFraming` player anchor and perspective marker scaling.

6. Add guard tests that pin the catalog rows and prove routed construction
   outputs still match current values.

## Drift Decisions To Pin Or Keep Render-Local

These render decoration colors disagree with the primary base color or duplicate
construction values. Do not silently choose a winner:

- `PlayerFocusIndicator`: construction `{226,230,211}` / `36`; renderer
  hardcodes the same color and effective size as a cross. Owner decision:
  probably route the renderer through metadata for color/size.
- `RoomEditorCursor`: construction `{245,214,96}` / `30`; renderer hardcodes
  the same primary color plus outline `{32,42,44}`. Owner decision: primary can
  route through metadata; outline should stay render-local unless table grows a
  secondary color.
- `ElevatedFloorTile`: base `{92,126,102}` / `58`; renderer inset color
  `{137,168,143}`.
- `RampTile`: base `{82,139,156}` / `58`; renderer stripe color
  `{183,213,210}`.
- `BlockedSlopeTile`: base `{184,82,74}` / `58`; renderer stripe color
  `{246,184,130}`.
- `WallTile`: base `{76,86,92}` / `62`; renderer border color
  `{116,128,132}`.
- `PropTile`: base `{151,102,58}` / `42`; render fill/detail colors
  `{198,142,82}` and `{88,58,34}`. Dynamic variants are ledge
  `{76,132,178}` / `52` and reset-zone `{214,74,92}` / `34`.
- `DoorMarker`: construction owns open `{126,201,176}` / `18` and closed
  `{220,178,86}` / `24`; renderer adds knob color `{94,74,54}`.
- `MapMakerCubePreview`: base `{126,221,186}` / `54`; frame presenter adds
  inner rect color `{48,65,72}`.

## Acceptance

- Base metadata for every `ProductPrimitiveDrawKind` is centralized.
- Dynamic and decorative variants are either still local policy or explicitly
  covered by tests and owner decisions.
- No render output, counts, or draw ordering behavior changes.
- Existing focused render/view tests remain green.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests product_render_bridge_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_primitive_draw_list_tests|product_render_bridge_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Metadata helper/API:
- Dynamic/decorative policy:
- Behavior-preservation tests:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`
  - `src/app/iggy3d/view/PrimitiveDrawList.cpp`
  - `tests/unit/product_primitive_draw_list_tests.cpp`
- Metadata helper/API:
  - Added header-only `ProductPrimitiveDrawKindMetadata` catalog keyed by
    `ProductPrimitiveDrawKind`.
  - Added `productPrimitiveDrawKindMetadataCatalog()`,
    `findProductPrimitiveDrawKindMetadata(...)`,
    `baseColorForProductPrimitiveDrawKind(...)`, and
    `markerSizeForProductPrimitiveDrawKind(...)`.
  - Catalog rows cover all 22 current primitive draw kinds.
  - No layer/z/order field was added; draw ordering remains append order.
- Dynamic/decorative policy:
  - Routed base/default construction through the catalog for scene markers,
    room floor/wall/prop defaults, room editor cursor/default placement preview,
    solid physics debug defaults, map-maker minor grid dots, map-maker cube
    preview, and player focus indicator.
  - Kept dynamic variants local: door open/closed state, ledge/reset-zone prop
    variants, placement preview wall/object/default sizing, physics sensor style,
    and map-maker major grid dots.
  - Kept decorative renderer-only colors local/out of scope: room/editor outlines,
    tile stripes/insets/borders, door knob, prop detail colors, and map-maker
    cube inner rect.
- Behavior-preservation tests:
  - `product_primitive_draw_list_tests` now pins every catalog row and helper
    lookup for color/marker size.
  - Existing draw-list assertions still prove scene item ordering/counts,
    player focus insertion, room geometry item colors/sizes, physics debug
    styles, map-maker variants, and room-editor preview behavior.
  - `product_render_bridge_tests` remained green, proving bridge/counter behavior
    did not change.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests product_render_bridge_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_primitive_draw_list_tests|product_render_bridge_tests)$' --output-on-failure` passed, 2/2.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files passed.
- Concerns/deferred:
  - Renderer dispatch/decorative drawing remains intentionally local to
    `OpeningMenuView`/presenter code; this slice only centralizes base
    construction data.
  - If future work adds secondary colors or layers, it should be an explicit
    catalog expansion with tests, not inferred from this base metadata table.
