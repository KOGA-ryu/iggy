# File Spec

Files: `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp`, `src/app/iggy3d/gameplay/ActiveRoomCollision.cpp`

Verified at: `f288fbd8`

## Owns

- Product active-room collision state packet and query-surface access.
- Conversion from active room spatial surfaces into runtime `SpatialSurfaceSet`.
- Runtime-owned surface filtering against session world entities, including inactive door blockers.
- Collision proof counts for query surfaces, walkable surfaces, actor blockers, projectile blockers, runtime-owned surfaces, filtered surfaces, and door blockers.

## Does Not Own

- Active room loading/building, active room revision freshness, physics broadphase/solver, movement execution, or renderer presentation.
- Runtime entity activation semantics beyond filtering surfaces by owner active state.

## Reads

- `ProductActiveRoomState`, `RoomAsset` spatial surfaces, optional `SessionState`, runtime owner stable names, entity active flags, and room surface blocker roles.

## Writes / Mutates

- Returns `ProductActiveRoomCollisionState` with copied room ids/counts, filtered surfaces, status, reason code, and proof counts.
- Does not mutate active room state, app window state, runtime session state, or files.

## Calls Out To / Wires Out To

- `buildSpatialSurfaceSet(...)` from runtime collision.
- `SessionState::world.findByStableName(...)` for runtime-owned surface filtering.
- `productActiveRoomCollisionSurfaces(...)` returns a nullable pointer for movement, traversal, automation, tape runner, projection, and tests.

## Called By / Entry Points

- `buildProductActiveRoomCollision(...)` without runtime state.
- `buildProductActiveRoomCollision(...)` with runtime state.
- `productActiveRoomCollisionSurfaces(...)`.
- Grep proof: `rg -n "buildProductActiveRoomCollision|productActiveRoomCollisionSurfaces" src tests/unit tests/smoke`.

## Invariants

- Unloaded active rooms must not expose query surfaces.
- Loaded rooms with no query surfaces report unavailable/missing surfaces.
- Runtime-owned inactive surfaces are filtered before building the query set.
- Door blocker counts distinguish total door blockers from active door blockers.
- Collision state is a derived cache/projection of active room and runtime state, not durable save truth.

## Tests / Proof Commands

- `rg -n "product_active_room_collision_tests|product_ascii_room_activation_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductActiveRoomCollision|productActiveRoomCollisionSurfaces|runtimeFilteredSurfaceCount|activeDoorBlockerSurfaceCount" tests/unit/product_active_room_collision_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active room packet/count fields change.
- `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.*` unless freshness/rebuild policy changes.
- `src/runtime/collision/*` unless spatial surface query contracts change.

## Update When

- Collision packet fields, surface filtering, blocker counts, status rules, or query-surface access semantics change.

## Do Not Update When

- Only active room construction, save/load, gameplay input, or render presentation changes without collision-state contract changes.
