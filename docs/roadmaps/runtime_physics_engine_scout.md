# Runtime Physics Engine Scout

Status: research/scout note
Repo: `/Users/kogaryu/iggy3d`
Branch context: `iggy3d-main`
Current build context: Builder Dex is working Room Asset Packet 2, loader
hardening plus spatial surface contract.

This document surveys physics-engine options for iggy3d and turns the research
into a conservative packet ladder. It is not an integration order. It should not
cause Builder Dex to change Packet 2.

## Sources Consulted

Official or primary sources used:

- Jolt Physics repository and documentation:
  - https://github.com/jrouwe/JoltPhysics
  - https://jrouwe.github.io/JoltPhysics/
  - https://raw.githubusercontent.com/jrouwe/JoltPhysics/master/LICENSE
- Bullet Physics repository, API docs, and license:
  - https://github.com/bulletphysics/bullet3
  - https://pybullet.org/Bullet/BulletFull/
  - https://pybullet.org/Bullet/BulletFull/classbtCollisionWorld.html
  - https://pybullet.org/Bullet/BulletFull/classbtKinematicCharacterController.html
  - https://raw.githubusercontent.com/bulletphysics/bullet3/master/LICENSE.txt
- NVIDIA PhysX repository and documentation:
  - https://github.com/NVIDIA-Omniverse/PhysX
  - https://nvidia-omniverse.github.io/PhysX/physx/5.3.1/docs/BuildingWithPhysX.html
  - https://nvidia-omniverse.github.io/PhysX/physx/5.4.1/docs/SceneQueries.html
  - https://nvidia-omniverse.github.io/PhysX/physx/5.3.1/docs/CharacterControllers.html
  - https://nvidia-omniverse.github.io/PhysX/physx/5.4.2/docs/Serialization.html
  - https://raw.githubusercontent.com/NVIDIA-Omniverse/PhysX/main/LICENSE.md
  - https://developer.nvidia.com/blog/announcing-physx-sdk-4-0-an-open-source-physics-engine/
- Rapier documentation and repository:
  - https://rapier.rs/docs/
  - https://github.com/dimforge/rapier
  - https://rapier.rs/docs/user_guides/javascript/scene_queries/
  - https://rapier.rs/docs/user_guides/javascript/scene_queries_shape_casting/
  - https://raw.githubusercontent.com/dimforge/rapier/master/LICENSE
- Box2D documentation and repository:
  - https://box2d.org/
  - https://box2d.org/documentation/
  - https://box2d.org/documentation/md_faq.html
  - https://github.com/erincatto/box2d
  - https://raw.githubusercontent.com/erincatto/box2d/main/LICENSE
- Architecture comparison references:
  - Godot Jolt docs: https://docs.godotengine.org/en/latest/tutorials/physics/using_jolt_physics.html
  - Unity raycast API reference: https://docs.unity3d.com/6000.4/Documentation/ScriptReference/Physics.Raycast.html

## What iggy3d Needs First

The first physics scope is not a full rigid-body world. The current room runtime
needs these in order:

1. Authored spatial surfaces from content:
   - walkable floor surfaces;
   - blocker surfaces for actor capsules;
   - projectile blocker surfaces;
   - opening/non-blocker surfaces;
   - traversal tags and collision masks.
2. Deterministic collision query core:
   - height sample;
   - surface normal sample;
   - raycast;
   - segment cast;
   - capsule or shape sweep;
   - overlap query.
3. Kinematic player movement:
   - max movement distance;
   - movement radius and height;
   - slope limit;
   - step height;
   - ground snap;
   - no-mutation-on-failure.
4. Projectile travel and collision:
   - projectile position, velocity, gravity, radius, owner, lifetime, collision
     mask, and impact event;
   - no hitscan default;
   - no lock-on default;
   - ray/sweep collision between fixed ticks.
5. Dynamic rigid bodies later:
   - crates, doors, debris, knockback, or physics props only after kinematic
     movement and projectile collision are deterministic.
6. Constraints, joints, ragdolls much later:
   - useful only after dynamic bodies exist and gameplay has a real need.
7. Deterministic replay and future multiplayer constraints:
   - physics inputs, query order, floating-point mode, event ordering, and state
     snapshots must be owned by runtime, not render or asset loaders.

## Engine Notes

### Jolt Physics

- License: MIT-style license in upstream `LICENSE`.
- Language/runtime fit: native C++, game-oriented, good iggy3d fit if a full
  3D physics engine becomes necessary.
- macOS/build: official repository lists macOS x64/ARM64 as supported and points
  to CMake/FetchContent examples.
- Character controller: Jolt documents `Character` and `CharacterVirtual`.
  `CharacterVirtual` is implemented through collision detection queries rather
  than being inserted as a normal body, which aligns with kinematic player
  control.
- Collision queries: docs expose broad/narrow phase query concepts, ray/cast
  result types, and collision filtering. Jolt has the query surface needed for
  ray/sweep/shape style gameplay.
- Rigid bodies: full rigid body simulation.
- Constraints/joints: fixed, distance, point, hinge, cone, slider, swing-twist,
  six-DOF, path, gear, rack-and-pinion, pulley, and vehicle constraints are
  documented.
- Determinism/replay: official docs say simulation is deterministic when APIs
  modifying simulation are called in the same order and the same binary code is
  used. Cross-platform determinism requires a CMake option and has caveats:
  same source/defines, broadphase query nondeterminism unless results are
  filtered/ordered, precise floating-point mode, no platform-dependent standard
  sort/hash/trig behavior.
- Serialization/loading: has state recording and rollback concepts; integration
  still needs iggy3d-owned save/replay state rather than blind engine snapshots.
- Abstraction fit: can be kept behind an iggy3d runtime physics abstraction if
  all Jolt body ids and query results are translated to iggy3d ids/events at the
  boundary.
- Near-term suitability: not yet. It solves more than Packet 2/3 need and would
  add dependency and determinism surface before the authored surface contract is
  proven.
- Future suitability: strongest future C++ candidate for dynamic rigid bodies
  because of license, C++ fit, macOS support, character controller options, query
  surface, and explicit determinism documentation.

### Bullet Physics

- License: zlib license in upstream `LICENSE.txt`.
- Language/runtime fit: native C++ and mature, but older API style and looser
  documentation for modern game integration.
- macOS/build: official repository says Bullet C++ is tested on Windows, Linux,
  Mac OSX, iOS, and Android, and needs only a C++ compiler for the core library.
- Character controller: `btKinematicCharacterController` exists and uses a ghost
  object plus convex sweep tests. The docs explicitly say dynamic rigid-body
  interaction must be implemented by the user.
- Collision queries: `btCollisionWorld` supports `rayTest` and
  `convexSweepTest`; callback choice controls first/all/any hit behavior.
- Rigid bodies: full rigid body and collision detection library.
- Constraints/joints: `btTypedConstraint` base covers constraint types such as
  hinge, slider, point-to-point, cone twist, gear, and generic six-DOF variants.
- Determinism/replay: no clear current official cross-platform determinism
  contract was found. Treat replay determinism as an integration burden.
- Serialization/loading: Bullet has serialization APIs in its class surface, but
  iggy3d should not make Bullet serialized blobs save truth.
- Abstraction fit: can be hidden behind an iggy3d abstraction, but Bullet object
  ownership and callback-heavy query APIs would need careful translation to
  deterministic ordered runtime results.
- Near-term suitability: not recommended. It is too broad for kinematic slopes
  and projectile collision, and its built-in character controller is not a clean
  policy match for iggy3d's authored movement semantics.
- Future suitability: acceptable fallback if Jolt fails a later integration
  spike, but not the first-choice future engine.

### NVIDIA PhysX

- License: current Omniverse PhysX repository uses BSD 3-Clause. NVIDIA's PhysX
  4 open-source announcement states BSD 3-licensed platforms include Apple
  MacOS, iOS, Android ARM, Linux, and Windows, with some console platforms under
  a separate NVIDIA EULA.
- Language/runtime fit: native C++ and powerful, but larger and heavier than
  iggy3d needs now.
- macOS/build: official docs describe a CMake-generation workflow with presets
  and generated build files. Expect heavier build integration than Jolt or a
  small custom query core.
- Character controller: PhysX CCT is an external component on top of PhysX. Docs
  describe it as kinematic, input-displacement based, and a base that games
  often customize.
- Collision queries: PhysX scene queries support raycasts, sweeps, and overlaps
  through `PxScene`; geometry queries cover raycasts, sweeps, overlaps, and
  penetration-style tests.
- Rigid bodies: full static, kinematic, and dynamic actor model.
- Constraints/joints: strong constraint/articulation support; PhysX 4+ docs call
  out TGS and reduced-coordinate articulations.
- Determinism/replay: no simple official cross-platform deterministic gameplay
  promise was found in the consulted docs. Treat deterministic replay as
  iggy3d-owned, not PhysX-owned.
- Serialization/loading: PhysX supports binary serialization and deprecated RepX
  XML. Binary serialization is platform/version-specific; therefore it should
  not be iggy3d save truth.
- Abstraction fit: can be hidden behind an abstraction, but the scene, cooking,
  serialization, and actor lifetime surface is broad.
- Near-term suitability: not appropriate. It is a large engine with build and
  scene-lifetime overhead before iggy3d has one deterministic kinematic movement
  core.
- Future suitability: good for industrial-grade dynamic bodies, vehicles, or
  complex constraints if a future packet demands them and accepts build
  complexity.

### Rapier

- License: Apache 2.0.
- Language/runtime fit: written in Rust, with official Rust crates and
  JavaScript bindings. This is not a natural fit for a C++ runtime without a
  deliberate FFI/binding strategy.
- macOS/build: Rust crates should be portable, but C++ iggy3d would pay an FFI
  and toolchain cost.
- Character controller: official docs describe a kinematic character controller
  that emits ray-casts and shape-casts to adjust a desired trajectory.
- Collision queries: official docs cover ray-casting and shape-casting; shape
  casts are explicitly described as sweep tests useful for character
  controllers. Rapier also advertises contact events, sensors, and snapshotting.
- Rigid bodies: full 2D/3D rigid bodies.
- Constraints/joints: official docs list joint constraints.
- Determinism/replay: official docs advertise optional cross-platform
  determinism and snapshotting.
- Serialization/loading: snapshotting is a useful concept, but iggy3d save truth
  would still need to remain runtime-owned.
- Abstraction fit: possible only behind a very hard FFI boundary. That boundary
  must translate all ids, results, and state snapshots to iggy3d-owned types.
- Near-term suitability: not recommended for C++ iggy3d because Rust/FFI would
  dominate the packet.
- Future suitability: technically attractive for determinism research, but only
  if iggy3d intentionally accepts Rust as a physics dependency.

### Box2D As 2D Contrast

- License: MIT.
- Language/runtime fit: current Box2D is C17 for the library; docs identify it
  as a 2D physics engine for games.
- macOS/build: official repository says Windows, Linux, and Mac are supported.
- Character/controller/query/rigid body support: excellent reference point for
  simple 2D collision/rigid body architecture, but not a 3D target.
- Determinism/replay implications: useful as conceptual contrast, not a direct
  iggy3d engine.
- Near-term suitability: not appropriate because iggy3d needs 3D world-y
  elevation, slopes, capsule sweeps, and 3D projectiles.
- Future suitability: none as a runtime dependency; useful only as a reference
  for simple APIs and tests.

### Custom Kinematic Collision Core

- License: iggy3d-owned.
- Language/runtime fit: native C++ in the existing runtime/content architecture.
- macOS/build: no new host dependency.
- Character controller support: exactly what iggy3d authors: capsule movement,
  slope policy, step height, ground snap, and no-mutation-on-failure.
- Collision queries: first build can implement deterministic plane/AABB/segment
  queries and swept capsule/shape casts over authored spatial surfaces.
- Rigid bodies: none at first.
- Constraints/joints: none at first.
- Determinism/replay: best near-term fit because query order, floating-point
  policy, and event ordering are fully owned by iggy3d.
- Serialization/loading: save runtime movement/projectile state and impact
  events only; content spatial surfaces reload from package assets.
- Abstraction fit: becomes the first implementation of the iggy3d physics query
  abstraction; later Jolt/PhysX/Bullet can replace or augment it behind the same
  boundary.
- Near-term suitability: best choice.
- Future suitability: keep for deterministic gameplay queries even if a third
  party handles optional dynamic rigid bodies later.

## Decision Matrix

| Option | License | C++ Fit | macOS/build | Character controller | Queries | Rigid bodies/constraints | Determinism | Near-term fit | Long-term fit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Custom kinematic core | iggy3d-owned | Best | No new dependency | Exact iggy3d policy | Build only needed ray/sweep/overlap | None first | Best owned path | Best | Keep as gameplay query core |
| Jolt | MIT | Strong native C++ | Official macOS x64/ARM64; CMake examples | `Character` and `CharacterVirtual` | Strong collision/query surface | Strong rigid bodies and constraints | Explicit deterministic/cross-platform caveats | Too early | Best future dynamic-body candidate |
| Bullet | zlib | Native C++ but older API | Tested on Mac OSX; simple core compiler requirement | Built-in kinematic controller exists but needs care | `rayTest`, `convexSweepTest` | Mature rigid bodies/constraints | No clear current official cross-platform contract found | Too broad | Fallback candidate |
| PhysX | BSD 3-Clause for current repo; platform caveats for consoles | Native C++ but broad SDK | CMake preset workflow; heavier integration | CCT module, customizable kinematic controller | Raycasts, sweeps, overlaps | Very strong rigid bodies/constraints/articulations | Treat as iggy3d-owned replay burden | Too heavy | Candidate for complex dynamic/constraint needs |
| Rapier | Apache 2.0 | Rust-first, not native C++ | Rust/FFI cost for iggy3d | Kinematic controller via ray/shape casts | Raycast and shape-cast docs | Rigid bodies and joints | Optional cross-platform determinism | Poor for C++ packet | Research candidate if Rust accepted |
| Box2D | MIT | C17 but 2D only | Official Windows/Linux/Mac | 2D only | 2D only | 2D rigid bodies/joints | Useful reference only | Not a target | Reference only |

## Architecture Recommendation

Do not integrate a full physics engine now.

Packet 2 should continue as planned: content-owned spatial surfaces, traversal
tags, collision masks, and validation. After Packet 2, iggy3d should add a small
runtime collision query core over those authored surfaces. That core should
support deterministic O(surface count) ray/segment/AABB/capsule queries before
any rigid-body dependency enters the repo.

Recommended next packets after spatial surfaces land:

1. Collision Query Core
   - Build deterministic value-result queries over content spatial surfaces.
   - Implement height sample, normal sample, ray/segment test, overlap, and a
     first swept capsule/shape cast.
   - Return ordered results with exact statuses and no runtime mutation.
2. Kinematic Movement Params And Dot-Policy
   - Add movement radius/height, max walkable slope, step height, and ground snap
     as runtime-owned movement params.
   - Use dot product against world up to classify floor/slope/wall/ceiling.
   - Mutate only through `WorldState::updateTransform`.
3. Slope/Elevation Demo Room
   - Add an authored ramp/step/over-limit slope and prove world `y` is gameplay
     height, not decoration.
4. Projectile Runtime
   - Add deterministic projectile state and fixed-tick integration without
     collision first.
5. Projectile Collision And Visual Arc
   - Add ray/sweep collision against world/entity blockers and projection debug
     arc/impact markers.
6. Tactical Preview Over Spatial Data
   - Show slope/path/projectile previews only after runtime facts exist.
7. Dynamic Rigid Body Evaluation
   - Only after the above, spike Jolt first. Compare Jolt integration against the
     existing iggy3d physics abstraction and deterministic replay tests.

Likely future third-party candidate: Jolt Physics. It has the best combination
of C++ fit, permissive license, macOS support, game-oriented character/query
surface, and documented determinism caveats. PhysX is the second candidate only
if iggy3d needs heavier articulations/vehicles/industrial-grade simulation.

## Runtime Math Concepts To Lock

- Dot-product surface classification:
  - `worldUp = Vec3{0, 1, 0}`;
  - `upDot = dot(normalizedSurfaceNormal, worldUp)`;
  - slope angle is `acos(clamp(upDot, -1, 1))` in degrees;
  - wall is near `upDot == 0`;
  - ceiling is negative `upDot`.
- Plane/ray/segment/AABB/capsule math:
  - planes store finite normal and distance;
  - ray has origin and direction;
  - segment is ray plus finite length;
  - AABB overlap is inclusive and deterministic;
  - capsule is vertical segment plus radius for player movement.
- Swept capsule or shape cast:
  - test desired displacement against blocker surfaces;
  - return first time of impact and contact normal in deterministic tie order.
- Time of impact:
  - scalar fraction `0..1` along desired motion;
  - ties break by lower surface index, then stable id.
- Projection of desired motion onto a surface:
  - remove component into contact normal;
  - clamp to finite residual displacement;
  - preserve no-mutation-on-failure semantics.
- Slope speed policy:
  - belongs to runtime movement params;
  - content supplies normal/tags only.
- Step height and ground snap:
  - step accepts vertical delta at or below runtime `stepHeightMeters`;
  - snap accepts downward distance at or below runtime `groundSnapMeters`;
  - both require a walkable destination surface.
- Projectile integration:
  - first build should use fixed-tick semi-implicit Euler:
    `velocity += gravity * dt`, then `position += velocity * dt`;
  - all values finite;
  - lifetime decrements by fixed ticks.
- Ray/sweep projectile collision:
  - segment from previous to next position for small radius;
  - swept sphere/capsule when radius matters;
  - blockers filtered by collision mask.
- Contact normals and impact response:
  - impact event records kind, point, normal, target id/reference, and tick;
  - response can initially stop projectile and emit event.
- Solver lambda:
  - only relevant later for rigid-body constraints/joints;
  - do not introduce solver terminology into kinematic movement packets unless a
    dynamic rigid body engine is selected.

## Data Ownership Boundaries

- Content owns spatial surfaces, collision tags, masks, source mesh links, and
  authored opening/projectile-blocker semantics.
- Runtime owns physics state, collision query views, movement decisions,
  projectile simulation, impact events, and deterministic tick results.
- Projection owns debug views: sampled height, normal, slope labels, movement
  path, projectile arc, and impact markers.
- Vulkan/render draws meshes/debug lines/markers and emits receipts only. It
  never decides physics.
- Save/replay owns runtime state and runtime events. It does not own GPU
  buffers, render receipts, third-party engine scenes, or content parser
  scratch state.
- Multiplayer later needs authority over physics inputs, deterministic tick
  ordering, spawned projectile ids, and any dynamic-body state exposed to
  gameplay.

## Integration Risks

- Nondeterminism across platforms:
  - third-party engines often need strict API ordering and exact build flags;
  - broadphase/query result order can be nondeterministic unless sorted by
    stable ids.
- Floating-point drift:
  - slope thresholds, time of impact, and projectile integration must use
    explicit epsilons and deterministic tie-breaking.
- Dependency size and build complexity:
  - PhysX and Bullet are broad; Rapier adds Rust/FFI; Jolt is lighter but still a
    new external dependency.
- Licensing/redistribution:
  - all surveyed candidates have permissive licenses for normal desktop use, but
    PhysX console/platform caveats and third-party notices must be checked
    before distribution.
- Hidden engine ownership of gameplay truth:
  - do not let engine body ids, scene serialization, or callbacks become
    runtime authority.
- Physics scene lifetime coupled to render scene lifetime:
  - render resources must not create/destroy gameplay physics state.
- Query results depending on unordered containers or thread scheduling:
  - runtime APIs must sort/tie-break query results by stable content/runtime ids.
- Save/replay mismatch:
  - engine snapshots are not iggy3d save truth unless a future durability packet
    explicitly owns them.

## Final Recommendation

Next iggy3d physics step: keep Builder Dex on Packet 2, then build a custom
deterministic kinematic collision query core over authored spatial surfaces.

Do not integrate a full physics engine yet. The near-term gameplay asks for
capsule movement, slopes, steps, snap, ray/sweep/overlap queries, and projectile
collision. A full engine would add build, determinism, ownership, and save/replay
surface before the authored spatial contract is proven.

Likely future third-party engine candidate: Jolt Physics. Revisit Jolt after
iggy3d has:

- content-owned spatial surfaces;
- deterministic collision query tests;
- kinematic movement proof over elevation/slopes;
- projectile collision and replay proof;
- an iggy3d `PhysicsWorld`/`CollisionQuery` abstraction that can host either the
  custom core or a third-party backend.

PhysX should be kept as a later heavy-engine option for vehicles, articulations,
or complex constraints. Bullet and Rapier are useful references but not the
first integration target for a C++ macOS iggy3d runtime.
