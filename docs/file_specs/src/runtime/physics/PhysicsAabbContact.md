# File Spec

Files:

- `src/runtime/physics/PhysicsAabbContact.hpp`
- `src/runtime/physics/PhysicsAabbContact.cpp`

Verified at: `f8ebbb1c`

## Owns

- AABB narrowphase contact request/result packets, contact packet, status names, and reason codes.
- Validation of broadphase pair indices, collider validity, pair body ids, same-body rejection, and overlap presence.
- Contact normal selection from the shallowest penetration axis.
- Penetration depth, contact point from intersection center, and sensor propagation.

## Does Not Own

- Broadphase pair collection.
- Material lookup or material pair combination.
- Contact solving, body delta application, or body store mutation.
- Collider construction or spatial-surface bake.

## Reads

- Caller-owned collider vector.
- Caller-owned `PhysicsBroadphasePair`.
- Collider bounds, world centers, body ids, and sensor flags.

## Writes / Mutates

- Local `PhysicsAabbContactResult` and `PhysicsAabbContact` packets.
- No collider, pair, body, material, or persistent runtime state.

## Calls Out To / Wires Out To

- `isValidPhysicsAabbCollider(...)` validates pair inputs.
- Core AABB helpers build the intersection used for contact point.
- Emitted contacts feed `PhysicsAabbContactSolver.*`, collision batches, and benchmarks.

## Called By / Entry Points

- Direct API: `generatePhysicsAabbContact(...)`.
- `PhysicsKernelBenchmark.cpp` benchmarks contact generation.
- Grep proof: `rg -n "generatePhysicsAabbContact|PhysicsAabbContact" src/runtime tests/unit`.

## Invariants

- Contact generation requires non-null colliders and pair pointers.
- Pair collider indices must be in range and pair body ids must match the referenced colliders.
- Same-body pairs are rejected even if collider bounds overlap.
- Negative overlap on any axis is `NoOverlap`.
- Ties in shallowest-axis selection keep the earliest axis candidate.
- Touching AABBs generate zero-penetration contacts.
- `includesSensor` propagates from the pair or either collider.

## Tests / Proof Commands

- `rg -n "physics_aabb_contact_tests" cmake tests/unit`.
- `rg -n "contactUsesShallowestAxisAndPositiveNormal|touchingAabbsGenerateZeroPenetrationContact|tiesPreferXAxisAndSensorIsPropagated" tests/unit/physics_aabb_contact_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsBroadphase.*` unless pair packet semantics change.
- `src/runtime/physics/PhysicsAabbContactSolver.*` unless contact packet interpretation changes.
- `src/runtime/physics/PhysicsAabbCollider.*` unless overlap/validity semantics change.

## Update When

- Contact packet shape, validation statuses, pair matching, overlap semantics, normal selection, penetration, point generation, or sensor propagation changes.

## Do Not Update When

- Broadphase candidate collection changes without changing emitted pair semantics consumed here.
- Solver math changes without changing contact packet meaning.
