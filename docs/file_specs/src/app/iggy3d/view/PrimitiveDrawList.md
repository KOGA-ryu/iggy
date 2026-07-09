# File Spec

Files: `src/app/iggy3d/view/PrimitiveDrawList.hpp`, `src/app/iggy3d/view/PrimitiveDrawList.cpp`

Verified at: `3dab1aa4`

## Owns

- Product primitive draw item/list packets and `buildProductPrimitiveDrawList(...)`.
- Conversion of scene items, debug projection items, active room geometry, room editor overlays, map maker overlays, and physics debug items into product draw primitives.
- Per-kind visibility/count mirrors used by viewport/render receipts.

## Does Not Own

- Runtime scene/debug fact generation.
- Viewport screen-space framing.
- SDL/Vulkan primitive rendering.
- Room editor or map maker command behavior.

## Reads

- `SceneProjectionResult`, `DebugProjectionResult`, `RoomAsset`, active room collision state, room editor overlay/preview, map maker grid overlay, and map maker cube preview.
- Primitive metadata for colors and marker sizes.

## Writes / Mutates

- Returned `ProductPrimitiveDrawList` items, flags, and counts only.
- No source projection, room, or app state mutation.

## Calls Out To / Wires Out To

- `PrimitiveDrawMetadata` for base colors/sizes.
- Active room collision, room/editor/map maker packet types, and debug projection item kinds.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds draw lists for gameplay projection frames.
- Viewport framing, render bridge, scene primitive view, and tests consume the list.
- Grep proof: `rg -n "ProductPrimitiveDrawKind|ProductPrimitiveDrawList|PhysicsAabbDebug|MapMakerGridDot|RoomEditorPlacementPreview|PrimitiveDrawList" src/app/iggy3d tests/unit cmake CMakeLists.txt`.

## Invariants

- Build accepts null inputs and emits an empty list where appropriate.
- Physics debug primitives originate from debug projection items, not direct physics state.
- Room editor/map maker primitives are overlays, not command execution.
- Counts must stay consistent with emitted visible item kinds.

## Tests / Proof Commands

- `rg -n "product_primitive_draw_list_tests|product_render_bridge_tests|product_viewport_framing_tests|product_room_editor_overlay_tests" cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/projection/*` unless source projection packets change.
- `src/app/iggy3d/view/ViewportFraming.*` and `RenderBridge.*` unless draw-list consumers change.
- Room editor/map maker source files unless overlay packet shapes change.

## Update When

- Primitive kinds, item fields, count semantics, source packet inputs, or overlay conversion rules change.

## Do Not Update When

- Rendering style changes without changing draw-list contracts.
