# Room Asset Packet 1: Map TOML To 3D Room

Status: builder-ready planning packet
Repo: `/Users/kogaryu/iggy3d`
Branch: `iggy3d-main`

This is a content/render bridge packet. It converts a conservative subset of
prior TOML room/map authoring into the first package-driven static 3D room asset
for the visible Vulkan demo. It must not implement gameplay AI expansion, full
map generation, Blender runtime import, tactical camera, multiplayer, or a new
data format.

## 1. Exact Scope Statement

Build the first package-driven static 3D room asset path.

In scope:

- author a first-room room asset in TOML under the first-room fixture package;
- load immutable room, primitive mesh, and material asset records from package
  assets;
- project the loaded room asset into backend-neutral scene/render items;
- upload static room vertex/index buffers to Vulkan once per room asset load;
- draw floor, walls, one opening, at least one prop, the key marker, and the
  dummy/NPC marker with depth-tested perspective;
- keep the existing runtime entities, save/load, replay, command admission, and
  gameplay acceptance truth authoritative and green;
- produce deterministic key-value render receipts proving the static room asset
  path was used.

Out of scope:

- no gameplay AI expansion;
- no full proving-ground multi-room generation;
- no full old `iggy` ASCII fixture conversion;
- no Blender runtime dependency;
- no glTF importer or glTF dependency in Packet 1;
- no JSON;
- no renderer-owned gameplay state;
- no tactical overhead camera implementation.

## 2. Source Data Decision

Source files inspected for planning:

- `/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml`
- `/Users/kogaryu/edi/docs/examples/guard-antechamber.room.toml`
- `/Users/kogaryu/iggy/engine/content/demos/product_loop_demo/scenario.toml`
- `/Users/kogaryu/iggy/engine/tests/fixtures/runtime/ascii_source_plan/locked_door_key_room.toml`
- `/Users/kogaryu/iggy/engine/tests/fixtures/runtime/ascii_source_plan/valid_guard_room.toml`

Decision: use `provingground.map.toml` as the source seed, but convert only a
one-room/corridor subset first.

First subset:

- `map.room.0.name = "spawn"`;
- dimensions: width `20 ft`, height `18 ft`, origin `(2,44)`, wall thickness
  `1 ft`, wall material `stone`;
- E plug `out`, type `door`, offset `9 ft`, opening width `10 ft`;
- connection context from `map.connection.0` only as an east corridor stub,
  not the full two-turn corridor or junction;
- spawn feature `player_spawn`;
- one packet-local visual blockout prop, such as `spawn_crate`; the
  proving-ground spawn room does not author props, so the prop must be marked as
  iggy3d Packet 1 blockout content rather than source-derived EDI content;
- compatibility anchors for current iggy3d first-room gameplay: `gold_key`,
  `training_dummy`, and `tactical_marker_alpha`.

Why this is lowest risk:

- the EDI proving-ground file already has real dimensions, openings, wall
  thickness, material tags, and provenance;
- the spawn room is rectangular and self-contained, so a builder can generate
  floor/wall/opening mesh without solving graph layout;
- the corridor stub proves openings/plugs without requiring full map routing;
- current `iggy3d` runtime acceptance can keep using its existing first-room
  entities while render content becomes real authored room geometry;
- the guard antechamber is useful as a smaller shape reference, but has no
  gameplay/key/dummy continuity;
- old `iggy` ASCII fixtures are valuable gameplay references, but they are tile
  plans with explicit `no_claims.runtime_truth=false` and are not the best first
  3D geometry source.

## 3. New And Changed Files

Must add fixture/package assets:

- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/assets/meshes/room_primitives.meshes.iggy3d.toml`
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/assets/materials/room_basic.materials.iggy3d.toml`

Must modify fixture package manifest:

- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml`

Must add or modify content asset loading:

- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.hpp`
- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.cpp`
- `/Users/kogaryu/iggy3d/src/content/assets/MeshAsset.hpp`
- `/Users/kogaryu/iggy3d/src/content/assets/MeshAsset.cpp`
- `/Users/kogaryu/iggy3d/src/content/assets/MaterialAsset.hpp`
- `/Users/kogaryu/iggy3d/src/content/assets/MaterialAsset.cpp`
- `/Users/kogaryu/iggy3d/src/content/PackageLoader.hpp`
- `/Users/kogaryu/iggy3d/src/content/PackageLoader.cpp`
- `/Users/kogaryu/iggy3d/src/content/PackageManifest.hpp`
- `/Users/kogaryu/iggy3d/src/content/PackageValidator.hpp`
- `/Users/kogaryu/iggy3d/src/content/PackageValidator.cpp`

Must add or modify projection:

- `/Users/kogaryu/iggy3d/src/projection/scene/SceneItem.hpp`
- `/Users/kogaryu/iggy3d/src/projection/scene/SceneProjection.hpp`
- `/Users/kogaryu/iggy3d/src/projection/scene/SceneProjection.cpp`
- optional if the shape is cleaner:
  `/Users/kogaryu/iggy3d/src/projection/scene/RoomProjection.hpp`
  and `/Users/kogaryu/iggy3d/src/projection/scene/RoomProjection.cpp`

Must add or modify Vulkan render path:

- `/Users/kogaryu/iggy3d/src/render/vulkan/BufferImageResources.hpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/BufferImageResources.cpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/CommandRecording.hpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/CommandRecording.cpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/RenderLoop.hpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/RenderLoop.cpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/VulkanBackend.hpp`
- `/Users/kogaryu/iggy3d/src/render/vulkan/VulkanBackend.cpp`
- optional if static room resources need their own owner:
  `/Users/kogaryu/iggy3d/src/render/vulkan/RoomMeshResources.hpp`
  and `/Users/kogaryu/iggy3d/src/render/vulkan/RoomMeshResources.cpp`

Must add or modify tests/smokes:

- `/Users/kogaryu/iggy3d/tests/smoke/package_room_asset_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_room_asset_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/room_asset_loader_tests.cpp`
- optional if projection grows a separate owner:
  `/Users/kogaryu/iggy3d/tests/unit/room_projection_tests.cpp`

Must update build registration:

- `/Users/kogaryu/iggy3d/CMakeLists.txt`
- `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`
- other existing CMake include files only if the current target layout requires
  them.

Current source anchors Builder must preserve:

- `/Users/kogaryu/iggy3d/src/content/PackageManifest.hpp` already defines
  `PackageManifest::assets` as `std::vector<PackageAssetRef>` with `id` and
  `path`. Extend this path; do not create a parallel package manifest.
- `/Users/kogaryu/iggy3d/src/content/PackageLoader.cpp` currently accepts only
  `[package]` plus `[[assets]]` records with `id` and `path`, and validates
  relative paths. Builder should add first-room asset refs through the existing
  `[[assets]]` mechanism.
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml`
  currently lists only the scenario. Packet 1 should add the room/material/mesh
  refs there instead of hardcoding fixture asset paths in the app or renderer.
- `/Users/kogaryu/iggy3d/src/content/FixtureScenarioLoader.cpp` already parses
  float vectors and arrays for scenario TOML. Reuse the local parsing style or
  small shared helpers; do not introduce JSON or a large new dependency for this
  packet.

## 4. Proposed TOML Semantics

Use TOML only. Do not introduce JSON.

Primary room asset:

```toml
[room]
id = "spawn_corridor"
version = 1
units = "ft"
source = "edi.provingground"
source_file = "/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml"
source_subset = "map.room.0 spawn + map.connection.0 corridor_stub"
source_notes = "Converted subset only; not full proving-ground map."

[conversion]
source_axis_x = "east"
source_axis_y = "south"
world_axis_x = "east"
world_axis_y = "up"
world_axis_z = "south"
feet_to_meters = 0.3048
origin_policy = "room_origin_to_world_origin"
floor_world_y = 0.0
wall_height_meters = 2.75
wall_thickness_source_ft = 1.0

[[materials]]
id = "stone_wall"
kind = "vertex_color"
color = [0.42, 0.43, 0.46]

[[materials]]
id = "floor_stone"
kind = "vertex_color"
color = [0.30, 0.32, 0.34]

[[primitive_meshes]]
id = "floor_rect"
kind = "box"
size_ft = [20.0, 0.10, 18.0]
material = "floor_stone"

[[primitive_meshes]]
id = "wall_segment"
kind = "box"
material = "stone_wall"

[[static_meshes]]
id = "spawn_floor"
mesh = "floor_rect"
position_ft = [10.0, 0.0, 9.0]
rotation_degrees = [0.0, 0.0, 0.0]
scale = [1.0, 1.0, 1.0]
collision = "walkable_floor"

[[static_meshes]]
id = "north_wall"
mesh = "wall_segment"
position_ft = [10.0, 0.0, 0.5]
size_ft = [20.0, 9.0, 1.0]
material = "stone_wall"
collision = "solid"

[[openings]]
id = "out"
edge = "E"
kind = "door"
offset_ft = 9.0
width_ft = 10.0
connects_to = "corridor_stub"

[[doors]]
id = "spawn_out_frame"
opening = "out"
state = "open"
material = "stone_wall"

[[props]]
id = "spawn_crate"
source = "iggy3d.room_asset_packet_1_blockout"
source_note = "Packet-local visual prop; not authored in the proving-ground spawn room."
asset_ref = "dungeon.crate"
primitive = "box"
position_ft = [4.0, 0.0, 16.0]
size_ft = [2.0, 2.0, 2.0]
material = "prop_wood"
collision = "solid"

[[anchors]]
id = "player_spawn"
kind = "spawn"
position_ft = [10.0, 0.0, 9.0]

[[anchors]]
id = "gold_key"
kind = "pickup"
position_ft = [12.0, 0.0, 9.0]
runtime_stable_name = "gold_key"

[[anchors]]
id = "training_dummy"
kind = "npc"
position_ft = [15.0, 0.0, 9.0]
runtime_stable_name = "training_dummy"

[[anchors]]
id = "tactical_marker_alpha"
kind = "marker"
position_ft = [12.0, 0.0, 12.0]
runtime_stable_name = "tactical_marker_alpha"
```

Rules:

- quoted strings remain strings; numeric TOML values are allowed for new iggy3d
  room assets;
- source provenance is required for this packet;
- primitive-generated meshes are authored content, not runtime gameplay state;
- anchors connect immutable room authoring to existing runtime stable names;
- collision bounds are coarse AABBs only in this packet;
- doors/openings are visual/static semantics only unless runtime gameplay later
  claims them.

## 5. Coordinate Conversion

Source coordinate rules:

- EDI source `x` is east/right;
- EDI source `y` is south/down on the map;
- source distances are feet;
- source room-local origin is the room NW corner;
- source map origin is not directly preserved for this subset.

iggy3d world conversion:

- source `x` maps to world `x`;
- source `y` maps to world `z`;
- floor maps to world `y = 0`;
- `feet_to_meters = 0.3048`;
- world axes stay `X right/east`, `Y up`, `Z south`;
- camera forward is a camera vector supplied by `FrameInput.camera`, not a
  content-axis alias. Do not assume `+Z` is always camera-forward;
- if the demo starts from the room center, default yaw should be chosen
  deliberately by the app camera and must not change the content conversion;
- no handedness flip is allowed in content conversion;
- first packet origin policy: room-local NW corner maps to world `(0,0,0)`
  before scale, then all authored positions are converted relative to that;
- authored room center `(width/2,height/2)` becomes world
  `(width_ft * 0.3048 / 2, 0, height_ft * 0.3048 / 2)`;
- wall height default: `2.75 m`;
- wall thickness default: source wall thickness in feet converted to meters;
- corridor stub length default: `10 ft` unless overridden;
- camera convention remains renderer contract: perspective camera consumes
  `FrameInput.camera`; renderer must not reinterpret world units.

## 6. Data Ownership

- Package/asset files own immutable authored content.
- `src/content/assets/*` owns parsing, validation, and immutable asset records.
- Runtime `WorldState` owns dynamic entity state only: active flags, transforms,
  inventory/objective/combat truth, and command effects.
- Room assets may provide anchor defaults, but runtime scenario still owns which
  dynamic entities exist and their authoritative state.
- `SceneProjection` maps runtime state plus room asset references into
  backend-neutral render items.
- Vulkan owns GPU resources, draw records, depth images, command buffers, and
  projection matrices consumed for drawing.
- No runtime/content/projection/save header may expose Vulkan or SDL types.
- Renderer receipts are diagnostics only; they are not save/replay/hash truth.

## 7. Render Path Plan

Implementation path:

1. `PackageLoader` reads package asset refs for room, mesh, and material TOMLs.
2. `RoomAsset` parses room metadata, static meshes, primitive mesh refs,
   materials, collision bounds, spawn/anchor records, openings/plugs, and source
   provenance.
3. `MeshAsset` resolves primitive generated mesh records into CPU vertex/index
   data. First packet supports boxes/rectangles only.
4. `MaterialAsset` resolves vertex-color materials only.
5. `SceneProjection` carries room asset render references in stable order. Do
   not make runtime world entities own static wall/floor mesh state.
6. `VulkanBackend` or `RoomMeshResources` uploads combined static room geometry
   once when the room asset changes or backend initializes.
7. `CommandRecording` records indexed draws for static room meshes and existing
   runtime markers.
8. `RenderLoop` reports the room path in receipts:
   `rendering_path=package_room_meshes`.

Required first frame:

- floor visible;
- walls visible;
- one opening/door frame visible;
- at least one prop visible;
- key marker visible;
- training dummy or NPC marker visible;
- depth enabled;
- camera projection perspective;
- draw count greater than zero.

Pipeline:

- keep flat/vertex-color material pipeline;
- no textures;
- no descriptor sets unless already required by current Vulkan code;
- model/view/projection via push constants or existing matrix path;
- depth testing required;
- front/back culling must not hide interior walls in the first proof. If needed,
  use two-sided wall proxy geometry or disable culling for room material and
  report it.

## 8. Perspective Correctness Plan

To avoid perspective warping:

- use a Vulkan-correct perspective matrix with depth range `0..1`;
- derive aspect ratio from current drawable size, not window logical size;
- apply projection exactly once: `clipFromModel = clipFromWorld * modelFromLocal`;
- do not bake perspective into mesh vertices;
- do not multiply by projection in both app and Vulkan command code;
- depth buffer is required and must be cleared each frame;
- default vertical FOV: `65` to `70` degrees;
- near plane: `0.05` to `0.10`;
- far plane: at least `100 m` for this room;
- first-person camera height around `1.6 m`;
- tactical mode later uses an orthographic camera and is out of scope here.

Receipt must include:

```text
camera_projection=perspective
camera_fov_degrees=<value>
drawable_aspect=<value>
depth_enabled=true
projection_application=single
```

## 9. Acceptance Commands And Receipts

Default build:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Targeted tests:

```sh
ctest --test-dir build --output-on-failure -R 'room_asset|package_visual|vulkan_first_room|render|projection'
```

Product app root launch:

```sh
cmake --build build --target iggy3d_app -j 8
./build/iggy3d --window --input auto --save-root "$HOME/.iggy3d/saves" --print-render-receipt
```

No-window product receipt:

```sh
./build/iggy3d --no-window --print-render-receipt
```

Product scripted gameplay receipt:

```sh
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

Compatibility/Test Shell:

Use the visual shell only for package-room renderer behavior that has not been
migrated to the product app yet.

```sh
./build/iggy3d_visual_demo \
  --renderer vulkan \
  --window \
  --frames 3 \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --print-render-receipt
```

Compatibility/Test Shell manual inspect command:

```sh
./build/iggy3d_visual_demo \
  --renderer vulkan \
  --window \
  --interactive \
  --hold-seconds 10 \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --print-render-receipt
```

Headless/runtime stability:

```sh
./build/iggy3d_headless_demo \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --summary fixtures/demos/first_room/expected_summary.txt \
  --save /tmp/iggy3d_room_asset_packet1_runtime.save

./build/iggy3d_replay_tool \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --save /tmp/iggy3d_room_asset_packet1_runtime.save \
  --expect-summary fixtures/demos/first_room/expected_summary.txt
```

Required receipt fields:

```text
room_asset_loaded=true
room_asset_id=spawn_corridor
room_asset_version=1
source_toml=/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml
source_subset=spawn_room_corridor_stub
room_static_mesh_count=<integer greater than 0>
room_material_count=<integer greater than 0>
room_anchor_count=<integer greater than 0>
mesh_draw_count=<integer greater than 0>
draw_count=<integer greater than 0>
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
rendering_path=package_room_meshes
result=pass|fail|skip
reason_code=<stable_lower_snake_case>
```

Firewall scans:

```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
rg -n 'nlohmann|rapidjson|\.json\b|JSON|Json' fixtures src/content src/projection src/render tests CMakeLists.txt cmake
git diff --check
```

Warnings-as-errors:

```sh
cmake -S . -B build-werror -DIGGY3D_WARNINGS_AS_ERRORS=ON
cmake --build build-werror
ctest --test-dir build-werror --output-on-failure
```

## 10. No-Go Surfaces

Explicitly deferred:

- full Blender importer;
- Blender runtime dependency;
- glTF importer or glTF dependency;
- real materials/textures;
- animation/skinning;
- navmesh/pathfinding;
- collision-perfect geometry;
- full proving-ground multi-room generation;
- full old `iggy` ASCII fixture conversion;
- tactical camera implementation;
- multiplayer integration;
- AI behavior expansion;
- JSON formats or JSON libraries;
- renderer writes to runtime save/replay/hash truth.

## 11. Builder Execution Order

Use large but ordered phases:

1. Fixture/package authoring:
   - add room, mesh, and material TOML files;
   - add package asset refs;
   - keep old scenario runtime facts intact.

2. Content asset loader:
   - implement `RoomAsset`, `MeshAsset`, `MaterialAsset` immutable records;
   - parse only the first packet TOML shape;
   - validate source provenance, dimensions, material refs, mesh refs, anchors,
     openings, and collision bounds;
   - reject invalid dimensions, duplicate ids, missing refs, non-finite values,
     and path escapes with deterministic statuses.

3. Projection bridge:
   - carry loaded room asset refs into scene projection;
   - keep dynamic runtime entities separate from static room meshes;
   - map anchors to runtime stable names for diagnostics only.

4. Vulkan resource upload:
   - convert primitive room meshes to CPU vertex/index arrays;
   - upload combined static mesh buffers once;
   - preserve existing marker/proxy rendering while adding room static mesh draws.

5. Command recording/render loop:
   - record room static mesh draws with depth;
   - ensure camera matrix is applied once;
   - emit receipt fields.

6. Tests and receipts:
   - add loader tests;
   - add package room asset smoke;
   - add visual room asset smoke;
   - run full runtime and replay proofs.

Stop after Packet 1 and report. Do not continue into multi-room generation.

## 12. Risks And Compute Costs

Parse cost:

- room TOML is small and should parse during package load;
- acceptable first-packet parse budget: under a few milliseconds on developer
  machines;
- parsing must not occur per frame.

Mesh memory cost:

- first room should stay under a few thousand vertices and indices;
- acceptable GPU memory for room static mesh buffers: well under 10 MB;
- combine small primitive meshes into one or a small number of buffers.

GPU upload timing:

- upload static room geometry at Vulkan backend initialization or package-room
  resource initialization;
- no per-frame static room buffer allocation;
- no per-frame shader/pipeline creation.

Per-frame draw cost:

- expected static mesh draw count can be more than one, but should stay small;
- acceptable first packet: floor, walls/opening, one prop, and markers;
- depth clear and draw should fit the existing first visual demo budget.

Perspective/camera risk:

- largest risk is double-applying projection or using screen-space proxy
  coordinates. The acceptance receipt and visual smoke must prove
  `camera_projection=perspective` and `projection_application=single`.

Content ownership risk:

- do not promote room static meshes into `WorldState` entities. Runtime state
  remains dynamic truth; room assets remain immutable render/content truth.

## Builder Handoff

Packet title:

```text
Room Asset Packet 1: Map TOML To 3D Room
```

Builder green condition:

- package loads the new room asset;
- Vulkan visual demo renders package-driven static room meshes, not only
  hardcoded/proxy rectangles;
- first visible frame shows floor, walls, one opening, one prop, key marker, and
  dummy/NPC marker;
- receipt proves room asset load, mesh draw count, perspective projection, depth,
  source TOML provenance, and `first_room_visible=true`;
- headless demo and replay proof remain green;
- firewall/no-JSON scans remain clean.

Expected Builder Dex final report:

1. Files changed.
2. Room subset converted and exact source provenance.
3. Room TOML/mesh/material ids added.
4. Loader/projection/render ownership summary.
5. Visual receipt with required fields.
6. Headless/replay proof result.
7. Full CTest, targeted tests, Werror, firewall/no-JSON scans.
8. Remaining blocker or `No remaining blocker in this packet.`
