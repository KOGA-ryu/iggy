# File Spec

Files: `src/runtime/physics/PhysicsCollisionQueries.hpp`, `src/runtime/physics/PhysicsCollisionQueries.cpp`

Verified at: `f5f0a691`

## Owns

- Public request/result packet types for AABB overlap, raycast, swept AABB, ground check, and segment-hit queries.
- `PhysicsCollisionQueryStatus`, status-name mapping, and reason-code visibility.
- Validated execution for overlap, raycast, swept AABB, ground check, and segment-hit queries.
- Deterministic hit ordering for raycast and swept queries.
- Sensor filtering according to each request's `includeSensors` flag, plus non-sensor-only segment blocking.
- `segmentHitsAnyPhysicsAabb(...)` as an any-hit segment helper with margin-expanded hit tests.

## Does Not Own

- Collider bake from spatial surfaces.
- Body or shape stores.
- AI occlusion verdict policy.
- Movement command execution.
- Product/app debug rendering or projection.

## Reads

- Caller-owned `PhysicsAabbCollider` vectors/spans.
- Query collider, origin/direction, displacement, ground direction, probe distance, and sensor inclusion flags.
- `PhysicsBodyId`, `Aabb3`, `Vec3`, collider validity checks, and AABB/ray helpers.

## Writes / Mutates

- Local result packets: `ok`, status, reason code, tested counts, hit counts, hit arrays, nearest hit, and start-inside fields.
- Optional `startInside` output pointer in `segmentHitsAnyPhysicsAabb(...)`.
- No persistent runtime state and no collider ownership.

## Calls Out To / Wires Out To

- Core AABB/ray math via runtime geometry helpers.
- `queryPhysicsAabbOverlaps(...)` is reused by zero-displacement sweep handling.
- `sweepPhysicsAabb(...)` is reused by `checkPhysicsGround(...)`.
- Query declarations are consumed by physics motor, benchmark, AI occlusion, movement, and tests.

## Called By / Entry Points

- `src/runtime/ai/SegmentOcclusion.cpp` uses `segmentHitsAnyPhysicsAabb(...)`.
- `src/runtime/physics/PhysicsKernelBenchmark.cpp` calls raycast and segment queries.
- Grep proof: `rg -n '\braycastPhysicsAabbs\b|\bsegmentHitsAnyPhysicsAabb\b|\bsweepPhysicsAabb\b|\bcheckPhysicsGround\b' src tests`.

## Invariants

- Query requests carry caller-owned collider pointers/spans; this API does not own collider storage.
- Invalid public query requests return non-ok packets with explicit status/reason where supported.
- Hit arrays remain sorted by distance/fraction then collider index where sorting is part of the query contract.
- Sensors are skipped unless a request explicitly includes them; segment blockers skip sensors.
- `segmentHitsAnyPhysicsAabb(...)` expands hit bounds by margin, but `startInside` reports containment in original collider bounds.
- Segment-hit invalid inputs return `false` instead of throwing or mutating global state.
- Runtime physics code here must stay independent of app, window, render, projection, and save.

## Tests / Proof Commands

- `rg -n 'physics_collision_queries_tests|player_physics_move_planner_tests' cmake tests`.
- `rg -n '\bsegmentHitsAnyPhysicsAabb\b|\braycastPhysicsAabbs\b' tests/unit/physics_collision_queries_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsSpatialSurfaceColliderBake.*` unless query inputs change.
- `src/runtime/physics/PhysicsAabbCollider.*` unless collider validity or geometry shape changes.
- `src/runtime/ai/SegmentOcclusion.*` unless AI verdict mapping changes.

## Update When

- Query request/result packet shape changes.
- Status enum, reason-code contract, validation semantics, hit ordering, sensor filtering, or start-inside behavior changes.
- Public query functions are added, removed, reinterpreted, or delegated to a new kernel.

## Do Not Update When

- Only benchmarks change.
- Callers add new uses without changing this file's query contracts.
- Private query implementation details change without behavior or contract impact.
