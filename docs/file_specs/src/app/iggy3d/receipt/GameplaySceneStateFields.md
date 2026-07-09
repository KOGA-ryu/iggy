# File Spec

Files: `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`

Verified at: `0ba40cb9`

## Owns

- Receipt field emission for position HUD, collision surface proof, physics movement planner proof, gameplay targeting/outcome/proximity/objective/inventory state, NPC behavior proof, camera/viewport state, creative fly state, and Vulkan room mesh projection proof.
- Table-driven mapping from product window state to gameplay scene receipt keys.

## Does Not Own

- Position HUD construction.
- Physics movement planner execution.
- Targeting, interaction, combat, AI, camera, or renderer behavior.
- Vulkan room mesh generation.

## Reads

- `ProductAppWindowState` debug HUD, gameplay, viewport, NPC behavior, inventory/objective, creative fly, and room mesh proof fields.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate window, gameplay, viewport, or renderer state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `floatReceiptValue(...)`.
- `creativeFlyAnchorModeName(...)`.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductGameplaySceneStateFields(...)`.
- Focused proof: `rg -n "appendProductGameplaySceneStateFields|position_hud_visible|physics_movement_planner_status|product_vulkan_room_mesh_cpu_ready" src/app tests`.

## Invariants

- This appender is a scene-state receipt view only.
- Physics movement planner fields are read from previously recorded planner proof.
- Renderer/Vulkan room fields are proof of projection/presentation state, not render commands.
- Camera and creative fly fields stay off durable save/hash unless their owning packets explicitly opt in elsewhere.

## Tests / Proof Commands

- `rg -n "position_hud_visible|physics_movement_planner_status|product_vulkan_room_mesh_cpu_ready" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_vulkan_room_frame_tests|product_gameplay_controls_smoke|product_gameplay_tape_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/debug/*` unless HUD proof fields change.
- `src/app/iggy3d/gameplay/*` unless gameplay scene proof packets change.
- `src/app/iggy3d/window/*` and `src/render/*` unless Vulkan room proof fields change.

## Update When

- Gameplay scene receipt keys, source window fields, or scene-state receipt grouping changes.

## Do Not Update When

- Only runtime gameplay behavior, camera math, or renderer implementation changes without changing emitted receipt fields.
