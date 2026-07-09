# File Spec

Files:

- `src/runtime/physics/PhysicsMaterialTraits.hpp`
- `src/runtime/physics/PhysicsMaterialTraits.cpp`

Verified at: `f8ebbb1c`

## Owns

- Physics material flags, descriptors, views, status names, validation packets, and pair-trait packets.
- `PhysicsMaterialTable` as the runtime material SoA table keyed by stable lower-snake material keys.
- Built-in physics material rows for debug floor/wall, stone, wood, metal, ice, rubber, cloth, and trigger.
- Material-pair combination policy: friction geometric mean, restitution maximum, damping minimum, flag union, trigger/no-solve derivation.

## Does Not Own

- Body, shape, collider, broadphase, contact, or solver storage.
- Product object catalogs, render materials, save catalog slots, or app-facing material UI.
- Collision response application to body stores.

## Reads

- Caller-provided `PhysicsMaterialDescriptor` values.
- `PhysicsMaterialView` values when combining material pairs.
- `PhysicsMaterialId`, `PhysicsWeightClass`, and id/name helpers from `PhysicsTypes`.

## Writes / Mutates

- Material table SoA vectors: ids, keys, friction, restitution, damping, weight class, and flags.
- `nextId_` for local runtime material ids.
- Local validation/table result packets and pair-trait packets.

## Calls Out To / Wires Out To

- `combinePhysicsMaterialPair(...)` feeds contact solver requests.
- `makeBuiltInPhysicsMaterialTable(...)` provides default runtime material facts for tests and physics pipeline callers.
- `readPhysicsMaterial(...)` and `findPhysicsMaterialByKey(...)` provide null-safe table access.

## Called By / Entry Points

- Direct API: material validation, add/read/find/reset, built-in table construction, and pair combine.
- `PhysicsAabbContactSolver.*` consumes `PhysicsMaterialPairTraits`.
- Grep proof: `rg -n "combinePhysicsMaterialPair|PhysicsMaterialTable|makeBuiltInPhysicsMaterialTable" src/runtime tests/unit`.

## Invariants

- Material keys must be nonempty lower-snake strings using lowercase letters, digits, and underscores.
- Static/dynamic friction must be finite and nonnegative.
- Restitution and damping multiplier must be finite values in unit range.
- Duplicate keys are rejected and do not mutate the table.
- Table SoA vectors must remain aligned by index.
- Trigger material flags produce `trigger=true` and `solveContact=false` in pair traits.
- Runtime material ids are local handles, not save/catalog identities.

## Tests / Proof Commands

- `rg -n "physics_material_traits_tests" cmake tests/unit`.
- `rg -n "builtInTableContainsRequiredMaterials|combinePhysicsMaterialPair|duplicateAndLookupFailuresAreStable" tests/unit/physics_material_traits_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsAabbContactSolver.*` unless solver interpretation of pair traits changes.
- `src/runtime/physics/PhysicsTypes.*` unless material id or weight class semantics change.
- Product/render material files; this file is runtime physics material truth only.

## Update When

- Material flags, descriptors, validation, built-in rows, table semantics, pair combine math, or trigger/no-solve policy changes.

## Do Not Update When

- A caller adds a new lookup or combines an existing pair without changing this file's contracts.
- Render material appearance changes without changing runtime physics traits.
