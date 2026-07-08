# E223 - OpeningMenuView Primitive Primary Metadata Handoff

## Status

Done.

## Context

E222 audited the post-E127 draw-kind metadata remainder. Base
`ProductPrimitiveDrawKind` color and marker-size metadata is already centralized
in `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`.

E222 found only two remaining primary/base renderer duplicates worth moving:

- `OpeningMenuView.cpp::drawFocusIndicator(...)` hardcodes the
  `PlayerFocusIndicator` primary color `{226,230,211}` and fixed 36-pixel cross
  span.
- `OpeningMenuView.cpp::drawRoomEditorCursor(...)` hardcodes the
  `RoomEditorCursor` primary color `{245,214,96}` while keeping a local outline
  color.

Everything else that looked similar was classified as dynamic state,
secondary/decorative renderer policy, or bridge/count dispatch.

## Objective

Route only the two primitive primary renderer colors through the already-built
draw item/metadata path while preserving current SDL output shape and size.

## Implementation Scope

Edit:

- `src/app/iggy3d/view/OpeningMenuView.cpp`
- this task card

Likely no CMake or test-source changes are needed.

## Required Behavior

For `drawFocusIndicator(...)`:

- Use the framed item primary color instead of hardcoded `{226,230,211}`.
- Preserve the current fixed 36-pixel cross span and 4-pixel thickness.
- Do not use perspective-scaled `framed.item.markerSize` for focus size unless
  the card is stopped and owner explicitly requests a behavior change.
- If a helper is useful, it should only set SDL color from
  `ProductPrimitiveColor`.

For `drawRoomEditorCursor(...)`:

- Use `framed.item.color` for the primary cursor cross color instead of
  hardcoded `{245,214,96}`.
- Preserve current cursor size from `item.markerSize`.
- Keep outline color `{32,42,44}` local in `OpeningMenuView.cpp`.

Preserve:

- draw ordering
- primitive shape dispatch
- tile decoration colors
- door marker detail/knob color
- map-maker cube inner rect color
- draw-list and render-bridge counts
- receipt fields/order/values

## Non-Scope

Do not edit:

- `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`
- `src/app/iggy3d/view/PrimitiveDrawList.*`
- `src/app/iggy3d/view/RenderBridge.*`
- `src/app/iggy3d/window/FramePresenter.cpp`
- tests
- CMake
- receipt golden files

Do not change:

- `drawRoomTile(...)`
- `drawDoorMarker(...)`
- `drawPrimitiveItem(...)` dispatch
- physics debug drawing
- map-maker drawing
- secondary/decorative color ownership
- focus indicator perspective scaling

No staging, commit, push, or window launch.

## Required Greps

Run and report:

```sh
rg -n "drawFocusIndicator|drawRoomEditorCursor|setColor\\(renderer, item\\.color|setColor\\(renderer, 226, 230, 211\\)|setColor\\(renderer, 245, 214, 96\\)" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp
rg -n "ProductPrimitiveDrawKindMetadata|PlayerFocusIndicator|RoomEditorCursor" /Users/kogaryu/iggy3d/src/app/iggy3d/view/PrimitiveDrawMetadata.hpp /Users/kogaryu/iggy3d/tests/unit/product_primitive_draw_list_tests.cpp
```

Expected classification:

- `drawFocusIndicator(...)` and `drawRoomEditorCursor(...)` consume item color
  for their primary primitive color.
- Other `{226,230,211}` or `{245,214,96}` hits in `OpeningMenuView.cpp`, if any,
  are unrelated menu/debug UI colors and are not part of this primitive handoff.
- Metadata/test rows for `PlayerFocusIndicator` and `RoomEditorCursor` remain
  unchanged.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- preserving the fixed focus-indicator size cannot be done cleanly
- implementation wants to use perspective-scaled focus size
- the change requires an SDL pixel/screenshot harness
- the change pulls in secondary/decorative color catalog work
- `RenderBridge`, `PrimitiveDrawList`, metadata shape, or CMake need edits
- receipt golden output changes

## Completion Brief

When done, report:

- files changed
- exact primary color handoff shape
- confirmation of preserved focus/cursor sizes
- grep classification
- focused build/CTest result
- receipt golden diff result
- diff/whitespace checks
- confirmation that metadata, draw-list, render-bridge, secondary colors, CMake,
  tests, staging, commit, push, and window launch were not touched

## Completion Brief - E223

- Files changed:
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `docs/creative_mode/builder_tasks/done/E223-openingmenuview-primitive-primary-metadata-handoff.md`
- Exact primary color handoff shape:
  - `drawFocusIndicator(...)` now reads `framed.item.color` through a local `const ProductPrimitiveDrawItem& item = framed.item` and calls `setColor(renderer, item.color.r, item.color.g, item.color.b)`.
  - `drawRoomEditorCursor(...)` now uses its existing `item` alias and calls `setColor(renderer, item.color.r, item.color.g, item.color.b)` for the primary cursor cross.
  - No `PrimitiveDrawMetadata.hpp` include was needed because `ProductViewportFramedItem` already carries the draw item color from the metadata-backed draw-list path.
- Preserved focus/cursor sizes:
  - Focus indicator still uses fixed `36.0F` span and `4.0F` thickness through the existing `fillRect(...)` calls.
  - Focus indicator does not use perspective-scaled `framed.item.markerSize`.
  - Room editor cursor still uses `item.markerSize`, `half = size * 0.5F`, and the existing 4-pixel cross / 3-pixel outline geometry.
  - Room editor cursor outline color `{32,42,44}` remains local.
- Grep classification:
  - Required `OpeningMenuView.cpp` grep shows `drawFocusIndicator(...)` and `drawRoomEditorCursor(...)` now consume `item.color` for the primitive primary color.
  - Remaining `{226,230,211}` hits are camera heading center dot, gameplay feedback title, position HUD text, ASCII/editor/menu panel text, and gameplay title text.
  - Remaining `{245,214,96}` hits are room editor HUD title and movement tuning HUD title.
  - Metadata/test grep shows `PlayerFocusIndicator` and `RoomEditorCursor` rows remain in `PrimitiveDrawMetadata.hpp` and are still pinned by `product_primitive_draw_list_tests.cpp`.
- Focused build/CTest result:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure` passed, 3/3.
- Receipt golden diff result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched source/card files returned no hits.
- Confirmation:
  - `PrimitiveDrawMetadata.hpp`, `PrimitiveDrawList.*`, `RenderBridge.*`, `FramePresenter.cpp`, secondary/decorative colors, tile/door/physics/map-maker drawing, draw dispatch, CMake, tests, receipt golden, staging, commit, push, and window launch were not touched.
