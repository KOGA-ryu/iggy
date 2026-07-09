# File Spec

Files: `src/app/iggy3d/ui/Widget.hpp`, `src/app/iggy3d/ui/Widget.cpp`

Verified at: `ac496769`

## Owns

- L1 value-widget emission helpers for product UI draw-list builders.
- `WidgetOutput`, `UiText`, and `UiPanel`.
- Deterministic translation of text/panel widget values into `ProductUiPrimitive` records.
- Optional button hit-region emission from the same rect as the primitive.
- Appending widget output into `ProductUiDrawList` with primitive and hit-region counters.

## Does Not Own

- Screen-specific layout or row ordering.
- Hit routing, input dispatch, or command execution.
- Renderer behavior or SDL/Vulkan presentation.
- Higher-level UI model construction.

## Reads

- Widget rect, tone, semantic id, text, action, selected, and enabled fields.
- `ProductUiPrimitive`, `ProductUiDrawList`, `UiHitRegion`, and `FrontendAction` contracts.

## Writes / Mutates

- Appends primitives and hit regions to `WidgetOutput`.
- Appends widget output into a supplied `ProductUiDrawList`.
- Increments draw-list text, rect, and hit-region counters.

## Calls Out To / Wires Out To

- Does not call screen or input routers.
- Consumers include menu draw-list builders, pause UI, and notebook UI.
- Hit regions emitted here are later consumed by UI hit routing.

## Called By / Entry Points

- `emit(const UiText&, WidgetOutput&)`.
- `emit(const UiPanel&, WidgetOutput&)`.
- `appendWidgetOutput(ProductUiDrawList&, const WidgetOutput&)`.
- Focused proof: `rg -n "emit\\(|appendWidgetOutput|WidgetOutput|UiText|UiPanel|product_ui_widget_tests" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Widgets are pure values; no retained UI state is owned here.
- A widget with `FrontendAction::None` does not emit a hit region.
- A widget with an action emits one button hit region using the primitive rect and semantic id.
- `appendWidgetOutput` keeps draw-list counters aligned with appended primitives and hit regions.

## Tests / Proof Commands

- `product_ui_widget_tests` covers primitive emission, action hit regions, and draw-list append counters.
- `rg -n "product_ui_widget_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/DrawList.*` unless primitive or hit-region schema changes.
- `src/app/iggy3d/menu/UiHitRouter.*` unless hit-region routing policy changes.
- Screen-specific UI builders unless their layout changes.

## Update When

- Widget structs, emitted primitive fields, hit-region policy, or draw-list counter ownership changes.

## Do Not Update When

- Only a screen-specific layout or text string changes.
