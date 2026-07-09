# File Spec

Files:

- `src/runtime/physics/PhysicsAabbContactSolver.hpp`
- `src/runtime/physics/PhysicsAabbContactSolver.cpp`

Verified at: `f8ebbb1c`

## Owns

- AABB contact solve request/config/plan packets, solve status names, and reason codes.
- Validation of contact, body views, material pair traits, and solve config.
- Position correction plan generation weighted by inverse mass.
- Optional velocity impulse solve with restitution and optional tangent friction impulse.
- Sensor/no-solve handling and no-effective-mass handling.

## Does Not Own

- Contact generation or broadphase.
- Material pair combination.
- Body store mutation or delta application.
- Persistent solver cache, iteration scheduling, or app/debug rendering.

## Reads

- Caller-owned `PhysicsAabbContact`.
- Caller-owned first/second `PhysicsBodyView` values.
- Caller-owned `PhysicsMaterialPairTraits`.
- Solve config values for slop, correction percent, max correction, velocity solve, and friction.

## Writes / Mutates

- Local `PhysicsAabbContactSolvePlan` only.
- Planned position and velocity deltas, impulse magnitudes, effective inverse mass, and status flags.
- No body store rows, collider vectors, contacts, or material table state.

## Calls Out To / Wires Out To

- Uses vector math helpers for normalizing, dot product, tangent speed, and finite checks.
- Solver plans are intended for later body-delta accumulation/application.
- `PhysicsKernelBenchmark.cpp` benchmarks solve planning.

## Called By / Entry Points

- Direct API: `isValidPhysicsAabbContactSolveConfig(...)` and `solvePhysicsAabbContact(...)`.
- Grep proof: `rg -n "solvePhysicsAabbContact|PhysicsAabbContactSolvePlan|PhysicsAabbContactSolveConfig" src/runtime tests/unit`.

## Invariants

- Null contact/body/material inputs return non-ok status except sensor contacts, which are ok no-solve plans.
- Contact body ids must match the supplied body views.
- Contact normal must be finite and nonzero; penetration must be finite and nonnegative.
- Effective inverse mass must be positive enough before correction or impulses are planned.
- Position correction is capped by config and split by inverse mass.
- Separating velocity updates status but can still return a finite plan.
- Emitted plan deltas and impulse magnitudes must be finite.

## Tests / Proof Commands

- `rg -n "physics_aabb_contact_solver_tests" cmake tests/unit`.
- `rg -n "dynamicAgainstStaticGetsPositionCorrectionOnlyOnDynamicBody|dynamicDynamicCorrectionUsesInverseMassWeights|restitutionCreatesNormalVelocityDeltasForApproach|friction" tests/unit/physics_aabb_contact_solver_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsAabbContact.*` unless contact packet semantics change.
- `src/runtime/physics/PhysicsMaterialTraits.*` unless pair-trait semantics change.
- `src/runtime/physics/PhysicsBodyStore.*` and body-delta files unless applying plans changes.

## Update When

- Solve request/config/plan shape, validation, no-solve policy, position correction, restitution impulse, friction impulse, or status semantics change.

## Do Not Update When

- Contact generation or broadphase changes without changing solver input contracts.
- Body delta application changes without changing solve plan output semantics.
