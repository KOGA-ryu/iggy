# File Spec

Files: `src/app/iggy3d/view/RenderBridge.hpp`, `src/app/iggy3d/view/RenderBridge.cpp`

Verified at: `9f9c8b29`

## Owns

- App-facing render bridge summary frame built from primitive draw-list facts, viewport framed items, and gameplay feedback.
- Render bridge item packets with kind, entity id, stable name, visibility, on-screen status, target/interactable flags, screen coordinates, and depth.
- Counts and visibility flags for draw items, frame items, on-screen items, targets, feedback lines, room editor markers, props, physics debug, and map-maker bridge facts.

## Does Not Own

- Primitive draw-list construction.
- Viewport projection math.
- Gameplay feedback construction.
- Receipt field emission.
- SDL or Vulkan drawing.
- Runtime/render backend resources.

## Reads

- Optional `ProductPrimitiveDrawList`, optional `ProductViewportFrame`, and optional `GameplayFeedback`.
- Primitive kind, entity id, stable name, visibility, targeting/interactable flags, frame coordinates, and depth.

## Writes / Mutates

- Returns `ProductRenderBridgeFrame`.
- Does not mutate input draw list, viewport frame, gameplay feedback, or window state.

## Calls Out To / Wires Out To

- `toUint64(...)` for entity id conversion.
- `ProjectionRefresh.cpp` copies bridge facts into `ProductViewportState`.
- Receipt appenders read the copied viewport bridge fields.

## Called By / Entry Points

- `ProjectionRefresh.cpp` calls `buildProductRenderBridgeFrame(...)`.
- `product_render_bridge_tests` calls the builder directly.
- Focused proof: `rg -n "buildProductRenderBridgeFrame|ProductRenderBridgeFrame|productRenderBridge" src/app tests/unit`.

## Invariants

- Bridge is ready only when draw list and viewport frame are both present.
- Feedback readiness requires visible feedback.
- Draw-list aggregate counts are preferred where available; framed fallback counts fill gaps for props, physics debug, and map-maker items.
- Target count includes targetable/interactable items and selected primitive kinds.
- This bridge summarizes projected draw facts; it must not become gameplay or render backend truth.

## Tests / Proof Commands

- `rg -n "product_render_bridge_tests|product_vulkan_room_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductRenderBridgeFrame|physicsDebugItemCount|mapMakerGridDotCount|feedbackLineCount" tests/unit/product_render_bridge_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/ViewportFraming.*` unless framed item fields change.
- `src/app/iggy3d/view/PrimitiveDrawList.*` unless primitive kind or aggregate counters change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless bridge-to-viewport copying changes.
- `src/app/iggy3d/receipt/GameplaySceneStateFields.*` unless receipt keys change.

## Update When

- Bridge frame fields, aggregate counts, target/interactable policy, fallback counting, or bridge readiness semantics change.

## Do Not Update When

- Only low-level drawing or runtime projection sources change without changing render bridge output.
