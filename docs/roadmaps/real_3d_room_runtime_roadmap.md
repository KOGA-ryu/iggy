# Real 3D Room Runtime Roadmap

Status: planning roadmap after Room Asset Packet 1 Tier B completion
Repo: `/Users/kogaryu/iggy3d`
Branch context: `iggy3d-main`
Current head accounted for: `e770a83 Add package room mesh rendering proof`

This roadmap starts from the current Packet 1 tree. It is not an implementation
packet. It maps the dependency ladder from the first package-driven static room
asset path to a durable 3D runtime with room assets, authored objects,
first-person play, and later tactical view.

## Current Reality

### Package And Scenario

The first-room package currently uses TOML only:

- `fixtures/demos/first_room/package.iggy3d.toml` declares package
  `iggy3d.first_room`, schema/runtime schema `1`, scenario
  `scenario.iggy3d.toml`, and three asset refs:
  `room.spawn_corridor`, `mesh.room_primitives`, and `material.room_basic`.
- `fixtures/demos/first_room/scenario.iggy3d.toml` still owns gameplay seed
  truth: local player slot `0`, `player`, `gold_key`, `tactical_marker_alpha`,
  `training_dummy`, and objective `collect_gold_key`.
- `PackageManifest` owns `PackageAssetRef { id, path }` and
  `PackageManifest::assets`; do not create a parallel manifest.
- `PackageLoader.cpp` currently parses `[package]` and `[[assets]]` refs, maps
  scenario parser statuses into `PackageLoadStatus`, and validates paths as
  relative package-local paths. Asset file parsing is a separate layer.

### Room Asset Packet 1 State

Packet 1 has already introduced current reality that later packets must build
on:

- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
  contains the one-room/corridor subset from
  `/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml`: spawn
  room, east opening, corridor stub, packet-local crate prop, and runtime anchor
  names for `gold_key`, `training_dummy`, and `tactical_marker_alpha`.
- `fixtures/demos/first_room/assets/meshes/room_primitives.meshes.iggy3d.toml`
  defines primitive box mesh records.
- `fixtures/demos/first_room/assets/materials/room_basic.materials.iggy3d.toml`
  defines vertex-color material records.
- `src/content/assets/RoomAsset.hpp`, `MeshAsset.hpp`, and `MaterialAsset.hpp`
  define packet-local parsed asset shapes with simple parse result structs.
- `RoomAsset.cpp`, `MeshAsset.cpp`, and `MaterialAsset.cpp` parse a small TOML
  subset with no JSON and no external serialization dependency.
- Tier B true static room mesh draw proof is green at `e770a83`. The verified
  package visual receipt reports `rendering_path=package_room_meshes`,
  `record_mode=room_mesh_draws`, `room_static_mesh_count=11`,
  `mesh_draw_count=11`, `indexed_draw_count=11`,
  `vertex_buffer_uploaded=true`, `index_buffer_uploaded=true`,
  `depth_enabled=true`, `camera_projection=perspective`,
  `projection_application=single`, `result=pass`, and
  `reason_code=package_room_meshes_presented`.

### Runtime State

Runtime gameplay state is still authoritative and independent from static room
assets:

- `EntityState` owns stable runtime entity identity, transform, bounds, active
  state, target actions, and canonical interaction metadata.
- Movement currently validates actor existence, active state, finite destination,
  and max distance, then mutates via `WorldState::updateTransform`.
- Reach currently measures actor transform position to target entity transform
  position or explicit target point. Bounds and room walls are not reach truth.
- `gold_key`, `training_dummy`, and `tactical_marker_alpha` are scenario
  entities, not room mesh records.

### Projection And Render

Projection has begun to separate runtime facts from render facts:

- `SceneItem` projects runtime entities into backend-neutral entity render facts.
- `SceneRoomProjection` currently carries package-room facts such as asset id,
  source subset, mesh/material/anchor counts, visibility booleans, and room mesh
  item summaries.
- `FrameInput` carries viewport, source tick/frame clock, camera matrices,
  scene/debug projections, and render camera mode.
- Vulkan has a depth-tested hardcoded first-room path, a proxy primitive
  fallback path, and a package-room mesh path. Current `RenderLoop` chooses
  package-room rendering when `scene.room.loaded`, creates room mesh resources,
  and records indexed room draws through `recordFirstRoomFrame`.
- `recordFirstRoomFrame` uses depth, viewport/scissor, 0..1 Vulkan depth, and a
  package-room branch using `FrameInput.camera.clipFromWorld` for the committed
  room mesh path. The hardcoded bootstrap path remains separate fallback/proof
  code.

### Current Gap

The biggest post-Packet-1 gap is that room asset data exists and is projected,
but runtime/gameplay and rendering are not yet using one coherent asset-backed
world path:

- room mesh records are not collision authority;
- movement is not clamped by walls/openings;
- reach ignores room surfaces and uses entity transform points only;
- entity visuals are not bound to mesh/prototype assets;
- package-room rendering is now a real indexed room mesh path, but content
  surfaces/tags for movement, slope, and projectiles are not authored yet;
- first-person camera matrices and package room meshes exist, but gameplay
  systems do not yet consume authored spatial surfaces.

## Dependency Graph

Static room asset path:

```text
package.iggy3d.toml [[assets]]
-> RoomAsset / MeshAsset / MaterialAsset parsed TOML
-> validated package asset registry
-> SceneRoomProjection
-> RenderDrawItem / static room draw list
-> Vulkan room buffers and material bindings
-> draw receipt fields
```

Runtime entity path:

```text
scenario entity rows
-> FixtureScenarioSeed
-> WorldState / PlayerRoster / inventory / objectives
-> SceneProjection SceneItem
-> entity render binding
-> runtime command result / summary / replay proof
```

Movement and reach path:

```text
CommandRecord
-> CommandAdmission
-> MovementSystem / ReachQuery
-> WorldState mutation or rejection
-> SceneProjection
-> render receipt / runtime summary
```

Authoring path:

```text
Blender or TOML authoring source
-> exporter output: iggy3d TOML plus mesh assets
-> package asset refs
-> content asset loader
-> projection/render/runtime gates
```

## Packet Ladder After Packet 1

Current sequencing rule:

- True Static Room Mesh Draw Proof is complete at `e770a83`.
- The next builder packet is loader hardening plus authored spatial surface
  contract.
- Do not build movement clamping, slopes, projectile simulation, or tactical
  preview until the spatial surface contract is green.

### Packet 1 Complete: True Static Room Mesh Draw Proof

Purpose: completed proof that the first package room asset produces actual
static room vertex and index draws.

Likely files:

- `src/content/assets/RoomAsset.hpp`
- `src/content/assets/MeshAsset.hpp`
- `src/projection/scene/SceneItem.hpp`
- `src/projection/scene/SceneProjection.cpp`
- `src/render/FrameInput.hpp`
- `src/render/vulkan/RenderLoop.cpp`
- `src/render/vulkan/CommandRecording.cpp`
- `tests/smoke/package_visual_room_asset_smoke.cpp`

Data ownership rules:

- content owns immutable room/mesh/material asset records;
- projection owns backend-neutral room draw facts;
- Vulkan owns GPU buffers and draw commands only;
- runtime `WorldState` does not own static room meshes;
- render receipts prove the draw path but are not save/replay truth.

Compute/runtime costs:

- CPU mesh expansion is O(room static mesh record count);
- GPU upload cost is O(vertex count + index count) at load or render-resource
  creation time;
- frame draw cost scales with room draw item count and indexed draw count;
- no per-frame TOML parsing.

Completion evidence:

- commit `e770a83 Add package room mesh rendering proof`;
- full `ctest --test-dir build --output-on-failure` passed `40/40` before
  commit;
- package visual receipt reports `package_room_meshes`, `room_mesh_draws`,
  positive mesh/indexed draw counts, uploaded buffers, depth, perspective, and
  single projection.

No-go surfaces:

- no proxy receipt labeled as true mesh draw;
- no `recordProxyPrimitiveFrame` for the package-room mesh proof path;
- no screen-space room meshes;
- no runtime-owned static meshes;
- no collision or gameplay slope semantics in this packet.

Acceptance commands or receipt fields:

- `rendering_path=package_room_meshes`;
- `record_mode=room_mesh_draws`;
- `room_asset_loaded=true`;
- `room_static_mesh_count>0`;
- `room_material_count>0`;
- `room_anchor_count>0`;
- `mesh_draw_count>0`;
- `indexed_draw_count>0`;
- `vertex_buffer_uploaded=true`;
- `index_buffer_uploaded=true`;
- `depth_enabled=true`;
- `camera_projection=perspective`;
- `projection_application=single`;
- `floor_visible=true`;
- `wall_visible=true`;
- `opening_visible=true`;
- `prop_visible=true`;
- `key_marker_visible=true`;
- `dummy_marker_visible=true`;
- `first_room_visible=true`.

Green proof:

- package-room rendering binds vertex/index buffers and issues indexed room
  geometry draw calls;
- receipt fields match the true mesh path and do not overclaim proxy rendering;
- visual smoke shows floor, walls, one opening, one prop, key marker, and dummy
  marker in perspective with depth enabled.

Deferred:

- collision and movement clamping;
- slope/elevation gameplay;
- projectile simulation;
- tactical overhead preview;
- loader hardening beyond fields needed to prove this path.

## Spatial Gameplay Foundation

This lane starts only after True Static Room Mesh Draw Proof is green. Spatial
gameplay must be built on authored world-space room geometry, not on proxy
rectangles, screen-space overlays, or tactical abstractions.

Packet 2 implements the first concrete piece of this lane by combining loader
hardening with Spatial Packet A surface authoring. Later packets consume those
content-owned surfaces through runtime query systems.

### Sequencing Rule

- Do not build slopes, projectiles, or tactical preview until the true static
  room mesh draw path proves world-space room geometry with depth.
- Do not use tactical mode to hide missing first-person spatial simulation.
- Do not build projectile combat as hitscan or lock-on and retrofit spatial
  motion later.
- The first spatial packets may use simple authored primitives, but they must
  be world-space and deterministic.

### Spatial Concepts And Semantics

- `elevation`: world `y` is actual height. It is not decorative vertical offset,
  screen layering, or a render-only lift.
- `walkable surface`: authored or derived surface that movement can stand on
  when finite, within slope limits, and not blocked by traversal tags.
- `blocker surface`: authored collision surface that prevents actor capsule
  movement through it.
- `projectile blocker surface`: authored collision surface that can stop or
  impact a projectile. It may differ from actor blockers, but the difference is
  explicit in content tags.
- `surface normal`: normalized finite world-space normal used for slope and
  impact decisions.
- `slope angle`: angle in degrees between the surface normal and world up
  `(0, 1, 0)`. Flat ground is `0.0`; vertical walls approach `90.0`.
- `max walkable slope`: runtime movement parameter in degrees. A walkable
  surface is accepted only when `slope_angle_degrees <= maxWalkableSlopeDegrees`.
- `step height`: runtime movement parameter in meters. A vertical height delta
  at or below this value can be stepped if the destination surface is otherwise
  walkable.
- `ground snap distance`: runtime movement parameter in meters. Movement can
  snap downward to a walkable surface within this distance after horizontal
  motion.
- `movement radius/height/capsule`: actor movement volume represented as a
  vertical capsule with finite `radiusMeters` and `heightMeters`; collision
  sweeps use the capsule, not a point.
- `traversal tags`: content-authored tags such as `walkable`, `blocker`,
  `projectile_blocker`, `no_player`, `low_cover`, or `debug_only`. Runtime
  systems read tags but do not author them.
- `projectile state`: runtime-owned deterministic state containing
  `projectileId`, `owner`, `position`, `velocity`, `gravity`, `radius`,
  `lifetimeTicksRemaining`, `collisionMask`, and latest `impactEvent` when one
  occurs.
- `impact event`: runtime event emitted when projectile collision resolves to
  `world`, `entity`, `expired`, or `none`. Impact events are runtime/replay
  truth; render markers are debug projections of those events.

### Spatial Data Ownership

- Content owns authored spatial surfaces, collision surfaces, slope tags,
  traversal tags, projectile blockers, and package-local asset references.
- Runtime owns movement parameters, actor capsule state, projectile simulation,
  projectile impact events, deterministic tick results, and command outcomes.
- Projection owns debug/readout facts such as movement path samples, slope
  decisions, projectile arcs, collision sweep summaries, and impact markers.
- Vulkan draws room meshes, entity meshes, debug lines, arcs, and markers. It
  never decides hits, movement legality, slope acceptance, or projectile
  collision.
- Save/replay truth comes from runtime state and runtime events. Render receipts
  and visual debug fields are proof output only.

### Spatial Acceptance Receipt Fields

Later spatial packets should use exact diagnostic fields like these:

```text
surface_height_sampled=true
surface_normal_valid=true
slope_angle_degrees=<value>
slope_accepted=true
slope_rejected=true
step_accepted=true
movement_clamped=true
collision_sweep_count=<n>
projectile_spawned=true
projectile_tick_count=<n>
projectile_used_hitscan=false
projectile_lock_on=false
projectile_impact_kind=world|entity|expired|none
projectile_arc_visible=true
```

These fields are receipts/diagnostics only. They do not become save envelope
fields and they are not replay hash inputs unless a later durability packet
explicitly promotes a runtime event or state field.

### Spatial Packet A: Spatial Surface Authoring Packet

Purpose: add authored spatial surfaces and traversal tags to the first room
asset without changing runtime movement yet.

Likely files:

- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
- `src/content/assets/RoomAsset.hpp`
- `src/content/assets/RoomAsset.cpp`
- `src/content/assets/RoomSpatialSurface.hpp`
- `src/content/assets/RoomSpatialSurface.cpp`
- `tests/unit/room_asset_loader_tests.cpp`

Dependencies:

- Packet 1 complete true static room mesh draw proof;
- room asset parser path and package asset refs.

Data ownership:

- content owns spatial surface ids, points, normals, traversal tags, and
  collision masks;
- runtime receives immutable validated surface data through session/package
  initialization;
- renderer may draw debug normals but does not own spatial facts.

Compute/runtime costs:

- parse and validate O(surface count + vertex count + tag count);
- no runtime tick cost until collision queries consume the surfaces.

No-go surfaces:

- no physics engine;
- no procedural navmesh;
- no tactical preview;
- no Blender dependency in runtime;
- no JSON.

Green proof:

- first room asset includes at least one walkable floor surface, one blocker wall
  surface, one projectile blocker surface, and one opening that is not a
  blocker;
- loader diagnostics reject non-finite points, zero normals, duplicate surface
  ids, and unknown traversal tags;
- receipt reports `surface_height_sampled=true` and
  `surface_normal_valid=true` for a known floor sample.

Deferred:

- movement clamping;
- projectile collision;
- multi-room surface stitching.

### Spatial Packet B: Collision Query Core Packet

Purpose: provide deterministic runtime queries over authored spatial surfaces.

Likely files:

- `src/runtime/collision/SpatialSurfaceSet.hpp`
- `src/runtime/collision/SpatialSurfaceSet.cpp`
- `src/runtime/collision/CollisionQuery.hpp`
- `src/runtime/collision/CollisionQuery.cpp`
- `src/runtime/collision/CollisionTypes.hpp`
- `tests/unit/collision_query_tests.cpp`

Dependencies:

- Spatial Surface Authoring Packet;
- core math `Vec3`, `Ray3`, `Plane`, and `Aabb3` helpers;
- no Vulkan dependency.

Data ownership:

- runtime collision owns derived query views over immutable content surfaces;
- query results are deterministic value results, not direct `WorldState`
  mutations;
- projection can consume query summaries for debug display.

Compute/runtime costs:

- first build uses O(surface count) scans per query;
- sweep queries use finite deterministic iterations over surfaces;
- no broadphase until multi-room fixtures prove need.

No-go surfaces:

- no renderer ray-pick as gameplay authority;
- no nondeterministic floating-point reductions;
- no hidden closest-bounds reach policy.

Green proof:

- height sample at player spawn returns finite world `y`;
- normal sample on flat floor returns world-up within epsilon;
- wall sweep returns blocker hit;
- opening sweep returns no blocker;
- receipt/test fields include `collision_sweep_count=<n>`.

Deferred:

- acceleration structures;
- sliding response;
- dynamic obstacles.

### Spatial Packet C: Kinematic Movement Params Packet

Purpose: move the player capsule through collision queries with explicit slope,
step, and snap parameters.

Likely files:

- `src/runtime/movement/MovementParams.hpp`
- `src/runtime/movement/MovementSystem.hpp`
- `src/runtime/movement/MovementSystem.cpp`
- `src/runtime/collision/CollisionQuery.hpp`
- `src/runtime/world/WorldState.cpp`
- `tests/unit/movement_system_tests.cpp`

Dependencies:

- Collision Query Core Packet;
- existing `WorldState::updateTransform` mutation contract;
- current command admission movement distance policy.

Data ownership:

- runtime owns movement params and actor capsule dimensions;
- content owns traversal tags and collision surfaces;
- `WorldState::updateTransform` remains the only actor transform mutation API.

Compute/runtime costs:

- each movement command performs O(surface count) height and sweep queries;
- no continuous physics tick; movement remains command-driven and deterministic.

No-go surfaces:

- no rigid-body simulation;
- no renderer collision decisions;
- no direct mutable entity pointer path;
- no tactical-only movement rules.

Green proof:

- movement onto accepted slope succeeds and reports `slope_accepted=true`;
- movement onto rejected slope fails and reports `slope_rejected=true`;
- valid small vertical transition reports `step_accepted=true`;
- blocked move reports `movement_clamped=true`;
- no movement command mutates world state on failure.

Deferred:

- sliding;
- jumping;
- crouching;
- moving platforms.

### Spatial Packet D: Elevation/Slope Demo Room Packet

Purpose: add a small authored test room section that proves actual elevation and
slope behavior in first-person play.

Likely files:

- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
- `fixtures/demos/first_room/assets/meshes/room_primitives.meshes.iggy3d.toml`
- `fixtures/demos/first_room/scenario.iggy3d.toml`
- `tests/smoke/package_visual_room_asset_smoke.cpp`
- `tests/unit/movement_system_tests.cpp`

Dependencies:

- Kinematic Movement Params Packet;
- true room mesh draw proof with depth and perspective;
- entity visual binding for player/key/dummy markers if present.

Data ownership:

- content owns the ramp/step/blocker geometry and traversal tags;
- runtime owns whether a movement command succeeds or fails;
- projection owns debug slope and path readouts.

Compute/runtime costs:

- same movement query costs as Kinematic Movement Params Packet;
- render cost increases by the added room mesh count.

No-go surfaces:

- no decorative-only elevation;
- no camera-only fake height;
- no tactical preview substitution for first-person proof.

Green proof:

- player can move up an accepted ramp using world `y` height;
- player cannot move up an over-limit slope;
- receipt/test reports finite `slope_angle_degrees=<value>`;
- visual smoke shows the ramp/step in perspective without projection warping.

Deferred:

- full stair systems;
- navmesh generation;
- multi-floor maps.

### Spatial Packet E: Projectile Runtime Packet

Purpose: add deterministic projectile state and tick simulation without
collision-driven impacts yet.

Likely files:

- `src/runtime/projectile/ProjectileState.hpp`
- `src/runtime/projectile/ProjectileSystem.hpp`
- `src/runtime/projectile/ProjectileSystem.cpp`
- `src/runtime/session/SessionState.hpp`
- `src/runtime/session/SessionTick.cpp`
- `tests/unit/projectile_system_tests.cpp`

Dependencies:

- deterministic clock/tick policy;
- runtime command/session ownership;
- no tactical preview requirement.

Data ownership:

- runtime owns projectile ids, owner, position, velocity, gravity, radius,
  lifetime, collision mask, and impact event state;
- projection receives projectile arcs and markers as derived debug facts;
- save/replay include projectile runtime state only when the durability packet
  explicitly adds those fields.

Compute/runtime costs:

- each tick advances O(active projectile count);
- first build uses fixed-tick Euler integration with deterministic finite
  validation;
- no per-frame visual simulation outside runtime tick results.

No-go surfaces:

- no hitscan default;
- no lock-on default;
- no renderer-owned projectile state;
- no networking coupling.

Green proof:

- spawn command creates a projectile with finite position and velocity;
- projectile advances for exact tick count and decrements lifetime;
- receipt/test reports `projectile_spawned=true`,
  `projectile_tick_count=<n>`, `projectile_used_hitscan=false`, and
  `projectile_lock_on=false`.

Deferred:

- collision impacts;
- damage;
- prediction/rollback.

### Spatial Packet F: Projectile Collision And Arc Demo Packet

Purpose: collide deterministic projectiles with world/entity blockers and show
their arc and impact marker.

Likely files:

- `src/runtime/projectile/ProjectileSystem.cpp`
- `src/runtime/collision/CollisionQuery.cpp`
- `src/runtime/world/WorldState.hpp`
- `src/projection/scene/SceneProjection.cpp`
- `src/render/vulkan/CommandRecording.cpp`
- `tests/unit/projectile_system_tests.cpp`
- `tests/smoke/projectile_arc_visual_smoke.cpp`

Dependencies:

- Projectile Runtime Packet;
- Collision Query Core Packet;
- Spatial Surface Authoring Packet projectile blocker tags;
- entity/world target ownership.

Data ownership:

- runtime owns impact event kind and target reference;
- content owns projectile blocker surfaces;
- projection owns arc samples and impact marker debug facts;
- Vulkan draws arc/debug markers only.

Compute/runtime costs:

- O(active projectile count * surface count) collision in first build;
- entity collision may scan O(targetable entity count);
- arc projection cost is O(debug sample count).

No-go surfaces:

- no hitscan fallback;
- no lock-on correction;
- no render-driven hit testing;
- no damage system unless a separate packet owns it.

Green proof:

- projectile hits a world blocker and emits
  `projectile_impact_kind=world`;
- projectile can expire with `projectile_impact_kind=expired`;
- arc receipt reports `projectile_arc_visible=true`;
- entity hit, if included, uses runtime entity collision and not screen-space
  picking.

Deferred:

- damage and health;
- area effects;
- multiplayer prediction.

### Spatial Packet G: Tactical Preview Over Spatial Data Packet

Purpose: add tactical preview only after spatial movement and projectile facts
exist as runtime/projection data.

Likely files:

- `src/runtime/camera/CameraModePolicy.cpp`
- `src/runtime/clock/Clock.cpp`
- `src/projection/scene/SceneProjection.cpp`
- `src/render/vulkan/CommandRecording.cpp`
- `tests/unit/camera_mode_policy_tests.cpp`
- `tests/smoke/package_visual_tactical_view_smoke.cpp`

Dependencies:

- Elevation/Slope Demo Room Packet;
- Projectile Collision And Arc Demo Packet if projectile arcs are previewed;
- current tactical clock/camera policy.

Data ownership:

- runtime owns tactical command timing and accepted movement/projectile facts;
- projection owns preview paths, slope labels, arc samples, and impact markers;
- Vulkan draws tactical overlays but does not decide movement or hits.

Compute/runtime costs:

- camera/mode transition is O(1);
- overlay generation is O(path sample count + projectile debug sample count);
- no gameplay simulation inside render.

No-go surfaces:

- no tactical preview before spatial runtime is proven;
- no separate tactical-only collision model;
- no lock-on projectile targeting;
- no save/replay truth in overlay receipts.

Green proof:

- tactical view shows the same spatial surfaces and runtime projectile arc
  facts as first-person mode;
- slow/pause/step behavior remains deterministic;
- overlay receipts include slope/path/arc fields without changing runtime state.

Deferred:

- tactical UI selection;
- fog of war;
- multiplayer-safe preview sharing.

### Spatial No-Go Surfaces

- no hitscan default for projectiles;
- no lock-on default;
- no tactical preview before spatial runtime is proven;
- no runtime-owned static meshes;
- no Vulkan collision decisions;
- no JSON;
- no Blender runtime dependency;
- no multiplayer coupling until deterministic local spatial simulation is
  proven.

### Packet 2: Room Asset Loader Hardening And Spatial Surface Contract

Purpose: turn the Packet 1 parser path into a builder-safe content contract and
add authored spatial surface semantics for later collision, movement, slope, and
projectile packets.

Likely files:

- `src/content/assets/RoomAsset.hpp`
- `src/content/assets/RoomAsset.cpp`
- `src/content/assets/RoomSpatialSurface.hpp`
- `src/content/assets/RoomSpatialSurface.cpp`
- `src/content/assets/MeshAsset.hpp`
- `src/content/assets/MeshAsset.cpp`
- `src/content/assets/MaterialAsset.hpp`
- `src/content/assets/MaterialAsset.cpp`
- `src/content/PackageLoader.hpp`
- `src/content/PackageLoader.cpp`
- `src/content/PackageValidator.hpp`
- `src/content/PackageValidator.cpp`
- `tests/unit/room_asset_loader_tests.cpp`
- `tests/unit/room_spatial_surface_tests.cpp`
- `tests/smoke/package_room_asset_smoke.cpp`

Data ownership rules:

- package manifest owns asset refs only;
- asset loader owns parsed immutable asset records;
- content owns authored spatial surfaces, traversal tags, collision masks,
  projectile blocker roles, and opening/non-blocker surfaces;
- package validator owns cross-asset references and duplicate id checks;
- runtime `WorldState` does not own room mesh data;
- renderer does not parse TOML.

Compute/runtime costs:

- O(package text + asset text bytes) parsing;
- O(asset count squared) duplicate id checks are acceptable for first fixtures;
- O(spatial surface count squared) duplicate surface id checks keep diagnostics
  deterministic;
- O(surface count * referenced static mesh/opening count) cross-reference
  validation is acceptable for the first room;
- no runtime tick cost except using the loaded immutable registry.

Dependencies:

- Packet 1 asset TOML files and parsers;
- completed Tier B package-room mesh draw proof at `e770a83`;
- current package loader asset ref mechanism;
- no Vulkan dependency in content headers.

No-go surfaces:

- no JSON;
- no glTF importer;
- no old `/Users/kogaryu/iggy` runtime dependency;
- no renderer-owned parse fallback;
- no package-local absolute paths;
- no movement clamping, projectile simulation, or tactical camera in this
  packet.

Acceptance commands or receipt fields:

- `room_asset_loader_tests` asserts room id, source subset, mesh/material counts,
  opening id `out`, anchors, and cross-reference validity.
- `package_room_asset_smoke` asserts package asset ids and parse statuses.
- spatial tests assert `spatial_surface_count`, `walkable_surface_count`,
  `blocker_surface_count`, `projectile_blocker_surface_count`,
  `opening_surface_count`, and valid normals.

Green proof:

- invalid asset path, duplicate asset id, duplicate room mesh id, missing mesh,
  missing material, invalid finite values, and missing anchors return exact
  statuses/diagnostics;
- valid first-room package produces one immutable room asset registry;
- valid first-room spatial surfaces include walkable floor, blocker wall,
  projectile blocker, and opening/non-blocker records;
- duplicate surface id, zero normal, non-finite point, unknown traversal tag,
  and invalid opening/blocker combination are rejected deterministically.

Deferred:

- runtime collision query use;
- entity mesh binding;
- Blender export;
- multi-room graph generation.

### Packet 3: Entity Mesh And Prototype Binding

Purpose: bind runtime entities to visual asset/prototype records without making
room assets gameplay truth.

Likely files:

- `src/content/assets/EntityPrototypeAsset.hpp`
- `src/content/assets/EntityPrototypeAsset.cpp`
- `fixtures/demos/first_room/assets/entities/first_room_entities.iggy3d.toml`
- `src/projection/scene/SceneItem.hpp`
- `src/projection/scene/SceneProjection.cpp`
- `src/render/FrameInput.hpp`
- `src/render/vulkan/CommandRecording.cpp`
- `tests/unit/projection_tests.cpp`
- `tests/smoke/package_visual_room_asset_smoke.cpp`

Data ownership rules:

- `WorldState` owns entity identity, active state, transform, and command truth;
- entity prototype assets own visual mesh/material ids only;
- `SceneProjection` joins runtime entity stable names to optional visual refs;
- missing visual binding must not remove runtime entities.

Compute/runtime costs:

- O(entity count * prototype count) scan is acceptable for first build;
- later packets can replace with an id map after behavior is locked.

Dependencies:

- Packet 2 asset registry;
- existing `SceneItem::assetRef`;
- current `EntityState::stableName`.

No-go surfaces:

- no entity ownership in Vulkan;
- no runtime mutation from render binding;
- no save/replay dependency on visual prototype assets.

Acceptance commands or receipt fields:

- scene projection reports `entity_visual_bound.gold_key=true`;
- render receipt reports `entity_draw_count`, `pickup_draw_count`,
  `npc_draw_count`, and material ids or stable visual ids.

Green proof:

- key, dummy/NPC, marker, and player have deterministic visual bindings;
- disabling a binding degrades to debug marker without changing command
  admission, save, replay, or summary.

Deferred:

- animation;
- skeletal meshes;
- equipment visuals;
- damage decals.

### Packet 4: Room Collision Bounds And Movement Clamping

Purpose: make static room geometry constrain player movement while keeping
runtime state authoritative.

Likely files:

- `src/content/assets/RoomAsset.hpp`
- `src/runtime/collision/RoomCollision.hpp`
- `src/runtime/collision/RoomCollision.cpp`
- `src/runtime/movement/MovementSystem.hpp`
- `src/runtime/movement/MovementSystem.cpp`
- `src/runtime/session/SessionState.hpp`
- `tests/unit/movement_system_tests.cpp`
- `tests/unit/room_collision_tests.cpp`

Data ownership rules:

- room asset owns immutable wall/opening/floor collision records;
- session may hold a derived immutable `RoomCollision` view or reference to the
  loaded package asset registry;
- `WorldState::updateTransform` remains the only entity transform mutation API;
- collision results are rejection facts, not save truth.

Compute/runtime costs:

- first build may use O(wall/opening count) segment/box checks per movement;
- no broadphase required for one-room and small multi-room fixtures;
- all checks deterministic and finite.

Dependencies:

- Packet 2 validation;
- current movement distance validation;
- room opening semantics from Packet 1/2.

No-go surfaces:

- no physics engine;
- no continuous rigid-body simulation;
- no renderer collision reads;
- no wall state stored in `EntityState`.

Acceptance commands or receipt fields:

- movement test asserts walking through the east opening succeeds;
- movement into north/south/west walls returns exact blocked status;
- summary/receipt reports `movement.collision_gate=pass`.

Green proof:

- player cannot move outside spawn room except through opening/corridor stub;
- valid current acceptance moves remain green or are explicitly migrated to room
  anchor coordinates in one packet.

Deferred:

- dynamic doors;
- sliding collision;
- stair/height handling;
- multi-floor navigation.

### Packet 5: Reach And Target Integration With Room Anchors

Purpose: align interactable gameplay anchors with authored room positions and
targetable surfaces.

Likely files:

- `src/content/assets/RoomAsset.hpp`
- `src/runtime/targeting/ReachQuery.hpp`
- `src/runtime/targeting/ReachQuery.cpp`
- `src/runtime/targeting/TargetQuery.hpp`
- `src/runtime/targeting/TargetQuery.cpp`
- `src/runtime/command/CommandAdmission.cpp`
- `src/projection/scene/SceneProjection.cpp`
- `tests/unit/target_reach_tests.cpp`
- `tests/unit/command_admission_tests.cpp`

Data ownership rules:

- runtime entity transforms remain reach truth for entity interactions;
- room anchors may initialize or validate entity placement;
- room surfaces can produce target points only through explicit targeting APIs;
- bounds and render meshes do not silently change reach admission.

Compute/runtime costs:

- O(targetable entity count + room target surface count) query;
- no spatial index until multi-room tests prove need.

Dependencies:

- Packet 4 movement/collision boundary;
- Packet 3 entity visual binding;
- Packet 2 anchor validation.

No-go surfaces:

- no hidden closest-bounds reach policy;
- no renderer ray-pick as command authority;
- no surface interaction without command admission status.

Acceptance commands or receipt fields:

- out-of-range key interaction still rejects with `OutOfRange`;
- retry command reaches key using room-aligned entity transform;
- target receipt shows `target.source=runtime_entity` or
  `target.source=room_surface` explicitly.

Green proof:

- all first-room target facts are deterministic from runtime state plus validated
  room anchors;
- room surface targets cannot bypass command admission.

Deferred:

- precise mesh triangle picking;
- occlusion;
- line-of-sight combat.

### Packet 6: Blender Authoring And Export Path

Purpose: introduce Blender as an authoring tool, not as a runtime dependency.

Likely files:

- `tools/blender/export_iggy3d_room.py`
- `fixtures/authoring/first_room_blender/spawn_corridor.blend`
- `fixtures/authoring/first_room_blender/README.md`
- `fixtures/demos/first_room/assets/rooms/spawn_corridor.room.iggy3d.toml`
- `fixtures/demos/first_room/assets/meshes/*.iggy3d.toml`
- `tests/smoke/blender_export_fixture_smoke.cpp` or script smoke if available

Data ownership rules:

- `.blend` files are source-authoring artifacts only;
- exporter output is iggy3d TOML plus mesh asset files;
- engine consumes iggy3d assets, not Blender files;
- package manifest points only at runtime-consumable assets.

Compute/runtime costs:

- exporter cost can be proportional to authored mesh/object count;
- runtime cost remains package asset load and render upload only.

Dependencies:

- Packet 2 asset validation;
- Packet 3 visual bindings;
- Packet 4 collision metadata contract.

No-go surfaces:

- no Blender runtime dependency;
- no glTF importer in this packet;
- no JSON sidecar;
- no Python dependency in game runtime.

Acceptance commands or receipt fields:

- exporter receipt: source blend, output room asset, object count, mesh count,
  material count, anchor count;
- package smoke loads exported TOML without reading `.blend`.

Green proof:

- editing the `.blend` and rerunning exporter regenerates deterministic TOML;
- runtime tests pass with exported assets only.

Deferred:

- glTF;
- skeletal animation;
- Blender-driven gameplay scripts;
- procedural full-map generation.

### Packet 7: Perspective Camera And First-Person Usability Proof

Purpose: make first-person view usable with real room geometry instead of
screen-space or proxy room shapes.

Likely files:

- `src/render/FrameInput.hpp`
- `src/render/FrameInput.cpp`
- `src/projection/scene/SceneProjection.cpp`
- `src/render/vulkan/CommandRecording.cpp`
- `src/render/vulkan/RenderLoop.cpp`
- `src/runtime/camera/CameraState.hpp`
- `src/runtime/camera/CameraModePolicy.cpp`
- `tests/smoke/package_visual_room_asset_smoke.cpp`
- `tests/smoke/package_visual_playable_proxy_smoke.cpp`

Data ownership rules:

- runtime camera state owns camera mode, yaw, pitch, target;
- render frame owns derived view/projection matrices for one frame;
- Vulkan consumes matrices and buffers but does not own camera truth;
- receipts are proof output only and are not save/replay truth.

Compute/runtime costs:

- matrix build is O(1);
- draw cost scales with room draw item count;
- no per-frame TOML parsing.

Dependencies:

- Packet 1 complete true room mesh draw items;
- Packet 3 entity bindings;
- Packet 4 collision for playable first-person movement.

No-go surfaces:

- no screen-space room meshes;
- no double application of projection;
- no aspect-ratio hardcode;
- no negative-one-to-one depth assumptions in Vulkan clip space.

Acceptance commands or receipt fields:

- receipt fields: `camera.mode=FirstPerson`, `clip_depth=0_to_1`,
  `projection_application=single`, `drawable_aspect`, `room_draw_count`,
  `entity_draw_count`;
- visual smoke compares a nonblank frame and checks floor/wall/key/dummy pixels
  are in plausible perspective positions.

Green proof:

- room geometry remains stable across window aspect changes;
- moving player changes view without warping the room;
- first-person frame draws asset-backed room and entity visuals.

Deferred:

- head bob;
- mouse-look polish;
- animation interpolation;
- tactical camera.

### Packet 8: Tactical Overhead Orthographic View Switch

Purpose: add tactical view after the static room, first-person, and spatial
runtime paths are real. This packet must not be scheduled before the Spatial
Gameplay Foundation lane proves movement/collision data in runtime.

Likely files:

- `src/runtime/camera/CameraState.hpp`
- `src/runtime/camera/CameraModePolicy.cpp`
- `src/runtime/clock/Clock.cpp`
- `src/render/FrameInput.cpp`
- `src/render/vulkan/CommandRecording.cpp`
- `tests/unit/camera_mode_policy_tests.cpp`
- `tests/unit/clock_tests.cpp`
- `tests/smoke/package_visual_tactical_view_smoke.cpp`

Data ownership rules:

- `CameraModePolicy` owns mode transitions;
- `Clock` owns Normal/Slow/Paused timing;
- render owns orthographic matrix derivation;
- runtime commands remain the only way to enter/exit tactical mode.

Compute/runtime costs:

- O(1) mode/matrix transitions;
- same room/entity draw lists with different camera projection.

Dependencies:

- Packet 7 perspective path;
- existing clock slow/pause/resume/step policy;
- asset-backed room draw list;
- Spatial Packet C for movement/collision readouts at minimum;
- Spatial Packet G if tactical preview includes slope/path/projectile overlays.

No-go surfaces:

- do not build tactical mode before static room path is real;
- do not build tactical preview before spatial runtime is proven;
- no separate tactical-only room representation;
- no second projection pipeline with incompatible coordinates.

Acceptance commands or receipt fields:

- `ToggleTacticalMode` enters Slow and `TacticalOverhead`;
- `Pause`, `StepTacticalTick`, `Resume`, and exit return to `ThirdPerson` or
  configured realtime mode;
- receipt reports `camera.projection=orthographic` and `clock.mode=Slow`.

Green proof:

- tactical view draws the same room/entity data as first person;
- slow/pause/step behavior remains deterministic and replay-safe.

Deferred:

- tactical selection UI;
- fog of war;
- strategy overlays.

### Packet 9: Multi-Room Map Subset From Provingground TOML

Purpose: expand from spawn room plus corridor stub to a small authored map subset.

Likely files:

- `src/content/assets/RoomAsset.hpp`
- `src/content/assets/MapAsset.hpp`
- `src/content/assets/MapAsset.cpp`
- `fixtures/demos/first_room/assets/maps/provingground_subset.map.iggy3d.toml`
- `src/runtime/collision/RoomCollision.cpp`
- `src/projection/scene/SceneProjection.cpp`
- `tests/unit/map_asset_loader_tests.cpp`
- `tests/smoke/package_visual_multi_room_smoke.cpp`

Data ownership rules:

- map asset owns static room graph and openings;
- runtime still owns actors/entities/objectives;
- entity placement may reference map anchors but remains runtime state after
  session creation.

Compute/runtime costs:

- first multi-room build may scan O(room count + opening count) per validation;
- rendering batches static meshes by material where practical, but correctness
  comes first.

Dependencies:

- Packet 2 hardening;
- Packet 4 collision;
- Packet 7 camera.

No-go surfaces:

- do not build full provingground generator before one-room loader/render path is
  green;
- no pathfinding expansion;
- no procedural generator replacing authored subset.

Acceptance commands or receipt fields:

- receipt reports `map.room_count`, `map.opening_count`, `visible_room_count`;
- movement can cross one validated opening to a second room/corridor cell.

Green proof:

- deterministic placement from provingground subset;
- no entity id/order drift;
- save/replay hashes remain stable.

Deferred:

- full provingground import;
- dynamic streaming;
- navmesh.

### Packet 10: Durability, Replay, And Render Receipts For Asset-Backed Rooms

Purpose: prove asset-backed room rendering survives save/load/replay without
turning receipts or room meshes into runtime truth.

Likely files:

- `src/runtime/save/SaveEnvelope.hpp`
- `src/runtime/save/SaveLoad.cpp`
- `src/runtime/replay/StateHash.cpp`
- `src/runtime/replay/CommandReplay.cpp`
- `src/runtime/diagnostics/RuntimeSummary.cpp`
- `src/render/RenderReceipt.hpp`
- `tests/unit/save_load_tests.cpp`
- `tests/unit/replay_state_hash_tests.cpp`
- `tests/acceptance/complete_runtime_demo_tests.cpp`

Data ownership rules:

- save stores package/scenario identity, runtime state, command log, and hash;
- asset package identity is durable; room mesh buffers and render receipts are
  not durable truth;
- replay reconstructs assets from package refs plus baseline seed.

Compute/runtime costs:

- save/load remains O(runtime state + command log);
- asset load/reload cost is package-bound and can be cached outside save truth;
- render receipt generation is O(receipt fields).

Dependencies:

- Packet 7 or 8 render receipt fields;
- save/load cursor proof already in current docs;
- stable asset package identity from Packet 2.

No-go surfaces:

- do not save Vulkan handles;
- do not hash receipt fields;
- do not store generated runtime summary as truth;
- do not let missing render backend change gameplay replay.

Acceptance commands or receipt fields:

- save/load proof reports package asset ids and room asset id;
- replay proof reaches same state hash and same room asset receipt facts;
- render receipt has deterministic `asset_room_id`, mesh/material counts, draw
  count, and camera mode.

Green proof:

- save, load, reset, replay, and render receipts agree without adding gameplay
  commands;
- replay can run headless without Vulkan and still verify package asset identity.

Deferred:

- binary save compression;
- network replication of asset payloads;
- render capture as authoritative test oracle.

### Packet 11+: Multiplayer-Safe Asset Ownership

Purpose: keep room assets deterministic when multiple local/remote players and
authority boundaries arrive.

Likely files:

- `src/runtime/multiplayer/Authority.hpp`
- `src/runtime/multiplayer/ReplicationCodec.hpp`
- `src/runtime/session/Session.hpp`
- `src/runtime/replay/CommandLog.hpp`
- `src/content/assets/*`
- `tests/unit/multiplayer_authority_tests.cpp`

Data ownership rules:

- server/authority chooses package id, scenario id, and asset version identity;
- clients may cache render assets but cannot mutate runtime state through them;
- command log remains gameplay truth;
- asset mismatch is a compatibility/session admission failure, not a render
  fallback.

Compute/runtime costs:

- first build can compare package/asset ids in O(asset count);
- no asset streaming protocol until explicit packet.

Dependencies:

- Packet 10 durability/replay proof;
- exact package asset identity and hashes if introduced later.

No-go surfaces:

- no nondeterministic asset discovery;
- no renderer-driven authority;
- no receipt-as-network-truth.

Acceptance commands or receipt fields:

- multiplayer authority tests assert package/scenario/asset identity agreement;
- replication receipt names package id and asset set version.

Green proof:

- local multiplayer sessions share deterministic package asset identity and
  command ordering;
- a remote/session mismatch rejects before command execution.

Deferred:

- online transport;
- streaming asset bundles;
- rollback netcode.

## Perspective And Camera Rules

These rules prevent warping and pipeline drift:

- Vulkan clip depth is `0..1`. Projection helpers and tests must not assume
  OpenGL `-1..1` depth.
- Drawable aspect comes from the actual render viewport/swapchain extent. Do not
  hardcode 16:9 or use package room dimensions as screen aspect.
- Apply projection exactly once. Runtime positions are world coordinates;
  projection produces view/clip matrices; Vulkan shaders consume those matrices.
- No screen-space room meshes. Room floors, walls, openings, props, and entity
  visuals must be world-space draw items.
- First-person perspective and tactical orthographic are camera modes over the
  same scene data, not separate room data models.
- The hardcoded `firstRoomClipFromModel` path is a temporary proof path. Packets
  after Packet 1 should move toward `FrameInput.camera.clipFromWorld` plus
  per-item model transforms.

## Blender Boundary

Blender enters after loader/render/collision contracts are stable enough to
consume exported data:

- Blender is an authoring dependency only, never a runtime dependency.
- Source `.blend` files live under a fixture authoring folder such as
  `fixtures/authoring/first_room_blender/`.
- Exporter output is iggy3d TOML plus mesh assets referenced by package
  manifests.
- Runtime consumes iggy3d assets, not `.blend` files.
- Do not add glTF import in the early Blender packet. A later explicit packet
  may choose glTF if the native iggy3d mesh asset format cannot carry needed
  data.

## Risks And Sequencing Warnings

- Do not build tactical mode before the static room path is real. Tactical view
  should prove camera/clock policy over real room geometry, not hide proxy
  rendering.
- Do not build full provingground generation before one-room loader/render path
  is green.
- Do not let runtime own static room mesh state. Runtime can reference validated
  package asset identity and derived collision views; content owns assets.
- Do not let Vulkan leak into runtime/content/projection public headers.
- Do not introduce JSON.
- Do not let receipt fields become save/replay truth.
- Do not let Blender files become package runtime inputs.
- Do not let collision silently alter existing command semantics without exact
  rejection statuses and tests.
- Do not let entity visual binding remove or rename runtime entities.

## Recommended Build Order Update

Next packet: Packet 2, Room Asset Loader Hardening And Spatial Surface Contract.

Reason: Packet 1 Tier B is complete. The next risk is that room assets are
renderable but not yet authored or validated as spatial gameplay surfaces.
Movement, slopes, projectiles, Blender, tactical, durability, and multiplayer
packets need content-owned walkable/blocker/projectile-blocker/opening records
before runtime systems consume room geometry.

After Packet 2 is green:

- Run Collision Query Core if the authored spatial surfaces can be converted
  directly into deterministic runtime query views.
- Run Entity Mesh And Prototype Binding if visual object binding is the higher
  risk for the next user-visible milestone.
- Keep tactical preview deferred until spatial runtime facts are proven.

Immediate feedback to Builder Dex for Packet 2: do not rebuild the room mesh
draw path. Preserve the current `package_room_meshes` receipt while adding exact
spatial surface TOML, parser validation, and tests.
