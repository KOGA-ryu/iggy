# File Spec

Files: `src/app/iggy3d/view/ViewportState.hpp`

Verified at: `9f9c8b29`

## Owns

- Product app viewport state packet embedded in `ProductAppWindowState`.
- Camera, map-maker, creative fly, primitive draw, viewport frame, render bridge, feedback bridge, and Vulkan room mesh proof fields.
- Default values for viewport receipts and frame/projection observability.

## Does Not Own

- Camera look-action math.
- Creative fly anchor helper behavior.
- Primitive draw-list construction.
- Viewport framing math.
- Render bridge aggregation.
- Vulkan room mesh baking or backend presentation.
- Receipt field emission.

## Reads

- This header defines state only; readers include projection refresh, input frame, renderer lifecycle, frame presenter, receipt field appenders, and tests.

## Writes / Mutates

- No functions in this file mutate state.
- State is mutated by camera controller, creative fly helpers, projection refresh, render bridge copying, room mesh proof helpers, transitions, input frame, and room-editing automation cleanup.

## Calls Out To / Wires Out To

- Includes `CreativeFlyAnchorStore.hpp` for the embedded creative fly anchor.
- Embedded by `ProductAppWindowState`.
- Receipt appenders consume fields from this packet.

## Called By / Entry Points

- `ProductAppWindowState.hpp` contains `ProductViewportState viewport`.
- `ProjectionRefresh.cpp`, `CameraController.cpp`, `RendererLifecycle.cpp`, `FramePresenter.cpp`, `InputFrame.cpp`, and receipt field appenders read or write fields.
- Focused proof: `rg -n "ProductViewportState|camera_controller_active|product_draw_item_count|product_vulkan_room_mesh" src/app tests`.

## Invariants

- Defaults represent no gameplay view, first-person product camera, inactive map-maker, no creative fly request, no draw/projection bridge readiness, and no Vulkan room mesh proof.
- This packet is app/view observability state and should not become runtime simulation or save truth.
- Field additions must identify the owning producer and receipt consumer.
- Keep transient/off-save nature explicit when adding mirrors of runtime or renderer facts.

## Tests / Proof Commands

- `rg -n "ProductViewportState|product_creative_fly_tests|product_camera_controller_tests|product_vulkan_room_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "camera_controller_active|product_draw_item_count|product_render_bridge_ready|product_vulkan_room_mesh_cpu_ready" src/app/iggy3d/receipt tests/smoke tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.hpp` unless the viewport state embedding changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless producer ownership changes.
- `src/app/iggy3d/receipt/GameplaySceneStateFields.*` unless receipt fields change.
- `src/app/iggy3d/view/CreativeFlyAnchorStore.*` unless creative fly anchor fields change.

## Update When

- Viewport state fields, default values, producer ownership, receipt mirror fields, or transient/off-save boundaries change.

## Do Not Update When

- Only a downstream renderer or runtime system changes behavior without changing viewport state fields or ownership.
