# File Spec

Files: `src/app/iggy3d/view/ScenePrimitiveView.hpp`, `src/app/iggy3d/view/ScenePrimitiveView.cpp`

Verified at: `2b846f78`

## Owns

- SDL drawing of projected primitive viewport items for first-person and top-down map views.
- Shape-specific marker drawing for room tiles, doors, focus indicators, room editor cursor/preview, physics debug markers, and generic actor/object/debug markers.
- Top-down map remapping from world positions into compact minimap or full overview coordinates.

## Does Not Own

- Primitive draw-list construction.
- Viewport projection/framing calculations.
- Runtime world state, physics state, or room editor state.
- Top-down overlay model construction.
- Vulkan mesh/render backend behavior.

## Reads

- `ProductViewportFrame`, framed primitive items, primitive draw kinds/colors/marker sizes/world positions, grid visibility, and `TopDownMapOverlay`.
- Player marker position as top-down map anchor when available.

## Writes / Mutates

- Draws to an `SDL_Renderer`.
- Locally remaps item screen coordinates for top-down drawing.
- Does not mutate `ProductViewportFrame`, primitive source data, overlay state, or runtime state.

## Calls Out To / Wires Out To

- `SdlDraw.*` helpers.
- `SDL_RenderLine(...)` for line details.
- `ViewportFraming.*` data types.

## Called By / Entry Points

- `OpeningMenuView.cpp` calls `drawFirstPersonPrimitiveViewport(...)` and `drawTopDownMapPrimitives(...)` for the gameplay panel.
- Focused proof: `rg -n "drawFirstPersonPrimitiveViewport|drawTopDownMapPrimitives|ProductPrimitiveDrawKind" src/app tests`.

## Invariants

- Null frame draws the viewport background and no primitive items.
- First-person viewport draws only framed items marked on-screen.
- Top-down view requires visible overlay and frame.
- Compact minimap and full overview use different origins, sizes, and scale factors.
- Hidden map-maker grid dots are intentionally skipped in primitive item drawing.
- This file renders projected facts; it must not become simulation or collision truth.

## Tests / Proof Commands

- `rg -n "drawFirstPersonPrimitiveViewport|drawTopDownMapPrimitives" src/app tests`.
- `rg -n "product_vulkan_room_frame_tests|product_top_down_map_overlay_tests|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/PrimitiveDrawList.*` unless primitive item kinds/fields change.
- `src/app/iggy3d/view/ViewportFraming.*` unless projection output semantics change.
- `src/app/iggy3d/debug/TopDownMapOverlay.*` unless overlay model fields change.
- `src/render/*` unless backend rendering changes.

## Update When

- Primitive draw kinds, SDL marker rendering, top-down remapping policy, minimap layout, or first-person viewport drawing behavior changes.

## Do Not Update When

- Only runtime facts or projection inputs change without changing projected primitive drawing policy.
