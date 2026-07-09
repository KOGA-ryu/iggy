# File Spec

Files: `src/app/iggy3d/window/ProductVulkanRendererState.hpp`

Verified at: `b50008a5`

## Owns

- Small window-owned state packet for product Vulkan renderer requested, created, and ready flags.
- Renderer lifecycle observability fields embedded through present-path state.

## Does Not Own

- Vulkan renderer creation, destruction, swapchain, frame rendering, or diagnostics.
- SDL window or Vulkan surface creation.
- Product frontend/menu state or receipt formatting.

## Reads

- This header defines state only.
- Readers include renderer lifecycle code, present-path state, and receipt appenders.

## Writes / Mutates

- No functions in this file mutate state.
- `RendererLifecycle.cpp` and window loop/present wiring mutate instances of this packet.

## Calls Out To / Wires Out To

- Included by `PresentPathStore.hpp`.
- Receipt appenders read copied fields as product Vulkan renderer proof.

## Called By / Entry Points

- `PresentPathStore` embeds `ProductVulkanRendererState`.
- `RendererLifecycle.cpp` updates renderer availability/readiness around backend setup.
- Focused proof: `rg -n "ProductVulkanRendererState|vulkanRenderer\\.requested|vulkanRenderer\\.created|vulkanRenderer\\.ready|product_vulkan_renderer" src tests`.

## Invariants

- Defaults mean no Vulkan renderer requested, created, or ready.
- These flags are app/window observability, not renderer backend ownership.
- Keep this packet narrow; add renderer details to renderer diagnostics or lifecycle surfaces instead.

## Tests / Proof Commands

- `rg -n "product_window_renderer_lifecycle_tests|product_vulkan_renderer" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/PresentPathStore.hpp` unless embedding changes.
- `src/app/iggy3d/window/RendererLifecycle.*` unless lifecycle state production changes.
- `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.*` unless receipt keys change.
- `src/render/vulkan/*` unless backend diagnostics or ownership changes.

## Update When

- Renderer requested/created/ready semantics, packet fields, embedding, or receipt interpretation changes.

## Do Not Update When

- Only backend rendering internals or SDL/Vulkan surface logic changes without changing this app-window state packet.
