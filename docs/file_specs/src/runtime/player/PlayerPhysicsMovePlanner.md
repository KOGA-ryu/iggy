# File Spec

Files:

- `src/runtime/player/PlayerPhysicsMovePlanner.hpp`
- `src/runtime/player/PlayerPhysicsMovePlanner.cpp`

Verified at: `f5f0a691`

## Owns

- Player physics movement planner request/result packets and status reason codes.
- Request validation for surfaces, body extents/id, desired displacement, bake config, and motor config.
- Optional use of caller-provided `precomputedSurfaceBake`, with fallback bake for direct callers.
- Conversion from spatial-surface bake to motor collider packets, motor execution, ground facts, and debug geometry.

## Does Not Own

- Session tick bake creation or lifetime.
- Movement command admission or world transform mutation.
- Physics broadphase/contact solver or persistent body storage.
- Cross-tick bake cache or surface revision scheme.

## Reads

- `SpatialSurfaceSet`, start center, body half extents, desired displacement, include-sensor flag, physics body id, planner config, and optional precomputed bake.
- Physics motor and ground query results.

## Writes / Mutates

- Returned planner result: status, reason code, upstream reason, final center, displacement facts, grounded/snap/block facts, hit lists, source surface ids, bake counts, and debug AABB geometry.
- No world state or persistent planner cache.

## Calls Out To / Wires Out To

- `bakePhysicsAabbCollidersFromSpatialSurfaces(...)` when no precomputed bake is supplied.
- `planPhysicsKinematicAabbMove(...)` for motor planning.
- `checkPhysicsGround(...)` for probe/snap grounding.

## Called By / Entry Points

- `MovementSystem.cpp` calls `planPlayerPhysicsMove(...)`.
- Direct planner tests call with and without `precomputedSurfaceBake`.
- Grep proof: `rg -n "planPlayerPhysicsMove|precomputedSurfaceBake|PlayerPhysicsMovePlanner" src/runtime tests/unit`.

## Invariants

- Direct planner callers still work through fallback bake.
- Precomputed bake is read-only and per-request only.
- Failed precomputed or fallback bake returns `SurfaceBakeFailed` with upstream reason and no movement.
- Walkable floor colliders are retained for ground checks but excluded from horizontal motor sweeps.

## Tests / Proof Commands

- `rg -n "player_physics_move_planner_tests" cmake tests/unit`.
- `rg -n "bakeSessionTickSurfaceColliders|precomputedSurfaceBake" src/runtime/session src/runtime/movement src/runtime/player tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/MovementSystem.*` unless movement adapter semantics change.
- `src/runtime/session/SessionTick.*` unless same-tick bake threading changes.
- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.*` and `PhysicsKinematicMotor.*` unless underlying physics contracts change.

## Update When

- Planner request/result packets, fallback bake behavior, precomputed bake handling, ground facts, or debug geometry change.
- Status/reason-code contract changes.

## Do Not Update When

- Session decides when to create a precomputed bake without changing planner request semantics.
