# File Spec

Files:

- `src/runtime/physics/PhysicsKernelBenchmark.hpp`
- `src/runtime/physics/PhysicsKernelBenchmark.cpp`

Verified at: `3b2f3fcb`

## Owns

- Physics kernel benchmark public enums, config, case request/result packet, suite result packet, status names, kernel names, and scenario names.
- Data fixtures for benchmark-only collider sets and synthetic room assets.
- Dispatch table from kernel/scenario pairs to benchmark runners.
- Case execution, optional timing collection, per-iteration counter accumulation, suite execution, and failed-case aggregation.
- Benchmark counters for broadphase, contact, solver, motor, spatial-surface bake, player move planner, full raycast, and segment any-hit paths.

## Does Not Own

- Physics kernel implementation correctness.
- Production simulation state, body stores, save/hash, app presentation, or renderer behavior.
- JSON serialization of benchmark packets.
- External benchmarking CLI/process orchestration.

## Reads

- `PhysicsKernelBenchmarkCaseRequest` and `PhysicsKernelBenchmarkConfig`.
- Runtime physics kernels: broadphase, contacts, solver, collision queries, kinematic motor, spatial-surface bake, and player move planner.
- Synthetic `RoomAsset` and `SpatialSurfaceSet` fixtures built inside this file.

## Writes / Mutates

- Local case/suite result packets and benchmark fixture vectors.
- Timing field when `collectTiming` is true.
- No runtime world/session state, persistent cache, save state, or source kernel state.

## Calls Out To / Wires Out To

- `collectPhysicsBroadphasePairs(...)`, `generatePhysicsAabbContact(...)`, `solvePhysicsAabbContact(...)`.
- `planPhysicsKinematicAabbMove(...)`, `bakePhysicsAabbCollidersFromSpatialSurfaces(...)`, `planPlayerPhysicsMove(...)`.
- `raycastPhysicsAabbs(...)` and `segmentHitsAnyPhysicsAabb(...)` for raycast-vs-any-hit comparison cases.

## Called By / Entry Points

- Direct API: `isValidPhysicsKernelBenchmarkConfig(...)`, `runPhysicsKernelBenchmarkCase(...)`, and `runPhysicsKernelBenchmarkSuite(...)`.
- JSON reporter calls the suite runner overload that accepts config.
- Grep proof: `rg -n "runPhysicsKernelBenchmarkCase|runPhysicsKernelBenchmarkSuite|PhysicsKernelBenchmark" src tests`.

## Invariants

- Config requires positive iterations and finite positive broadphase cell size.
- Unknown kernel/scenario pairs return `UnknownKernel` with stable names.
- `collectTiming=false` leaves elapsed nanoseconds at zero.
- Suite runs the default included handler rows in deterministic order.
- Full raycast and segment-any-hit comparison cases are callable directly but excluded from the default suite.
- Iteration accumulation sums per-iteration counters while preserving scenario-size counters such as collider and surface counts from the iteration result.
- This file is benchmark support only; it must not become production physics policy.

## Tests / Proof Commands

- `rg -n "physics_kernel_benchmark_tests" cmake tests/unit`.
- `rg -n "suiteRunsDefaultCasesInOrder|iterationCountersAccumulateDeterministically|aabbSegmentAnyHitBenchmarkComparesFullRaycastPath" tests/unit/physics_kernel_benchmark_tests.cpp`.

## Nearby Files Usually Not Touched

- Runtime kernel files unless their public result packet contracts change.
- `src/runtime/physics/PhysicsKernelBenchmarkJson.*` unless output serialization changes.
- App/window/render code; benchmark execution is runtime support, not product UI.

## Update When

- Kernel/scenario enums, default suite membership/order, config validation, case/suite packets, counter accumulation, timing semantics, or benchmark fixture scenarios change.

## Do Not Update When

- A measured kernel changes internal behavior but the benchmark harness contract and counters remain stable.
- JSON formatting changes without changing case/suite packet semantics.
