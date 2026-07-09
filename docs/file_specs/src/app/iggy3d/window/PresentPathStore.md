# File Spec

Files: `src/app/iggy3d/window/PresentPathStore.hpp`, `src/app/iggy3d/window/ProductVulkanRendererState.hpp`

Verified at: `bdb9e108`

## Owns

- Product presentation-path state packet embedded in `ProductAppWindowState`.
- Vulkan renderer requested/created/ready flags, surface/swapchain/frame submission flags, frame submitted count, Vulkan status/reason, rendering path, and record mode.

## Does Not Own

- Renderer backend creation, frame recording, frame presentation, renderer shutdown, gameplay projection readiness, Vulkan menu UI proof, or receipt field writing.

## Reads

- No live inputs; these headers define packet types.
- Callers read this packet to decide Vulkan gameplay readiness, final renderer status, and receipt/proof fields.

## Writes / Mutates

- Mutated by `Loop.*`, `RendererLifecycle.*`, and `FramePresenter.*`.
- This file itself has no behavior.

## Calls Out To / Wires Out To

- `PresentPathStore` contains `ProductVulkanRendererState`.
- State is consumed by renderer lifecycle, frame presenter, receipt builder, and tests.

## Called By / Entry Points

- Instantiated through `ProductAppWindowState`.
- Grep proof: `rg -n "PresentPathStore|presentPath\\.|ProductVulkanRendererState|productVulkanRenderer" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- `productVulkanRenderer.requested` is separate from backend created/ready.
- Surface, swapchain, frame-submitted, rendering-path, and record-mode facts are present-path proof, not renderer backend ownership.
- Frame submitted count increments only through submit recording.
- Status/reason strings must stay stable for receipts and smoke tests.

## Tests / Proof Commands

- `rg -n "product_window_renderer_lifecycle_tests|product_vulkan_room_frame_tests|product_render_bridge_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "productVulkanRenderer|productVulkanFrameSubmitted|productVulkanStatus|productVulkanReasonCode|productVulkanRenderingPath|productVulkanRecordMode" tests/unit/product_window_renderer_lifecycle_tests.cpp tests/unit/product_vulkan_room_frame_tests.cpp src/app/iggy3d/window`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/RendererLifecycle.*` unless lifecycle writes/readiness rules change.
- `src/app/iggy3d/window/FramePresenter.*` unless submit/present facts change.
- `src/render/**` unless renderer API facts change.

## Update When

- Present-path fields, default values, nested renderer flag ownership, status/reason meanings, or submit proof facts change.

## Do Not Update When

- Only renderer backend internals, menu layout, gameplay projection, or receipt ordering changes without changing present-path packet semantics.
