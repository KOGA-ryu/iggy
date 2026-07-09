# File Spec

Files: `src/render/vulkan/BufferImageResources.hpp`, `src/render/vulkan/BufferImageResources.cpp`

Verified at: `3dab1aa4`

## Owns

- Vulkan buffer/depth/image resource creation and destruction for first-room and room-mesh rendering.
- CPU mesh emission for projected room meshes, grid/wall helpers, bean meshes, and debug gaze blades.
- Render receipt fields for this resource path.

## Does Not Own

- Scene projection truth, runtime AI truth, physics truth, or product UI state.
- Room asset loading or gameplay collision.
- Debug gaze semantics beyond drawing/coloring received roles.

## Reads

- `BufferImageResourcesCreateInfo`, `SceneRoomMeshItem` roles/geometry, Vulkan device/memory handles, and resource budgets.
- Mesh roles such as `floor`, `wall`, `ledge`, `npc_gaze_perceived`, `npc_gaze_blocked`, and `npc_gaze_scan`.

## Writes / Mutates

- Vulkan buffers/images/memory handles owned by `BufferImageResources`.
- CPU vertex/index/draw-range vectors during resource creation.
- `BufferImageResourcesResult` and render receipts.

## Calls Out To / Wires Out To

- Vulkan allocation/buffer/image helper paths.
- Bean mesh generation for player/NPC models.
- `appendGazeBlade(...)` for debug NPC vision roles emitted by scene projection.

## Called By / Entry Points

- `VulkanBackend.cpp` and `RenderLoop.cpp` create/update room resources.
- Tests include this file for room mesh and product Vulkan frame verification.
- Grep proof: `rg -n "BufferImageResources|npc_gaze_perceived|npc_gaze_blocked|npc_gaze_scan|appendGazeBlade" src/render src/projection tests/unit cmake CMakeLists.txt`.

## Invariants

- Renderer draws projected roles; it does not infer AI state.
- Failed gaze blade append is skipped and must not invalidate the frame.
- Hard render failures clear/return resource results through existing error paths.
- No runtime/session/app ownership moves into Vulkan resource code.

## Tests / Proof Commands

- `rg -n "product_vulkan_room_frame_tests|render_room_mesh_geometry_tests|render_memory_budget_policy_tests" cmake tests/unit`.
- `rg -n "npc_gaze_perceived|appendGazeBlade|colorForRoomRole" src/render/vulkan/BufferImageResources.cpp tests/unit`.

## Nearby Files Usually Not Touched

- `src/projection/scene/SceneProjection.*` unless role emission changes.
- Runtime AI files unless source debug facts change.
- Product/app frame code unless resource input contracts change.

## Update When

- Room mesh role rendering, gaze blade geometry/coloring, resource ownership, or receipt semantics change.

## Do Not Update When

- Runtime or projection semantics change without changing renderer input roles or resource behavior.
