# Collision Query

File:

- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionTypes.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionQuery.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionQuery.cpp`

Verified at: `00e97746`

## Owns

- Generic runtime collision query packet types: `CollisionSurfaceView`, `CollisionQueryResult`, `CollisionQueryStatus`, `CollisionQueryKind`.
- Stable string names for collision query statuses, surface roles, and surface shapes.
- Surface height and normal sampling over `SpatialSurfaceSet`.
- Segment, point-overlap, and AABB-overlap queries over `CollisionSurfaceView` rows.
- Deterministic hit selection for equal-height or equal-time candidates.

## Does Not Own

- Building surface sets from content assets; that belongs to `SpatialSurfaceSet`.
- Entity target hit testing; that belongs to `EntityHitQuery`.
- Hot physics AABB collider queries; those belong to `src/runtime/physics/PhysicsCollisionQueries.*`.
- Movement, projectile, or AI policy decisions after a hit.

## Reads

- `SpatialSurfaceSet::surfaces()` for immutable `CollisionSurfaceView` rows.
- `CollisionSurfaceRole`, `CollisionSurfaceShape`, and actor/projectile mask flags.
- `Aabb3` and `Vec3` math helpers from core math.

## Writes / Mutates

- Does not mutate runtime state.
- Returns `CollisionQueryResult` packets with hit surface id, role, shape, point, normal, distance, time, height, counts, and reason code.

## Calls Out To / Wires Out To

- Uses core math helpers such as `contains`, `closestPoint`, `center`, `isFinite`, and distance functions.
- Keeps routing local to query kind matching; no app, render, save, or session dependencies.

## Called By / Entry Points

- `sampleSurfaceHeight(...)`
- `sampleSurfaceHeightAtOrBelow(...)`
- `sampleSurfaceNormal(...)`
- `querySegment(...)`
- `queryPointOverlap(...)`
- `queryAabbOverlap(...)`

## Invariants

- Empty surface sets return `EmptySurfaceSet` with `collision_empty_surface_set`.
- Non-finite points, invalid tolerances, degenerate segments, or invalid bounds return `InvalidInput`.
- Height sampling only considers walkable surfaces and selects the highest valid height, tie-broken by stable surface id.
- Segment queries select earliest time of impact, tie-broken by stable surface id.
- Point/AABB overlap queries return the lexicographically smallest matching surface id.
- Actor queries match actor blockers or actor masks; projectile queries match projectile blockers or projectile masks.

## Tests / Proof Commands

- `rg -n "collision_query_tests|CollisionQuery|querySegment|sampleSurfaceHeight" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `collision_query_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/physics/PhysicsCollisionQueries.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/EntityHitQuery.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/SpatialSurfaceSet.*`

## Update When

- Query status, result fields, query-kind matching, or deterministic selection policy changes.
- A new runtime system consumes collision query reason codes as a contract.

## Do Not Update When

- Only content parsing, physics broadphase, or app debug rendering changes.
