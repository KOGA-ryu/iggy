# File Spec

Files: `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`, `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`

Verified at: `3dab1aa4`

## Owns

- `ProductGameplayProjectionFrame` assembly for active gameplay/editor views.
- Product-facing refresh of scene projection, debug projection, primitive draw list, viewport frame, HUD packets, map maker overlays, room editor overlays, and render bridge frame.
- Window metric copies for projection/render proof fields.
- Product Vulkan frame input construction from scene/debug projections.

## Does Not Own

- Runtime simulation, AI perception truth, physics query truth, or renderer submission.
- SDL/Vulkan drawing of HUD panels.
- Save/load or frontend route decisions.

## Reads

- Active `Session`, `ProductAppWindowState`, `FrontendState`, renderer/debug flags, active room store, creative state, and window viewport state.
- Runtime debug snapshots via session state and movement transient facts.

## Writes / Mutates

- Returned `ProductGameplayProjectionFrame`.
- Product window projection/HUD/viewport proof mirrors through copy/apply helpers.
- No runtime `SessionState` mutation.

## Calls Out To / Wires Out To

- `buildSceneProjection(...)`, `buildDebugProjection(...)`, `buildNpcBehaviorDebugSnapshot(...)`, and debug append helpers.
- `buildProductPrimitiveDrawList(...)`, viewport framing, map-maker/room-editor presentation, HUD builders, and render bridge builders.
- `vulkan::buildRoomMeshCpuGeometry(...)` for room mesh proof metrics.

## Called By / Entry Points

- `Loop.cpp` builds frames; `AppKernel.cpp` and tests refresh projection metrics.
- `FramePresenter.cpp` consumes `ProductGameplayProjectionFrame`.
- Grep proof: `rg -n "buildProductGameplayProjectionFrame|buildProductDebugProjectionWithNpcBehavior|applyGameplayProjectionMetrics|makeProductVulkanFrame|refreshProductGameplayProjectionMetrics" src/app/iggy3d tests/unit cmake CMakeLists.txt`.

## Invariants

- Projection refresh mirrors runtime/app facts; it does not advance simulation.
- Debug physics geometry is included only when developer tools and debug overlay are both enabled.
- HUD visibility follows active surface/input-owner policy and creative document editor gating.
- Metrics must clear when projection pointers are absent.

## Tests / Proof Commands

- `rg -n "product_vulkan_room_frame_tests|product_creative_world_launch_tests|product_primitive_draw_list_tests" cmake tests/unit`.
- `rg -n "productDrawPhysicsAabbDebugCount|productRenderBridgePhysicsAabbDebugCount|productVulkanRoomMesh" src/app/iggy3d tests/unit`.

## Nearby Files Usually Not Touched

- `src/projection/*` unless projection packet contracts change.
- `src/app/iggy3d/window/FramePresenter.*` unless frame consumption changes.
- Runtime AI/physics files unless source facts change.

## Update When

- Projection frame shape, HUD copy rules, metric mirrors, debug geometry gating, or frame input construction changes.

## Do Not Update When

- Runtime facts change without changing projected product-frame contracts.
