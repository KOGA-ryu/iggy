# Room Asset Packet 2: Loader Hardening And Spatial Surface Contract

Status: builder-ready packet spec
Repo: `/Users/kogaryu/iggy3d`
Branch: `iggy3d-main`
Head accounted for: `e770a83 Add package room mesh rendering proof`

This packet starts after the true static room mesh draw gate is green. It must
not rebuild the package room mesh draw path. It hardens content loading and adds
authored spatial surface semantics needed by later collision, movement, slope,
and projectile packets.

## 1. Exact Scope Statement

Package room mesh drawing is already green at `e770a83` and must remain green.
The known proof command is:

```sh
cmake --build build --target iggy3d_app -j 8
./build/iggy3d --window --input auto --save-root "$HOME/.iggy3d/saves" --print-render-receipt
```

No-window product receipt proof:

```sh
./build/iggy3d --no-window --print-render-receipt
```

Product scripted gameplay receipt:

```sh
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

Compatibility/Test Shell command for package-room renderer behavior not yet
migrated to the product app:

```sh
./build/iggy3d_visual_demo --renderer vulkan --window --frames 3 --package fixtures/demos/first_room/package.iggy3d.toml --print-render-receipt
```

The committed Tier B proof includes:

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

In scope:

- harden room, mesh, material, and package asset parsing with exact status and
  diagnostic expectations;
- add authored spatial surface records to the first-room room asset TOML;
- define content-owned traversal tags and collision masks for walkable floors,
  blocker walls, projectile blockers, and openings/non-blockers;
- parse and validate spatial surfaces as content data;
- expose spatial surface counts in package/load or content diagnostics;
- preserve the existing room mesh render proof and receipt fields;
- add unit tests for valid and invalid spatial surface records.

Out of scope:

- no movement clamping;
- no runtime collision query system;
- no projectile simulation;
- no tactical camera or tactical preview;
- no renderer-driven gameplay decisions;
- no save/replay schema changes.

Movement clamping is not inseparable from this packet. This packet gives later
runtime movement code an exact authored surface contract, but it does not decide
movement legality.

## 2. Current Source Audit

Current files and ownership after `e770a83`:

- `fixtures/demos/first_room/package.iggy3d.toml`
  - Owns package id `iggy3d.first_room`, schema/runtime schema `1`, scenario
    path, and three package-local `[[assets]]` refs:
    `room.spawn_corridor`, `mesh.room_primitives`, and `material.room_basic`.
  - This packet adds no new manifest type; it may keep using the existing room
    asset path and add spatial data inside the room asset file.
- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
  - Owns room id `spawn_corridor`, version, units, source metadata, conversion
    metadata, 11 `[[static_meshes]]`, opening `out`, and gameplay anchors for
    `player_spawn`, `gold_key`, `training_dummy`, and
    `tactical_marker_alpha`.
  - It does not yet own spatial surface tables.
- `fixtures/demos/first_room/assets/meshes/room_primitives.meshes.iggy3d.toml`
  - Owns primitive box mesh definitions: `floor_rect`, `wall_segment`,
    `door_frame`, and `crate_box`.
  - It does not own gameplay collision or traversal semantics.
- `fixtures/demos/first_room/assets/materials/room_basic.materials.iggy3d.toml`
  - Owns vertex-color material records: `stone_wall`, `floor_stone`,
    `prop_wood`, and `opening_trim`.
  - It does not own collision masks, movement roles, or projectile rules.
- `src/content/assets/RoomAsset.hpp`
  - Owns `RoomAsset`, `RoomStaticMeshAsset`, `RoomAnchorAsset`,
    `RoomOpeningAsset`, and `RoomAssetParseResult`.
  - Current result shape is `bool ok`, string `reason`, and parsed `RoomAsset`.
  - This packet should add `RoomSpatialSurface` types here or in the new
    `RoomSpatialSurface.hpp` if keeping spatial parsing isolated is cleaner.
- `src/content/assets/RoomAsset.cpp`
  - Owns the current small TOML parser for `[room]`, `[conversion]`,
    `[[static_meshes]]`, `[[anchors]]`, and `[[openings]]`.
  - It rejects unsupported tables/keys, non-finite values, empty required ids,
    and invalid positive sizes.
  - This packet should add `[[spatial_surfaces]]` parsing or delegate those
    rows to `RoomSpatialSurface.cpp`.
- `src/content/assets/MeshAsset.hpp` and `MeshAsset.cpp`
  - Own primitive mesh library records and parse `[[primitive_meshes]]` with
    `id`, `kind`, `size_ft`, and `material`.
  - This packet should add duplicate-id and invalid reference coverage through
    tests or validator logic, but should not move collision semantics into mesh
    primitives.
- `src/content/assets/MaterialAsset.hpp` and `MaterialAsset.cpp`
  - Own vertex-color material records and parse `[[materials]]` with `id`,
    `kind`, and `color`.
  - This packet may harden duplicate material ids and invalid colors; material
    records remain visual content only.
- `src/content/PackageLoader.hpp`
  - Owns `PackageLoadStatus`, `PackageLoadRequest`, `PackageLoadResult`, and
    the public `loadPackage` / `parsePackageText` boundary.
  - Current `PackageLoadResult` already carries `rooms`, `meshes`, and
    `materials`.
- `src/content/PackageLoader.cpp`
  - Owns package TOML parsing, relative path validation, scenario parser status
    mapping, and asset file dispatch by filename segment.
  - Current asset parser failures collapse to `PackageLoadStatus::ParseError`
    with parser-owned string reason as diagnostic code.
  - This packet should keep that public boundary stable unless adding exact
    asset hardening statuses is explicitly cheaper than reason-code tests.
- `src/projection/scene/SceneItem.hpp`
  - Owns `SceneRoomProjection` with asset id, source subset, static mesh count,
    material count, anchor count, visibility booleans, and room mesh summaries.
  - This packet may add spatial debug/readout counts later, but projection must
    not decide movement or hits.
- `src/projection/scene/SceneProjection.hpp` and `SceneProjection.cpp`
  - Own runtime entity projection from `SessionState`.
  - Room projection population is currently driven by package/runtime app path,
    not by runtime world ownership.
- `src/render/vulkan/BufferImageResources.hpp` and
  `src/render/vulkan/BufferImageResources.cpp`
  - Own Vulkan buffer/image resources and `createRoomMeshResources` for package
    room geometry.
  - Current room mesh resource creation expands `SceneRoomProjection::meshes`
    into boxes, uploads room vertex/index buffers, and records indexed draw
    ranges.
  - This packet must not add collision decisions here.
- `src/render/vulkan/RenderLoop.cpp`
  - Owns Vulkan frame orchestration and receipt fields.
  - Current package-room path creates room mesh resources, records
    `recordFirstRoomFrame`, reports `rendering_path=package_room_meshes`,
    `record_mode=room_mesh_draws`, positive mesh/indexed draw counts, uploaded
    buffer booleans, depth, perspective, and single projection.
  - `recordProxyPrimitiveFrame` still exists for fallback/proxy modes, but it is
    not the package-room mesh proof path.
- `src/render/vulkan/CommandRecording.cpp`
  - Owns command recording. `recordFirstRoomFrame` records the mesh path used by
    package-room rendering; `recordProxyPrimitiveFrame` remains a separate
    fallback/proxy path.
- Current tests/smokes:
  - `tests/unit/room_asset_loader_tests.cpp` proves the first-room package loads
    room, mesh, material, opening, anchor, and static mesh role facts.
  - `tests/smoke/package_visual_room_asset_smoke.cpp` proves the Tier B Vulkan
    receipt fields for package room meshes.
  - `tests/unit/projection_tests.cpp` loads the package and exercises projection
    integration.
  - `ctest --test-dir build --output-on-failure` passed `40/40` before commit
    `e770a83`.

## 3. Spatial TOML Semantics To Add

Use TOML only. Do not introduce JSON or a second data format.

Add zero or more `[[spatial_surfaces]]` rows to
`spawn_corridor.room.iggy3d.toml`. For Packet 2, first-room fixture must include
at least:

- one walkable floor surface;
- one blocker wall surface;
- one projectile blocker surface;
- one opening/non-blocker surface.

Exact accepted table:

```toml
[[spatial_surfaces]]
id = "spawn_floor_walkable"
source_static_mesh = "spawn_floor"
shape = "box"
role = "walkable"
points_ft = [
  [0.0, 0.05, 0.0],
  [20.0, 0.05, 0.0],
  [20.0, 0.05, 18.0],
  [0.0, 0.05, 18.0],
]
normal = [0.0, 1.0, 0.0]
traversal_tags = ["walkable"]
collision_mask = ["actor"]
blocks_actor = false
blocks_projectile = false
opening_id = ""
```

Required keys for every row:

- `id`: non-empty unique string within the room asset;
- `source_static_mesh`: non-empty id of an existing `[[static_meshes]]` row;
- `shape`: first-build accepted values are exactly `box`, `plane`, or
  `opening`;
- `role`: first-build accepted values are exactly `walkable`, `blocker`,
  `projectile_blocker`, or `opening`;
- `points_ft`: inline array of 3 or 4 point arrays, each exactly three finite
  numbers in room source feet; parser converts to meters;
- `normal`: exactly three finite numbers in world coordinates; must normalize
  to non-zero length;
- `traversal_tags`: inline string array preserving document order;
- `collision_mask`: inline string array preserving document order;
- `blocks_actor`: boolean;
- `blocks_projectile`: boolean;
- `opening_id`: string; empty for non-opening surfaces, required to match an
  existing opening id when `role = "opening"`.

Accepted traversal tags:

- `walkable`
- `blocker`
- `projectile_blocker`
- `opening`
- `no_player`
- `debug_only`

Accepted collision mask tokens:

- `actor`
- `projectile`
- `sight`

Role semantics:

- `role = "walkable"` means the surface may be considered by future movement
  ground queries. It must include traversal tag `walkable`, `blocks_actor=false`,
  and a normal with positive world-up dot.
- `role = "blocker"` means the surface blocks actor capsule movement. It must
  include traversal tag `blocker` and `blocks_actor=true`.
- `role = "projectile_blocker"` means the surface blocks projectile collision
  queries. It must include traversal tag `projectile_blocker` and
  `blocks_projectile=true`. It may also block actors only when
  `blocks_actor=true`.
- `role = "opening"` means the surface marks an opening/non-blocker span. It
  must include traversal tag `opening`, `blocks_actor=false`,
  `blocks_projectile=false`, and a non-empty valid `opening_id`.

Elevation/world-y ownership:

- source `points_ft` convert through existing room conversion feet-to-meters;
- world `y` is actual height;
- no render-only offset may change spatial surface height;
- `normal` is already in world coordinates after conversion policy.

Slope metadata:

- Packet 2 derives slope angle from `normal` and world up `(0, 1, 0)` for tests
  and diagnostics only;
- `max_walkable_slope` is not authored here and belongs to later runtime
  movement params;
- this packet does not accept content-authored slope pass/fail decisions.

## 4. Data Ownership Rules

- Content owns authored spatial surfaces, traversal tags, collision masks,
  opening links, projectile blocker roles, and source mesh references.
- Runtime later owns collision query views, movement params, movement decisions,
  projectile simulation, impact events, and deterministic tick results.
- Projection may expose debug/readout facts such as spatial surface counts,
  sampled heights, normals, slope decisions, projectile arcs, and impact
  markers. Projection cannot decide gameplay legality.
- Vulkan draws room meshes and debug lines/markers only. It never decides hits,
  reach, movement, slope acceptance, projectile collision, or command outcomes.
- Save/replay truth must not include render receipts, GPU buffers, or debug-only
  projection fields. Later runtime collision/projectile packets must explicitly
  add durable runtime state if needed.

## 5. Compute Costs

- Room/mesh/material/spatial parse cost is O(file bytes + table count).
- Spatial duplicate-id validation is O(surface count squared) in fixture order
  for deterministic first-failing diagnostics.
- Cross-reference validation to `source_static_mesh` and `opening_id` is
  O(surface count * referenced record count) for the first build.
- Finite point/normal validation is O(total surface point count).
- Future runtime query cost may start as O(surface count) scans per height,
  ray, or sweep query.
- No broadphase, navmesh, physics engine, or acceleration structure is needed
  for one room plus corridor stub.

## 6. Builder Implementation Plan

Likely files to modify:

- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
  - Add `[[spatial_surfaces]]` rows after openings or after static meshes.
  - Include deterministic ids for `spawn_floor_walkable`,
    `north_wall_actor_blocker`, `east_opening_non_blocker`, and
    `crate_projectile_blocker` or an equivalent first-room prop/wall blocker.
  - Keep existing static mesh, opening, and anchor ids unchanged.
- `src/content/assets/RoomAsset.hpp`
  - Add `std::vector<RoomSpatialSurface> spatialSurfaces;` to `RoomAsset`, or
    include `RoomSpatialSurface.hpp` and store the vector there.
  - Do not expose renderer handles, runtime collision query objects, or Vulkan
    types.
- `src/content/assets/RoomAsset.cpp`
  - Recognize `[[spatial_surfaces]]`.
  - Parse required keys with the same deterministic local parser style.
  - Convert `points_ft` to meters.
  - Normalize or validate `normal` and store a finite world-space normal. If the
    stored value is normalized by the parser, document and test that exact
    behavior.
  - Reject unsupported keys/tables using stable `reason` strings.
  - Validate required fields before returning `ok=true`.
- `src/content/assets/RoomSpatialSurface.hpp`
  - New header if spatial types are split out.
  - Define:

    ```cpp
    enum class RoomSpatialSurfaceShape : std::uint8_t {
      Box,
      Plane,
      Opening,
    };

    enum class RoomSpatialSurfaceRole : std::uint8_t {
      Walkable,
      Blocker,
      ProjectileBlocker,
      Opening,
    };

    struct RoomSpatialSurface {
      std::string id;
      std::string sourceStaticMeshId;
      RoomSpatialSurfaceShape shape = RoomSpatialSurfaceShape::Plane;
      RoomSpatialSurfaceRole role = RoomSpatialSurfaceRole::Walkable;
      std::vector<Vec3> pointsMeters;
      Vec3 normal;
      std::vector<std::string> traversalTags;
      std::vector<std::string> collisionMask;
      bool blocksActor = false;
      bool blocksProjectile = false;
      std::string openingId;
    };
    ```

  - Keep this content-only. No runtime collision query ownership.
- `src/content/assets/RoomSpatialSurface.cpp`
  - New source if parser helpers are split out.
  - Implement string-to-enum helpers for shape/role and tag/mask validation.
  - Implement `bool isFiniteSurface(const RoomSpatialSurface&)` or equivalent
    content validation helper returning value status, not diagnostics.
  - Provide deterministic duplicate-tag handling: duplicate traversal or mask
    tokens are invalid.
- `src/content/assets/MeshAsset.cpp`
  - Add or confirm duplicate primitive id rejection if the packet hardens all
    room asset cross references in one pass.
  - Do not add collision roles here.
- `src/content/assets/MaterialAsset.cpp`
  - Add or confirm duplicate material id rejection if the packet hardens all
    asset cross references in one pass.
  - Keep material records visual-only.
- `src/content/PackageLoader.cpp`
  - Keep loading assets through existing `[[assets]]` refs.
  - Preserve public `PackageLoadStatus` unless the builder intentionally adds
    asset-specific statuses in `PackageLoader.hpp` and tests every mapping.
  - Ensure non-Ok asset parse results keep payload non-authoritative.
- `tests/unit/room_asset_loader_tests.cpp`
  - Expand existing test or add named tests for spatial surfaces and hardening
    failures.
- `tests/unit/room_spatial_surface_tests.cpp`
  - Add this test if spatial parsing/validation is split out and deserves a
    focused unit boundary.
- `CMakeLists.txt` / `cmake/iggy3d_tests.cmake`
  - Register new `.cpp` sources/tests only if new files are added.
  - Do not register fake source files for header-only helpers.

Likely files not to modify:

- `src/runtime/movement/*`
- `src/runtime/targeting/*`
- `src/runtime/projectile/*`
- `src/runtime/save/*`
- `src/render/vulkan/*`

The only acceptable Vulkan change is a receipt/readout field if a later review
decides content spatial counts should be echoed visually. This packet should not
need that.

## 7. Tests And Acceptance Gates

Minimum unit tests:

- `valid_first_room_spatial_surfaces_parse`
  - Load `fixtures/demos/first_room/package.iggy3d.toml`.
  - Assert `PackageLoadStatus::Ok`.
  - Assert one room with `spatialSurfaces.size() >= 4`.
  - Assert counts:
    - `walkable_surface_count >= 1`;
    - `blocker_surface_count >= 1`;
    - `projectile_blocker_surface_count >= 1`;
    - `opening_surface_count >= 1`.
- `duplicate_surface_id_rejected`
  - Parse room text with two `[[spatial_surfaces]]` rows sharing the same `id`.
  - Expect `RoomAssetParseResult::ok == false`.
  - Expected reason: `room_duplicate_spatial_surface_id`.
- `zero_or_invalid_normal_rejected`
  - Use `normal = [0.0, 0.0, 0.0]`.
  - Expected reason: `room_invalid_spatial_surface_normal`.
- `non_finite_surface_point_rejected`
  - Use a malformed or non-finite `points_ft` number such as `nan` or `inf`.
  - Expected reason: `room_invalid_spatial_surface_point`.
- `unknown_traversal_tag_rejected`
  - Use `traversal_tags = ["walkable", "magic"]`.
  - Expected reason: `room_unknown_traversal_tag`.
- `opening_surface_is_not_blocker`
  - Valid opening surface has `role = "opening"`, `blocks_actor=false`, and
    `blocks_projectile=false`.
  - Invalid opening with either blocker boolean true is rejected with
    `room_invalid_opening_surface`.
- `projectile_blocker_distinct_from_actor_blocker`
  - Valid projectile blocker has `blocks_projectile=true` and can set
    `blocks_actor=false`.
  - Assert role/count distinguishes it from `role = "blocker"`.
- `surface_static_mesh_reference_required`
  - Unknown `source_static_mesh` rejects with
    `room_unknown_spatial_surface_mesh`.
- `surface_opening_reference_required_for_opening_role`
  - Unknown or empty `opening_id` on `role = "opening"` rejects with
    `room_unknown_spatial_surface_opening` or `room_invalid_opening_surface`.

Acceptance/status fields for test output or future receipts:

```text
spatial_surface_count=<n>
walkable_surface_count=<n>
blocker_surface_count=<n>
projectile_blocker_surface_count=<n>
opening_surface_count=<n>
surface_normal_valid=true
surface_height_sampled=true
```

`surface_height_sampled=true` is optional in this packet and only allowed if it
is a cheap content-level sample over parsed surface points. It must not imply a
runtime collision query exists.

Green proof:

- `ctest --test-dir build -R "room_asset|package" --output-on-failure` passes;
- full `ctest --test-dir build --output-on-failure` remains green;
- `package_visual_room_asset_smoke` still reports the Tier B mesh draw receipt;
- no runtime movement/clamping behavior changes.

## 8. Movement Dot-Policy Bridge

This packet prepares movement policy without implementing it.

Future movement classification:

- world up is `(0, 1, 0)`;
- `dot(surfaceNormal, worldUp)` classifies orientation;
- flat/upward floor has dot near `1.0`;
- steep walkable candidates have dot between `cos(maxWalkableSlopeDegrees)` and
  `1.0`;
- walls have dot near `0.0`;
- ceilings have negative dot.

Ownership:

- `maxWalkableSlopeDegrees`, acceleration, friction, step height, ground snap,
  movement radius, and movement height belong to runtime movement params;
- content provides surfaces/tags/masks only;
- ability systems may later ignore slope, acceleration, or friction through
  explicit ability policy;
- ability systems must not ignore wall collision unless the ability is
  explicitly phasing, teleporting, or otherwise documented as bypassing blockers.

This packet must not build a 1000-ability rules layer. It should expose clean
surface roles and tags so future ability policy has one place to read authored
spatial facts.

## 9. No-Go Surfaces

- no JSON;
- no Blender runtime;
- no glTF importer;
- no physics engine;
- no movement clamping in this packet unless explicitly re-scoped;
- no projectile simulation;
- no tactical camera;
- no Vulkan gameplay decisions;
- no render receipt as save/replay truth;
- no old `/Users/kogaryu/iggy` code import;
- no runtime-owned static meshes;
- no save/load schema changes for spatial surfaces.

## 10. Recommended Next Builder Order

Builder Dex should receive this packet next.

Builder-ready: yes.

Recommended target:

```text
Room Asset Packet 2: Loader Hardening And Spatial Surface Contract
```

No hard blocker was found that should come first. The only sequencing caution is
that Builder Dex must preserve the current Tier B package-room mesh receipt while
adding content spatial surface semantics. If the builder discovers the existing
room parser cannot be extended cleanly without making diagnostics ambiguous, the
fallback is to add `RoomSpatialSurface.hpp/.cpp` and keep the existing
`RoomAsset.cpp` as the table dispatcher.
