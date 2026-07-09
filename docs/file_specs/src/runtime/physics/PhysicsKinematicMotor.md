# File Spec

Files:

- `src/runtime/physics/PhysicsKinematicMotor.hpp`
- `src/runtime/physics/PhysicsKinematicMotor.cpp`

Verified at: `9288e13b`

## Owns

- Kinematic AABB motor request/config/result packets, hit packets, status names, and reason codes.
- Request validation for collider vectors, body collider, desired displacement, max displacement, and motor config.
- Iterative swept-AABB movement planning with skin distance, blocking hits, bounded iteration count, and slide-along-plane remaining displacement.
- Ground probe and snap query integration.
- Sensor inclusion policy for motor sweeps and ground checks.

## Does Not Own

- Collider bake from spatial surfaces.
- Player-specific movement policy or authored room surface selection.
- Body store mutation, velocity integration, broadphase, contact generation, or solving.
- App input, camera, or debug presentation.

## Reads

- Caller-owned collider vector and body collider.
- Desired displacement, include-sensor flag, and motor config.
- Collision query results from swept AABB and ground check helpers.

## Writes / Mutates

- Local motor result packet with final center, applied/remaining displacement, hit list, grounded/snap facts, tested counts, and upstream reason code.
- No mutation of caller-owned colliders or body collider.
- No persistent motor state or cross-frame cache.

## Calls Out To / Wires Out To

- `queryPhysicsAabbOverlaps(...)` validates collider packets before motion.
- `sweepPhysicsAabb(...)` provides nearest blocking hit per iteration.
- `checkPhysicsGround(...)` provides grounded and snap facts.
- `PlayerPhysicsMovePlanner.cpp` calls `planPhysicsKinematicAabbMove(...)`.

## Called By / Entry Points

- Direct API: `isValidPhysicsKinematicMotorConfig(...)` and `planPhysicsKinematicAabbMove(...)`.
- `PhysicsKernelBenchmark.cpp` benchmarks motor scenarios.
- Grep proof: `rg -n "planPhysicsKinematicAabbMove|PhysicsKinematicMotorResult" src/runtime tests/unit`.

## Invariants

- Config requires one to eight iterations, finite nonnegative skin/probe/snap, positive min/max distances, and max distance at least min distance.
- Invalid requests return non-ok and no movement.
- Movement stops before impact by skin distance when blocked.
- Slide movement projects remaining displacement along the hit plane.
- Initial overlaps or unusable hit normals terminate remaining movement.
- Ground snap only runs after the ground probe fails and snap distance is positive.
- Sensor colliders are ignored unless `includeSensors` is true.

## Tests / Proof Commands

- `rg -n "physics_kinematic_motor_tests" cmake tests/unit`.
- `rg -n "wallCollisionStopsBeforeImpact|diagonalMovementSlidesAlongWall|groundProbeAndSnapUseQueryLayer|sensorPolicyIsHonored" tests/unit/physics_kinematic_motor_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsCollisionQueries.*` unless sweep/ground query contracts change.
- `src/runtime/player/PlayerPhysicsMovePlanner.*` unless player planner request/result mapping changes.
- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.*` unless motor input collider facts change.

## Update When

- Motor request/config/result shape, validation, iteration policy, slide behavior, skin stop, ground probe/snap behavior, sensor policy, or hit packet semantics change.

## Do Not Update When

- Player movement decides different desired displacement without changing motor API contracts.
- Collider bake internals change while emitted collider facts are unchanged.
