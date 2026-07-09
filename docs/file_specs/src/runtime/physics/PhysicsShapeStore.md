# File Spec

Files:

- `src/runtime/physics/PhysicsShapeStore.hpp`
- `src/runtime/physics/PhysicsShapeStore.cpp`

Verified at: `b2151294`

## Owns

- `PhysicsShapeStore` as the runtime physics shape row store.
- SoA vectors for shape ids, shape kinds, local center offsets, half extents, and sensor flags.
- Shape descriptor validation, add, read, remove, reset, compaction, and id allocation.
- `PhysicsShapeView`, validation packets, store result packets, and shape-status names.

## Does Not Own

- Body storage or body motion state.
- AABB collider construction from shapes.
- Broadphase, contact generation, solver, or movement behavior.
- Product-facing object catalogs or render mesh truth.

## Reads

- Caller-provided `PhysicsShapeDescriptor` values.
- Shape id validity helpers and shape-kind names from `PhysicsTypes`.
- Existing SoA rows when reading, removing, or building shape views.

## Writes / Mutates

- Shape SoA vectors and `nextId_`.
- Add appends validated shape facts.
- Remove erases matching rows from every SoA vector and preserves remaining order.
- Reset clears all rows and restarts ids from one.

## Calls Out To / Wires Out To

- `validatePhysicsShapeDescriptor(...)` gates mutation on add.
- `PhysicsAabbCollider.*` reads shapes through `PhysicsShapeStore::read(...)`.
- Tests use shape kind/status names as stable reason-code proof.

## Called By / Entry Points

- Direct callers use `add(...)`, `read(...)`, `remove(...)`, and `reset(...)`.
- `buildPhysicsAabbColliderFromShape(...)` reads shape rows by id.
- Grep proof: `rg -n "PhysicsShapeStore|validatePhysicsShapeDescriptor|buildPhysicsAabbColliderFromShape" src/runtime tests/unit`.

## Invariants

- All SoA vectors must remain the same length and row order.
- Shape descriptors require valid kind, finite local center offset, and finite positive half extents.
- Invalid descriptors must not mutate the store.
- `read(...)` and `remove(...)` reject zero or missing ids with `ShapeNotFound`.
- Remove compacts rows by erase and keeps the remaining relative order.
- Reset clears rows and restarts ids; ids are local runtime handles, not persistence ids.

## Tests / Proof Commands

- `rg -n "physics_shape_store_tests|physics_aabb_collider_tests" cmake tests/unit`.
- `rg -n "addAndReadShapePreservesSoAFacts|invalidShapeDoesNotMutateStore|removePreservesRemainingOrderAndMissingIsStable|invalidShapeIdReadsFailWithStableReason" tests/unit/physics_shape_store_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsTypes.*` unless shape kind/id contracts change.
- `src/runtime/physics/PhysicsAabbCollider.*` unless collider construction from shapes changes.
- Product object catalog files; shape rows are runtime physics descriptors, not product metadata.

## Update When

- Store row layout, validation, id allocation, compaction, reset behavior, status names, or shape result packet semantics change.

## Do Not Update When

- A caller uses existing shape APIs without changing their contract.
- Collider query or solver behavior changes without touching shape storage semantics.
