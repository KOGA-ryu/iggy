# Spatial Surface Set

File:

- `/Users/kogaryu/iggy3d/src/runtime/collision/SpatialSurfaceSet.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/collision/SpatialSurfaceSet.cpp`

Verified at: `00e97746`

## Owns

- Runtime collision-facing `SpatialSurfaceSet` container over `CollisionSurfaceView` rows.
- Conversion from content `RoomAsset::spatialSurfaces` into runtime collision surface views.
- Optional world-offset bake for room surface points.
- Surface role, shape, collision-mask, opening, owner stable-name, and traversal-tag projection into collision rows.

## Does Not Own

- Authoring or validating `RoomAsset` text/files.
- Querying surfaces after conversion; that belongs to `CollisionQuery` and physics bake/query systems.
- App active-room collision freshness or render projection.

## Reads

- `RoomAsset::spatialSurfaces`.
- `RoomSpatialSurface` id, shape, role, points, normal, collision masks, blocker flags, owner stable name, and traversal tags.
- Optional world offset.

## Writes / Mutates

- Writes only the private vector inside a newly built `SpatialSurfaceSet`.
- Skips invalid or incomplete source surfaces instead of mutating the `RoomAsset`.

## Calls Out To / Wires Out To

- Produces `CollisionSurfaceView` rows consumed by collision queries, movement, projectile, session, and AI occlusion callers.
- Uses core AABB/vector math to build bounds and normalize normals.

## Called By / Entry Points

- `SpatialSurfaceSet::surfaces()`
- `SpatialSurfaceSet::empty()`
- `SpatialSurfaceSet::size()`
- `buildSpatialSurfaceSet(const RoomAsset&)`
- `buildSpatialSurfaceSet(const RoomAsset&, Vec3 worldOffsetMeters)`

## Invariants

- Empty ids, empty point lists, non-finite points, invalid bounds, and non-finite or zero normals are skipped.
- Bounds are built from all source points after applying the supplied world offset.
- Normals are normalized before being stored.
- Actor/projectile masks are derived from string mask entries while blocker booleans are preserved.
- No app/window/render dependencies belong here.

## Tests / Proof Commands

- `rg -n "SpatialSurfaceSet|buildSpatialSurfaceSet|spatial_surface" cmake/iggy3d_tests.cmake tests/unit src/runtime src/app/iggy3d`
- `collision_query_tests`, `physics_spatial_surface_collider_bake_tests`, and room asset tests cover consumers of this surface.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionQuery.*`
- `/Users/kogaryu/iggy3d/src/runtime/physics/PhysicsSpatialSurfaceColliderBake.*`

## Update When

- Room spatial surface fields, mask mapping, offset semantics, or skip rules change.

## Do Not Update When

- Only query logic, physics collider bake internals, or app active-room cache policy changes.
