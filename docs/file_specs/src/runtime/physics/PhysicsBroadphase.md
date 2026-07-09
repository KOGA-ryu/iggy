# File Spec

Files:

- `src/runtime/physics/PhysicsBroadphase.hpp`
- `src/runtime/physics/PhysicsBroadphase.cpp`

Verified at: `f8ebbb1c`

## Owns

- Grid broadphase request/result packets, pair packets, status names, and broadphase metrics.
- AABB collider validation before candidate collection.
- Cell-entry generation from collider AABB bounds and configurable cell size.
- Candidate pair collection, duplicate rejection, deterministic pair ordering, same-body filtering, final overlap filtering, and sensor-pair tagging.

## Does Not Own

- Collider construction or collider storage lifetime.
- Narrowphase contact generation.
- Contact solving, body mutation, material lookup, or movement policy.
- App/render debug presentation of broadphase results.

## Reads

- Caller-owned `std::vector<PhysicsAabbCollider>`.
- Collider bounds, body ids, and sensor flags.
- Cell size from `PhysicsBroadphaseRequest`.

## Writes / Mutates

- Local result packet metrics and generated `PhysicsBroadphasePair` vector.
- No collider mutation, body mutation, persistent grid cache, or cross-frame state.

## Calls Out To / Wires Out To

- `isValidPhysicsAabbCollider(...)` gates collection.
- `physicsAabbOverlaps(...)` performs final overlap filtering.
- Generated pairs are consumed by `PhysicsAabbContact.*`, benchmarks, and batch collision pipeline callers.

## Called By / Entry Points

- Direct API: `collectPhysicsBroadphasePairs(...)`.
- `PhysicsKernelBenchmark.cpp` benchmarks broadphase collection.
- Grep proof: `rg -n "collectPhysicsBroadphasePairs|PhysicsBroadphasePair" src/runtime tests/unit`.

## Invariants

- Missing collider vector or invalid cell size returns non-ok without candidate work.
- Invalid collider returns non-ok with invalid collider index and no emitted pairs.
- Empty and single-collider inputs are valid no-pair results.
- Candidate keys are normalized by collider index, sorted, deduplicated, then tested.
- Same-body candidates are skipped before emitted pairs.
- Pair ordering is deterministic by collider index order.
- Sensors are not filtered here; pairs only record whether a sensor is included.

## Tests / Proof Commands

- `rg -n "physics_broadphase_tests" cmake tests/unit`.
- `rg -n "overlappingPairsAreCollectedDeterministically|candidateOrderingIsDeterministicByColliderIndex|invalidGridConfigRejectsBeforePairCollection" tests/unit/physics_broadphase_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsAabbCollider.*` unless collider validity or overlap semantics change.
- `src/runtime/physics/PhysicsAabbContact.*` unless pair packet semantics change.
- `src/runtime/physics/PhysicsKernelBenchmark.*` unless only benchmark scenario data changes.

## Update When

- Broadphase request/result shape, cell mapping, candidate ordering, duplicate filtering, same-body filtering, sensor tagging, or metrics change.

## Do Not Update When

- Narrowphase or solver internals change without changing broadphase pair contracts.
- Benchmarks add scenarios without changing broadphase behavior.
