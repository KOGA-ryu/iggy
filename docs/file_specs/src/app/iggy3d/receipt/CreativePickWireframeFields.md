# File Spec

Files: `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`

Verified at: `cd02b03e`

## Owns

- Receipt field emission for CreativeDocument viewport pick diagnostics.
- Receipt field emission for CreativeDocument wireframe and debug-line diagnostics.
- Vulkan gameplay readiness fields appended beside creative wireframe proof.

## Does Not Own

- Viewport pick ray/grid/object selection.
- Wireframe item generation.
- Vulkan readiness evaluation.
- Creative UI command execution.

## Reads

- `ProductAppWindowState.creativeAuthoring` viewport pick, wireframe, debug-line state.
- `ProductVulkanGameplayReadiness`.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate creative authoring state or render readiness state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductCreativePickWireframeFields(...)`.
- Focused proof: `rg -n "appendProductCreativePickWireframeFields|creative_viewport_pick_requested|creative_wireframe_requested" src/app tests`.

## Invariants

- Viewport pick and wireframe facts stay separate from Creative UI command facts.
- Pick coordinates/object facts are emitted from previously recorded diagnostics.
- Wireframe debug-line counts and skipped-degenerate facts are receipt proof only.
- Vulkan gameplay readiness is read-only context here, not render policy.

## Tests / Proof Commands

- `rg -n "product_creative_viewport_pick_frame_tests|creative_viewport_pick_tests|product_vulkan_room_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_viewport_pick_requested|creative_wireframe_status|product_vulkan_gameplay_ready" tests/unit src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/ViewportPickFrame.*` unless pick diagnostics change.
- `src/app/iggy3d/creative/bridge/WireframeFrame.*` unless wireframe diagnostics change.
- `src/app/iggy3d/window/RendererLifecycle.*` unless Vulkan readiness fields change.

## Update When

- Viewport pick receipt keys, wireframe/debug-line receipt keys, or Vulkan readiness receipt fields in this group change.

## Do Not Update When

- Only underlying pick math, wireframe drawing, or Vulkan backend behavior changes without changing emitted receipt fields.
