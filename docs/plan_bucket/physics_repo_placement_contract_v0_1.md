# Physics Repo Placement Contract v0.1

This document defines where the Iggy3D physics engine lives in the repository,
which existing systems it may touch, and the build slice order that prevents
builders from inventing file placement.

It depends on:

- `physics_math_foundation_v0_1.md`;
- `physics_step_pipeline_v0_1.md`;
- `physics_collision_queries_v0_1.md`;
- `physics_material_traits_v0_1.md`;
- `physics_lab_contract_v0_1.md`.

## Intent

Physics work must start as a runtime engine layer, not as product UI behavior
and not as a renderer feature.

The repo should separate:

```text
object metadata      -> src/runtime/object
physics engine truth -> src/runtime/physics
existing collision   -> src/runtime/collision
existing movement    -> src/runtime/movement and src/runtime/player
product labs later   -> src/app/iggy3d/labs
projection/debug     -> src/projection/debug
renderer/Vulkan      -> src/render/vulkan
```

This lets the engine grow without turning AppShell, movement, collision, or
render files into mixed-ownership code.

## Existing Repo Surfaces

Current relevant runtime folders:

```text
src/runtime/collision/
src/runtime/movement/
src/runtime/player/
src/runtime/session/
src/runtime/world/
src/runtime/object/
```

Current relevant product folders:

```text
src/app/iggy3d/gameplay/
src/app/iggy3d/room/
src/app/iggy3d/room_editor/
src/app/iggy3d/ascii_room/
src/app/iggy3d/view/
src/app/iggy3d/window/
```

`src/runtime/object/` already exists and is the correct home for object asset
metadata, interaction traits, material-facing object tags, and palette-facing
object definitions. It must not become the hot physics body solver.

## New Runtime Physics Folder

All new physics engine truth should live under:

```text
src/runtime/physics/
```

Expected files:

```text
src/runtime/physics/PhysicsTypes.hpp
src/runtime/physics/PhysicsTypes.cpp
src/runtime/physics/PhysicsMath.hpp
src/runtime/physics/PhysicsMath.cpp
src/runtime/physics/PhysicsBodyStore.hpp
src/runtime/physics/PhysicsBodyStore.cpp
src/runtime/physics/PhysicsShapeStore.hpp
src/runtime/physics/PhysicsShapeStore.cpp
src/runtime/physics/PhysicsMaterialTraits.hpp
src/runtime/physics/PhysicsMaterialTraits.cpp
src/runtime/physics/PhysicsStep.hpp
src/runtime/physics/PhysicsStep.cpp
src/runtime/physics/PhysicsBroadphase.hpp
src/runtime/physics/PhysicsBroadphase.cpp
src/runtime/physics/PhysicsCollisionQuery.hpp
src/runtime/physics/PhysicsCollisionQuery.cpp
src/runtime/physics/PhysicsContact.hpp
src/runtime/physics/PhysicsContact.cpp
src/runtime/physics/PhysicsSolver.hpp
src/runtime/physics/PhysicsSolver.cpp
src/runtime/physics/PhysicsDebugSnapshot.hpp
src/runtime/physics/PhysicsDebugSnapshot.cpp
src/runtime/physics/PhysicsLab.hpp
src/runtime/physics/PhysicsLab.cpp
```

Do not create one-file feature islands for every small physics concept. Use the
files above as consolidation targets.

## File Ownership

### PhysicsTypes

Owns small ids, enums, result statuses, common config values, and shared request
types.

Expected content:

```text
PhysicsBodyId
PhysicsShapeId
PhysicsMaterialId
PhysicsBodyKind
PhysicsShapeKind
PhysicsCollisionMask
PhysicsAabb
PhysicsStepStatus
PhysicsQueryStatus
```

No solver loops.

No AppShell/product includes.

### PhysicsMath

Owns reusable math helpers that are physics-specific.

Expected content:

```text
isFiniteVec3
lengthSquared
normalizeSafe
overlapAabb
closestPointOnAabb
expandedAabb
rayAabbSlab
```

This file is where approved `sqrt` or division-heavy helpers should be named and
tested.

No object/material policy.

### PhysicsBodyStore

Owns hot body runtime data in SoA form.

Expected content:

```text
positions[]
previousPositions[]
velocities[]
inverseMasses[]
shapeIds[]
materialIds[]
flags[]
aabbs[]
```

This is runtime truth for physics bodies.

It may import cold body descriptors from object/session code later through an
adapter. It must not depend on product UI.

### PhysicsShapeStore

Owns compact runtime shape definitions.

Expected first shapes:

```text
box
capsule
floor_span
wall_slab
trigger_aabb
```

Renderer meshes are not physics shapes. ASCII room data may eventually compile
into these shapes through an adapter.

### PhysicsMaterialTraits

Owns runtime physics material tables and material-pair tables.

It may relate to `src/runtime/object/ObjectTraits.*`, but it must not collapse
into object metadata. Object traits are authored/cold data. Physics material
traits are runtime tables used by query/solver loops.

### PhysicsStep

Owns the fixed tick pipeline.

Expected phase calls:

```text
applyForces
integrateVelocities
buildBroadphase
generateContacts
buildIslands
warmStartContacts
solveVelocityConstraints
solvePositionConstraints
integratePositions
publishSnapshot
```

It should coordinate phases. It should not contain all narrowphase or solver
math inline.

### PhysicsBroadphase

Owns broadphase data structures and candidate generation.

First algorithm:

```text
uniform spatial hash grid
```

It owns counters:

```text
occupied_cell_count
max_bucket_size
candidate_pair_count
duplicate_pair_rejected_count
```

### PhysicsCollisionQuery

Owns named query APIs:

```text
queryAabbOverlap
queryClosestPointOnAabb
queryRayCast
querySweep
queryGround
queryCapsule
```

It may use `PhysicsMath` and `PhysicsBroadphase`. It must not call renderer or
AppShell code.

### PhysicsContact

Owns contact point/manifold structs and contact cache identity.

Expected content:

```text
PhysicsContactPoint
PhysicsContactManifold
PhysicsContactFeatureId
PhysicsContactCache
```

Warm starting belongs here and in solver, not in product code.

### PhysicsSolver

Owns sequential impulse solver implementation.

Expected content:

```text
normal impulse solve
friction impulse solve
restitution threshold
Baumgarte or position correction
warm start application
```

No shape-pair dispatch and no broadphase.

### PhysicsDebugSnapshot

Owns read-only debug facts and sampled rows.

It may later feed projection/debug. It must not emit product receipts directly.

### PhysicsLab

Owns no-window lab scenario setup and proof results.

It should use runtime physics APIs. It must not fake behavior that the runtime
does not support yet.

## Existing System Boundaries

### `src/runtime/collision/`

Existing collision remains the current movement/projectile collision system.

Do not rewrite it during physics foundation work.

Allowed later:

```text
PhysicsStaticRoomAdapter.*
```

This adapter may convert existing `SpatialSurfaceSet` or room collision data
into physics static shapes.

### `src/runtime/movement/` and `src/runtime/player/`

Existing movement and first-person player motor stay active until a specific
replacement/integration contract exists.

Physics foundation slices must not change:

```text
MovementSystem
MovementTraversal
PlayerMotor
SessionTick movement execution
```

Allowed later:

```text
PlayerPhysicsController.*
```

Only after physics lab proves room collision, ground checks, and wall sweep.

### `src/runtime/object/`

Object traits are cold object metadata.

Allowed:

- asset definitions;
- editor palette traits;
- object physics profiles;
- interaction verbs;
- material labels;
- save flags.

Not allowed:

- hot body arrays;
- solver loops;
- broadphase buckets;
- contact manifolds;
- per-tick physics state.

### `src/runtime/session/`

No session integration until physics runtime/lab proof exists.

Allowed later:

```text
SessionPhysicsAdapter.*
```

It should map session/world objects to physics bodies and map physics results
back to runtime state.

### `src/app/iggy3d/`

No AppShell or product UI dependency in foundation slices.

Allowed later:

```text
src/app/iggy3d/labs/ProductPhysicsLab.*
```

Only after `PhysicsLab.*` exists and passes no-window unit proof.

### `src/render/vulkan/`

Vulkan does not own physics.

Allowed later:

- render physics debug draw generated by projection;
- draw collider overlays;
- draw contact normals;
- draw lab preview.

Not allowed:

- use render mesh triangles as default collision truth;
- put physics step code in Vulkan files;
- let Vulkan frame delta drive physics directly.

## Tests

Physics unit tests should live under:

```text
tests/unit/physics_math_tests.cpp
tests/unit/physics_body_store_tests.cpp
tests/unit/physics_shape_store_tests.cpp
tests/unit/physics_material_traits_tests.cpp
tests/unit/physics_step_tests.cpp
tests/unit/physics_broadphase_tests.cpp
tests/unit/physics_collision_query_tests.cpp
tests/unit/physics_solver_tests.cpp
tests/unit/physics_debug_snapshot_tests.cpp
tests/unit/physics_lab_tests.cpp
```

Do not create product smokes for foundation math before runtime tests exist.

Product/no-window smokes become legal only after lab-level runtime proof exists.

## CMake Placement

Runtime physics `.cpp` files should be added to the main app/runtime source list
in `CMakeLists.txt` beside other `src/runtime/*` sources.

New tests should be registered in:

```text
cmake/iggy3d_tests.cmake
```

Use focused CTest targets:

```text
ctest --test-dir build --output-on-failure -R '^physics_step_tests$'
```

No broad CTest loop is required for narrow physics slices unless a shared API is
changed.

## Naming Rules

Use `Physics*` for runtime physics engine files and types.

Use `Object*` only for object metadata and object catalogs.

Use `Product*` only for product app/UI/lab surfaces.

Use `Runtime*` only for generic runtime diagnostics already following that
pattern.

Stable reason codes should use lower snake case:

```text
physics_step_ready
physics_step_invalid_world
physics_body_store_ready
physics_query_no_hit
physics_query_hit
physics_material_invalid_friction
physics_lab_drop_completed
```

Do not use `ProductPhysics*` for runtime engine truth.

## Slice Order

### Slice 1: Physics Types And Body Store

Allowed files:

```text
src/runtime/physics/PhysicsTypes.*
src/runtime/physics/PhysicsBodyStore.*
tests/unit/physics_body_store_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Behavior:

- define ids/enums/status names;
- create body descriptors;
- store SoA body arrays;
- validate finite position/velocity/mass;
- compute inverse mass;
- static bodies use inverse mass zero.

No collision, step, solver, AppShell, product, render, save/load.

### Slice 2: Fixed Step Integration

Allowed files:

```text
src/runtime/physics/PhysicsStep.*
tests/unit/physics_step_tests.cpp
```

Behavior:

- fixed dt request/result;
- gravity acceleration;
- semi-implicit Euler;
- previous/current position update;
- counters.

No collision/contact/solver.

### Slice 3: Physics Math Helpers

Allowed files:

```text
src/runtime/physics/PhysicsMath.*
tests/unit/physics_math_tests.cpp
```

Behavior:

- AABB validation;
- overlap;
- closest point;
- expanded AABB;
- normalizeSafe;
- ray slab helper.

No engine integration beyond tests.

### Slice 4: Material Traits Model

Allowed files:

```text
src/runtime/physics/PhysicsMaterialTraits.*
tests/unit/physics_material_traits_tests.cpp
```

Behavior:

- built-in material descriptors;
- validation;
- id-to-index resolution;
- material-pair table;
- friction/restitution combine rules.

No solver integration.

### Slice 5: Shape Store

Allowed files:

```text
src/runtime/physics/PhysicsShapeStore.*
tests/unit/physics_shape_store_tests.cpp
```

Behavior:

- box/capsule/floor span/wall slab shape descriptors;
- shape validation;
- body shape id validation;
- AABB generation for basic shapes.

No broadphase/contact solver.

### Slice 6: Broadphase Uniform Grid

Allowed files:

```text
src/runtime/physics/PhysicsBroadphase.*
tests/unit/physics_broadphase_tests.cpp
```

Behavior:

- grid config;
- body AABB insertion;
- static shape insertion;
- candidate pair generation;
- deterministic ordering;
- counters.

No narrowphase.

### Slice 7: Collision Query Helpers

Allowed files:

```text
src/runtime/physics/PhysicsCollisionQuery.*
tests/unit/physics_collision_query_tests.cpp
```

Behavior:

- ray vs AABB;
- ground check against floor span;
- sweep against wall slab/AABB;
- query result statuses/counters.

No solver.

### Slice 8: Contact Manifold

Allowed files:

```text
src/runtime/physics/PhysicsContact.*
tests/unit/physics_contact_tests.cpp
```

Behavior:

- contact point/manifold structs;
- feature id;
- cache lookup/update;
- warm-start impulse preservation.

No solver application yet.

### Slice 9: Sequential Impulse Solver

Allowed files:

```text
src/runtime/physics/PhysicsSolver.*
tests/unit/physics_solver_tests.cpp
```

Behavior:

- normal impulse;
- friction impulse;
- restitution threshold;
- accumulated impulse clamping;
- static/dynamic body behavior.

No product integration.

### Slice 10: Lab Drop And Slide

Allowed files:

```text
src/runtime/physics/PhysicsLab.*
tests/unit/physics_lab_tests.cpp
```

Behavior:

- drop scenario;
- slide scenario;
- fixed step count;
- summary counters.

No CLI/product UI yet.

### Slice 11: Room Collision Adapter

Allowed files:

```text
src/runtime/physics/PhysicsStaticRoomAdapter.*
tests/unit/physics_static_room_adapter_tests.cpp
```

Behavior:

- convert existing static room/collision surfaces into physics floor spans and
  wall slabs;
- preserve existing collision system;
- prove authored room walls/floors become physics static shapes.

No movement replacement.

### Slice 12: Lab Wall Hit And Room Collision

Allowed files:

```text
src/runtime/physics/PhysicsLab.*
tests/unit/physics_lab_tests.cpp
```

Behavior:

- wall_hit scenario;
- room_collision scenario;
- ground and sweep proof against static room shapes.

No product UI.

### Slice 13: Product Physics Lab Surface

Allowed files:

```text
src/app/iggy3d/labs/ProductPhysicsLab.*
tests/unit/product_physics_lab_tests.cpp
tests/smoke/product_physics_lab_smoke.cpp
```

Behavior:

- consume runtime `PhysicsLab`;
- emit product receipt fields;
- no-window proof only.

No AppShell direct physics implementation. AppShell may route a request to the
product lab adapter only.

## Cartographer Registration Notes

The cartographer should register:

- `src/runtime/physics/` as the physics engine runtime owner;
- `src/runtime/object/` as object metadata, not solver storage;
- `src/runtime/collision/` as legacy/current collision query owner until
  physics adapters exist;
- `src/runtime/movement/` and `src/runtime/player/` as current movement/player
  systems not to be replaced during foundation slices;
- `src/app/iggy3d/labs/` as the future product-facing lab surface;
- the five physics plan-bucket docs plus this placement contract as the source
  of builder placement truth.

## Acceptance Gate

This contract is accepted when:

- builders can place new physics files without guessing;
- existing movement/collision/object folders have explicit boundaries;
- source slices are ordered from data model to lab proof;
- no product/render/session integration is allowed before runtime physics proof;
- file consolidation targets are named before one-off files appear.

