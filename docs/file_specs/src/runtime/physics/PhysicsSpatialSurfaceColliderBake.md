# File Spec

Files:

- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp`
- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.cpp`

Verified at: `9288e13b`

## Owns

- Conversion from `SpatialSurfaceSet` collision surfaces into baked `PhysicsAabbCollider` packets.
- Bake request/config/result packets, status names, and reason codes.
- Inclusion policy for walkable, actor-blocking, projectile-blocking, opening, and sensor surfaces.
- Deterministic generated physics body ids and source-surface metadata arrays.
- Plane thickness and minimum half-extent expansion policy for generated AABBs.

## Does Not Own

- Authored room surface generation or `SpatialSurfaceSet` construction.
- Physics collider query, motor, broadphase, contact, or solver behavior.
- Player movement policy or session tick bake lifetime.
- Render mesh generation or product object catalog truth.

## Reads

- Caller-owned `SpatialSurfaceSet` and `CollisionSurfaceView` rows.
- Bake config values for thickness, minimum extents, first generated body id, inclusion flags, and sensor policy.
- Surface bounds, normal, role, shape, id, and runtime owner stable name.

## Writes / Mutates

- Local bake result packet with colliders and source metadata arrays.
- Generated collider body ids, bounds, centers, half extents, and sensor flags.
- No source surface mutation, no persistent collider cache, and no body store mutation.

## Calls Out To / Wires Out To

- `isValidPhysicsAabbCollider(...)` validates generated colliders.
- Baked colliders feed collision queries, kinematic motor, player movement planner, AI reasoning, session same-tick bake reuse, and tests.
- `Session.cpp` and `PlayerPhysicsMovePlanner.cpp` call `bakePhysicsAabbCollidersFromSpatialSurfaces(...)`.

## Called By / Entry Points

- Direct API: `isValidPhysicsSpatialSurfaceColliderBakeConfig(...)` and `bakePhysicsAabbCollidersFromSpatialSurfaces(...)`.
- Grep proof: `rg -n "bakePhysicsAabbCollidersFromSpatialSurfaces" src/runtime tests/unit`.

## Invariants

- Missing surface set or invalid config returns non-ok without colliders.
- Empty surface sets bake successfully with zero colliders.
- Included surfaces preserve deterministic source order.
- Default policy includes walkable and actor blockers, skips projectile-only blockers and openings.
- Included openings are rejected today because opening shape is unsupported by this AABB bake.
- Up-facing planes preserve the authored plane as the slab top face; down-facing planes preserve it as bottom face.
- Generated body ids must be valid nonzero `std::uint32_t` ids.

## Tests / Proof Commands

- `rg -n "physics_spatial_surface_collider_bake_tests" cmake tests/unit`.
- `rg -n "roomSurfacesBakeInDeterministicOrderAndPreserveMetadata|bakedFloorWorksWithGroundCheck|bakedWallWorksWithKinematicMotor" tests/unit/physics_spatial_surface_collider_bake_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/collision/SpatialSurfaceSet.*` unless collision surface view semantics change.
- `src/runtime/physics/PhysicsAabbCollider.*` unless generated collider validity changes.
- `src/runtime/player/PlayerPhysicsMovePlanner.*` unless planner bake consumption changes.

## Update When

- Bake request/config/result shape, inclusion policy, source metadata, generated body id policy, plane thickness, shape support, or generated collider semantics change.

## Do Not Update When

- Callers reuse existing baked colliders without changing bake contracts.
- Query, motor, or solver internals change while bake output stays the same.
