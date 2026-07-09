# File Spec

Files: `src/projection/scene/SceneProjection.hpp`, `src/projection/scene/SceneProjection.cpp`

Verified at: `3dab1aa4`

## Owns

- `SceneProjectionConfig`, `SceneProjectionResult`, and `buildSceneProjection(...)`.
- Projection of runtime entities and room asset meshes into app/render-facing scene items.
- Optional NPC vision debug "gaze blade" room meshes.

## Does Not Own

- Runtime simulation truth, AI perception decisions, render buffer creation, or Vulkan colors.
- Room asset loading or save serialization.
- Debug HUD text.

## Reads

- `SessionState` world/entities/camera/AI mirrors and optional `RoomAsset`.
- Entity interaction/targeting state and room static meshes/anchors/material ids.

## Writes / Mutates

- Returned scene projection items, room projection meshes, counts, camera facts, and optional debug gaze mesh items.
- No session/world/room mutation.

## Calls Out To / Wires Out To

- Uses bean model ids for player/NPC model refs.
- Emits room mesh roles such as `floor`, `wall`, `ledge`, `npc_gaze_perceived`, `npc_gaze_blocked`, and `npc_gaze_scan` for render consumption.

## Called By / Entry Points

- Product/gameplay/creative projection refresh paths and render/frame-input tests.
- `BufferImageResources.cpp` consumes room mesh roles from `SceneProjectionResult`.
- Grep proof: `rg -n "buildSceneProjection|includeNpcVisionDebug|npc_gaze_perceived|npc_gaze_blocked|npc_gaze_scan" src tests cmake CMakeLists.txt`.

## Invariants

- `includeNpcVisionDebug` is off by default.
- Gaze meshes are projection/debug geometry only; they do not feed AI perception.
- Gaze role is `perceived` from `lastPerceived`, `blocked` from radius/cone/vertical/LOS blocked, otherwise `scan`.
- Projection should not become runtime state or save/hash truth.

## Tests / Proof Commands

- `rg -n "projection_tests|render_room_mesh_geometry_tests" cmake tests/unit`.
- `rg -n "npc_gaze_perceived|npc_gaze_blocked|npc_gaze_scan" tests/unit/projection_tests.cpp src/projection src/render`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/AiState.hpp` unless AI mirror fields change.
- `src/render/vulkan/BufferImageResources.*` unless room mesh role rendering changes.
- `src/projection/debug/*` unless debug item projection changes.

## Update When

- Scene result/config shape, entity kind mapping, room mesh role projection, or NPC gaze debug emission changes.

## Do Not Update When

- Renderer implementation changes without changing scene projection contracts.
