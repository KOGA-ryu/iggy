# File Spec

Files:

- `src/runtime/physics/PhysicsAabbCollider.hpp`
- `src/runtime/physics/PhysicsAabbCollider.cpp`

Verified at: `b2151294`

## Owns

- AABB collider descriptor, shape, collider, result, and status packet contracts.
- Stable status names and reason codes for collider build/validation results.
- AABB collider construction from explicit descriptor facts.
- AABB collider construction from `PhysicsShapeStore` rows for box and trigger-AABB shapes.
- Collider validity, overlap, and point-containment helpers.

## Does Not Own

- Persistent body or shape store rows.
- Spatial-surface collider bake.
- Broadphase candidate generation, collision queries, contact generation, or solver behavior.
- Render geometry or product object metadata.

## Reads

- `PhysicsBodyId`, shape facts, body position, and sensor flags from caller descriptors.
- `PhysicsShapeStore::read(...)` when building from a shape id.
- Core `Aabb3` and `Vec3` math helpers.

## Writes / Mutates

- Local `PhysicsAabbColliderResult` packets.
- Built collider facts: body id, world center, half extents, AABB bounds, and sensor flag.
- No persistent store state and no caller-owned collider vectors.

## Calls Out To / Wires Out To

- `PhysicsShapeStore::read(...)` for shape-backed builds.
- `aabbFromCenterExtents(...)`, `intersects(...)`, and `contains(...)` for bounds facts.
- Validity helpers are reused by collision queries, broadphase, contacts, motor, bake, benchmarks, AI, and projection tests.

## Called By / Entry Points

- Direct API: `buildPhysicsAabbCollider(...)`, `buildPhysicsAabbColliderFromShape(...)`, `isValidPhysicsAabbCollider(...)`, `physicsAabbOverlaps(...)`, and `physicsAabbContainsPoint(...)`.
- Grep proof: `rg -n "buildPhysicsAabbCollider|isValidPhysicsAabbCollider|physicsAabbOverlaps|physicsAabbContainsPoint" src/runtime tests/unit`.

## Invariants

- Collider builds require valid body id, finite body position, finite local center offset, and finite positive half extents.
- Built world center is body position plus local center offset.
- Bounds are derived from world center and half extents.
- Shape-backed build accepts `Box` and `TriggerAabb`; trigger shapes always produce sensor colliders.
- Invalid inputs return non-ok packets with stable reason codes and do not mutate stores.
- Touching AABBs count as overlap through core AABB intersection semantics.

## Tests / Proof Commands

- `rg -n "physics_aabb_collider_tests" cmake tests/unit`.
- `rg -n "buildColliderCreatesWorldBoundsFromBodyPositionAndOffset|shapeStoreHelperBuildsIdenticalBounds|shapeStoreHelperRejectsBadInputs|overlapUsesValidAabbBounds|pointContainmentUsesValidColliderBounds" tests/unit/physics_aabb_collider_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsShapeStore.*` unless shape row compatibility changes.
- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.*` unless baked collider facts change.
- `src/runtime/physics/PhysicsCollisionQueries.*` unless collider validity semantics change.

## Update When

- Collider descriptor/result shape, validation, sensor interpretation, shape-kind compatibility, bounds construction, overlap, or containment semantics change.

## Do Not Update When

- Callers add new query/motor/bake uses without changing the collider contract.
- Solver or broadphase algorithms change while collider facts and validation stay stable.
