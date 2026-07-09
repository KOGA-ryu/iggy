# File Spec

Files:

- `src/runtime/physics/PhysicsBodyStore.hpp`
- `src/runtime/physics/PhysicsBodyStore.cpp`

Verified at: `b2151294`

## Owns

- `PhysicsBodyStore` as the runtime physics body row store.
- SoA vectors for body ids, motion kinds, positions, velocities, masses, and inverse masses.
- Body add, read, remove, reset, compaction, and id allocation.
- `PhysicsBodyView` and `PhysicsBodyStoreResult` packets.
- Friend access for body-delta application and body integration kernels.

## Does Not Own

- Shape storage, material storage, collider bake, broadphase, contact generation, or solving.
- Movement/session command policy.
- Save/load persistence or cross-run id stability.
- Product/app debug presentation.

## Reads

- `PhysicsBodyDescriptor` values supplied by callers.
- Validation and inverse-mass helpers from `PhysicsTypes`.
- Existing SoA rows when reading, removing, or building body views.

## Writes / Mutates

- Body SoA vectors and `nextId_`.
- Add appends validated rows and computed inverse mass.
- Remove erases matching rows from every SoA vector and preserves remaining order.
- Reset clears all rows and restarts ids from one.

## Calls Out To / Wires Out To

- `validatePhysicsBodyDescriptor(...)` before mutation on add.
- `computePhysicsInverseMass(...)` when storing row facts.
- Friend kernels in `PhysicsStep.*` and `PhysicsBodyDeltaAccumulator.*` mutate internal vectors through the declared seam.

## Called By / Entry Points

- Direct callers use `add(...)`, `read(...)`, `remove(...)`, and `reset(...)`.
- Physics step and delta-apply functions receive `PhysicsBodyStore*`.
- Grep proof: `rg -n "PhysicsBodyStore|stepPhysicsBodies|applyPhysicsBodyDeltas" src/runtime tests/unit`.

## Invariants

- All SoA vectors must remain the same length and row order.
- Invalid descriptors must not mutate the store.
- `read(...)` and `remove(...)` reject zero or missing ids with `BodyNotFound`.
- Remove compacts rows by erase and keeps the remaining relative order.
- Reset clears rows and restarts ids; ids are local runtime handles, not persistence ids.
- Store internals are exposed to physics integration only through friend seams.

## Tests / Proof Commands

- `rg -n "physics_body_store_tests" cmake tests/unit`.
- `rg -n "addAndReadBodyPreservesSoAFacts|invalidBodyDoesNotMutateStore|removePreservesRemainingOrderAndMissingIsStable|resetClearsStoreAndRestartsIds" tests/unit/physics_body_store_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsTypes.*` unless descriptor validation or inverse-mass policy changes.
- `src/runtime/physics/PhysicsStep.*` unless integration needs new store mutation access.
- `src/runtime/physics/PhysicsBodyDeltaAccumulator.*` unless delta application changes.

## Update When

- Store row layout, id allocation, validation, compaction, reset behavior, friend seams, or result packet semantics change.

## Do Not Update When

- A caller uses existing store APIs without changing their contract.
- Solver, broadphase, or collider math changes without touching body storage semantics.
