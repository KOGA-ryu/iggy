# File Spec

Files:

- `src/runtime/physics/PhysicsTypes.hpp`
- `src/runtime/physics/PhysicsTypes.cpp`

Verified at: `b2151294`

## Owns

- Physics id wrappers for bodies, shapes, and materials.
- Body motion, shape kind, weight class, and body-status enums.
- Stable lower-snake names for physics type/status values.
- `PhysicsBodyDescriptor` validation for motion, position, velocity, and mass facts.
- Inverse-mass policy for static, kinematic, and dynamic bodies.

## Does Not Own

- Persistent body storage or shape storage.
- AABB collider construction or collision query execution.
- Integration, broadphase, contact generation, or solver behavior.
- Save/load, app, projection, or render ownership.

## Reads

- `Vec3` finite checks from core math.
- Caller-provided body descriptors and enum values.

## Writes / Mutates

- Local `PhysicsValidationResult` packets only.
- No store state, world state, cache, save state, or debug projection state.

## Calls Out To / Wires Out To

- `computePhysicsInverseMass(...)` is consumed by `PhysicsBodyStore`.
- `validatePhysicsBodyDescriptor(...)` gates `PhysicsBodyStore::add(...)`.
- Name helpers are used by tests and status packet construction.

## Called By / Entry Points

- `PhysicsBodyStore.*` calls body validation and inverse-mass helpers.
- Physics store/collider modules include the id and enum contracts.
- Grep proof: `rg -n "validatePhysicsBodyDescriptor|computePhysicsInverseMass|PhysicsBodyMotionKind|PhysicsShapeKind" src/runtime tests/unit`.

## Invariants

- Zero-valued physics ids are invalid sentinels.
- Dynamic bodies require finite positive mass.
- Static and kinematic bodies use zero inverse mass.
- Static and kinematic bodies may carry finite nonnegative mass metadata.
- Invalid enum values map to explicit invalid status/name fallbacks.
- This file must remain app, window, render, projection, and save independent.

## Tests / Proof Commands

- `rg -n "physics_body_store_tests|physics_shape_store_tests" cmake tests/unit`.
- `rg -n "stableNamesAreLowerSnake|validationRejectsInvalidDescriptorFacts|inverseMassMatchesMotionPolicy" tests/unit/physics_body_store_tests.cpp tests/unit/physics_shape_store_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsBodyStore.*` unless body descriptor/storage semantics change.
- `src/runtime/physics/PhysicsShapeStore.*` unless shape id/kind contracts change.
- `src/runtime/physics/PhysicsAabbCollider.*` unless shape kind compatibility changes.

## Update When

- Public physics ids, enums, status names, body descriptor validation, or inverse-mass policy change.
- A new physics kind/status needs a stable public name.

## Do Not Update When

- Callers add new uses without changing the type contracts here.
- Store internals or solver math change while id/enum/validation semantics stay stable.
