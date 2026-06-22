# Room Asset Packet 1 Render Proof Contract

Status: resolved decision/proof contract
Repo: `/Users/kogaryu/iggy3d`
Packet source: `docs/build_packets/room_asset_packet_1_map_toml_to_3d_room.md`
Resolved by: `e770a83 Add package room mesh rendering proof`

This contract locked what Room Asset Packet 1 was allowed to claim in receipts
and reports while the package-room branch could still fall back to proxy
primitive rendering. Current committed truth is Tier B: true static room mesh
draw proof is green at `e770a83`.

Verified Tier B receipt facts from the controller thread:

```text
rendering_path=package_room_meshes
record_mode=room_mesh_draws
room_static_mesh_count=11
mesh_draw_count=11
indexed_draw_count=11
vertex_buffer_uploaded=true
index_buffer_uploaded=true
depth_enabled=true
camera_projection=perspective
projection_application=single
result=pass
reason_code=package_room_meshes_presented
```

## Decision Summary

Room Asset Packet 1 has two acceptable proof tiers:

- Tier A: package-room asset proxy proof.
- Tier B: true static room mesh draw proof.

Tier A was minimally acceptable before `e770a83` if Builder Dex could not finish
real static room vertex/index buffers in Packet 1. Tier A had to be labeled
honestly as proxy rendering informed by package room assets.

Tier B is now the committed proof. Later packets may rely on package-driven room
geometry being rendered through vertex/index buffers, while still treating
content-owned collision/spatial surfaces as separate work.

Forbidden wording:

- Do not claim `package-driven static room meshes`.
- Do not claim `true room mesh draw`.
- Do not use `rendering_path=package_room_meshes`.
- Do not set `mesh_draw_count > 0`.
- Do not set `indexed_draw_count > 0`.
- Do not set `vertex_buffer_uploaded=true`.
- Do not set `index_buffer_uploaded=true`.

Those claims are forbidden when the command path for `scene.room.loaded` still
uses `recordProxyPrimitiveFrame` or any equivalent screen/clear-primitive proxy
path rather than binding room vertex/index buffers and issuing room indexed draw
commands.

## Tier A Contract: Package-Room Asset Proxy Proof

Tier A proves the package room asset is loaded, parsed, projected, and used to
influence a visible proxy frame. It does not prove true room mesh drawing.

Tier A is acceptable only if all of these are true:

- package asset refs load from `[[assets]]` in
  `fixtures/demos/first_room/package.iggy3d.toml`;
- room, material, and mesh TOML assets parse from package-local paths;
- `SceneRoomProjection` or an equivalent projection result carries room asset
  id, source subset, static mesh count, material count, and anchor count;
- Vulkan presents a visible proxy frame informed by room projection booleans;
- receipt names the path honestly as
  `rendering_path=package_room_asset_proxy`;
- receipt does not use `mesh_draw_count > 0` unless actual room mesh draw
  commands happened.

Required Tier A receipt fields:

```text
room_asset_loaded=true
room_asset_id=spawn_corridor
source_subset=spawn_room_corridor_stub
room_static_mesh_count=<count from asset/projection>
room_material_count=<count from asset/projection>
room_anchor_count=<count from asset/projection>
rendering_path=package_room_asset_proxy
record_mode=draw_primitives
proxy_floor_visible=true
proxy_room_bounds_visible=true
proxy_player_marker_visible=true
proxy_target_marker_visible=true
depth_enabled=false
mesh_draw_count=0
indexed_draw_count=0
vertex_buffer_uploaded=false
index_buffer_uploaded=false
first_room_visible=true
```

Tier A may include additional receipt fields such as `draw_count`,
`proxy_objective_marker_visible`, `fallback_room_proxy`, `source_toml`, or
`drawable_aspect`, but those fields must not imply true room mesh drawing.

Tier A green proof:

- package manifest includes room, mesh, and material asset refs;
- room/mesh/material parser tests or smoke output report success;
- projection reports `room_asset_loaded=true` and correct room counts;
- visible frame receipt reports `package_room_asset_proxy`;
- room proxy floor/bounds/player/target markers are visible;
- no receipt field claims actual mesh draw commands.

## Tier B Contract: True Static Room Mesh Draw Proof

Tier B proves the first package-driven room asset produced actual room geometry
draw commands.

Tier B requires all of these:

- CPU mesh data generated from room and mesh assets;
- material records resolved for room geometry;
- GPU vertex buffer uploaded for room geometry;
- GPU index buffer uploaded for room geometry;
- Vulkan command recording binds the room vertex/index buffers;
- Vulkan command recording issues indexed draw calls for room geometry;
- model/view/projection is applied exactly once;
- depth attachment exists, is cleared, and depth test/write are enabled for room
  geometry;
- visible frame draws floor, walls, one opening, one prop, key marker, and dummy
  marker.

Required Tier B receipt fields:

```text
rendering_path=package_room_meshes
record_mode=room_mesh_draws
room_asset_loaded=true
room_asset_id=spawn_corridor
source_subset=spawn_room_corridor_stub
room_static_mesh_count=<positive count>
room_material_count=<positive count>
room_anchor_count=<positive count>
mesh_draw_count=<positive count>
indexed_draw_count=<positive count>
vertex_buffer_uploaded=true
index_buffer_uploaded=true
depth_enabled=true
camera_projection=perspective
projection_application=single
floor_visible=true
wall_visible=true
opening_visible=true
prop_visible=true
key_marker_visible=true
dummy_marker_visible=true
first_room_visible=true
```

Tier B green proof:

- command recording path for package room does not call
  `recordProxyPrimitiveFrame`;
- command recording binds room vertex/index buffers;
- receipt reports positive `mesh_draw_count` and `indexed_draw_count`;
- receipt reports `record_mode=room_mesh_draws`;
- visual smoke proves a nonblank frame with floor, walls, opening, prop, key
  marker, and dummy marker;
- no runtime/save/replay state treats render receipt fields as truth.

## Current Source Audit After `e770a83`

Relevant current files:

- `/Users/kogaryu/iggy3d/src/render/vulkan/RenderLoop.cpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/CommandRecording.cpp`
- `/Users/kogaryu/iggy3d/src/projection/scene/SceneProjection.cpp`
- `/Users/kogaryu/iggy3d/src/projection/scene/SceneItem.hpp`
- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.hpp`
- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.cpp`
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`

Current implication:

- `SceneItem.hpp` defines `SceneRoomProjection` with room asset id, source
  subset, static mesh count, material count, anchor count, visibility booleans,
  and mesh summaries.
- `SceneProjection.cpp` projects runtime entities and currently keeps runtime
  truth separate from room asset facts.
- `RoomAsset.hpp` / `RoomAsset.cpp` define and parse `RoomAsset`,
  `RoomStaticMeshAsset`, `RoomAnchorAsset`, and `RoomOpeningAsset`.
- `spawn_corridor.room.iggy3d.toml` contains `spawn_corridor`, source subset
  `spawn_room_corridor_stub`, static room mesh records, opening `out`, and
  runtime anchors for `gold_key`, `training_dummy`, and
  `tactical_marker_alpha`.
- `BufferImageResources.cpp::createRoomMeshResources` expands projected room
  mesh items into room vertex/index buffers and indexed draw ranges.
- `RenderLoop.cpp` creates room mesh resources when `scene.room.loaded` is true,
  then records the package-room branch through `recordFirstRoomFrame`.
- `CommandRecording.cpp::recordProxyPrimitiveFrame` still exists for fallback
  proxy modes, but it is not the package-room mesh proof path.
- Current receipt labels `package_room_meshes`, `room_mesh_draws`,
  `mesh_draw_count`, `indexed_draw_count`, uploaded buffer booleans, depth,
  perspective, and single projection are now valid Tier B claims for Packet 1.

## Builder Dex Feedback Block

Builder Dex: Packet 1 proof-tier clarification is resolved. The committed Packet
1 state is Tier B. Preserve `rendering_path=package_room_meshes`,
`record_mode=room_mesh_draws`, positive `mesh_draw_count` and
`indexed_draw_count`, uploaded buffer booleans true, depth enabled true, and
visible floor/wall/opening/prop/key/dummy fields true. Future packets should not
rebuild this proof unless they are explicitly repairing a regression.

## Recommended Packet 2 Adjustment

Tier B landed for Packet 1.

- Packet 2 should be loader hardening plus authored spatial surface contract.
- Reason: true room drawing is proven, so the next risk is asset schema
  correctness, cross-reference validation, deterministic diagnostics, and
  content-owned spatial surfaces for later collision/movement/projectile work.

## No-Go Surfaces

- no JSON;
- no Blender runtime;
- no glTF importer;
- no tactical camera;
- no multi-room generation;
- no runtime-owned static room meshes;
- no receipt-as-save/replay truth;
- no Vulkan types in runtime/content/projection public headers;
- no old `/Users/kogaryu/iggy` runtime dependency.

## Acceptance Summary

Packet 1 is closed at Tier B. Any future regression that routes
`scene.room.loaded` back through proxy primitives must either fail the room mesh
smoke test or explicitly downgrade the receipt; it must not silently keep Tier B
receipt fields.
