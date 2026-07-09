# File Spec

Files: `src/app/iggy3d/view/ViewportFraming.hpp`, `src/app/iggy3d/view/ViewportFraming.cpp`

Verified at: `9f9c8b29`

## Owns

- Projection of product primitive draw items into a lightweight first-person viewport frame.
- Camera anchor selection from the visible player marker or caller-provided override.
- Yaw, pitch, depth, perspective marker scaling, on-screen testing, and framed-item packet creation.

## Does Not Own

- Primitive draw-list construction.
- Runtime camera state or render-camera matrices.
- SDL or Vulkan drawing.
- Hit testing or input handling.
- Simulation, collision, or world state.

## Reads

- `ProductPrimitiveDrawList`, draw item visibility, world positions, marker sizes, grid visibility, and `ProductViewportFrameConfig`.

## Writes / Mutates

- Returns `ProductViewportFrame`.
- Copies primitive draw items into framed items with projected screen coordinates, depth, marker size, and on-screen status.
- Does not mutate the source draw list or viewport config.

## Calls Out To / Wires Out To

- Uses `Vec3` math and standard math functions.
- `ProjectionRefresh.cpp`, room editor mouse-pick helpers, render bridge, and SDL draw paths consume the frame.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds viewport frames for product gameplay projection.
- `product_render_bridge_tests` and `product_viewport_framing_tests` call `buildProductViewportFrame(...)`.
- Focused proof: `rg -n "buildProductViewportFrame|ProductViewportFrameConfig|ProductViewportFrame" src/app tests/unit`.

## Invariants

- Player marker anchor is preferred unless an explicit camera anchor override is available.
- Override anchors also mark player-anchor-found true for receipt/proof purposes.
- Near-depth clamping protects marker size and screen projection math.
- Items behind or too close to the near threshold are not on screen.
- The frame keeps draw-list grid visibility and projection mode facts for downstream receipts/render bridges.

## Tests / Proof Commands

- `rg -n "product_viewport_framing_tests|product_render_bridge_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductViewportFrame|playerAnchorFound|yawApplied|pitchApplied" tests/unit/product_viewport_framing_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/PrimitiveDrawList.*` unless primitive item fields change.
- `src/app/iggy3d/view/RenderBridge.*` unless framed packet consumption changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless projection orchestration changes.
- `src/render/*` unless backend render camera policy changes.

## Update When

- Projection math, frame config, anchor policy, on-screen policy, perspective scaling, or framed item fields change.

## Do Not Update When

- Only primitive draw-list source facts or SDL/Vulkan rendering changes without changing viewport framing output.
