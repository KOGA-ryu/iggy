# File Spec

Files:

- `src/runtime/physics/PhysicsFrameStats.hpp`
- `src/runtime/physics/PhysicsFrameStats.cpp`

Verified at: `9288e13b`

## Owns

- `PhysicsFrameStats` packet shape for per-frame physics observability.
- Frame stats status names and reason codes.
- Empty ready stats construction.
- Accumulation from broadphase, kinematic motor, and player physics movement planner result packets.
- Packet outcome accounting for source packets, failed packets, status, and first upstream reason.

## Does Not Own

- Physics simulation, movement planning, collision queries, or debug rendering.
- Broadphase, motor, or player planner packet generation.
- Persistence, save/hash, product receipts, or app HUD formatting.

## Reads

- `PhysicsBroadphaseResult` counters and ok/reason.
- `PhysicsKinematicMotorResult` counters and ok/reason.
- `PlayerPhysicsMovePlannerResult` bake/movement counters and ok/reason.

## Writes / Mutates

- Caller-provided `PhysicsFrameStats&`.
- Counter totals, max bucket value, source/failed packet counts, aggregate status, reason code, and first upstream reason code.
- No source packet mutation and no persistent runtime state.

## Calls Out To / Wires Out To

- `PhysicsDebugSnapshot.*` reads frame stats and derives warning flags.
- `MovementSystem.cpp` builds stats and accumulates player planner facts.
- App projection/debug code can consume snapshots built from this packet.

## Called By / Entry Points

- Direct API: `buildPhysicsFrameStats(...)`, `accumulatePhysicsBroadphaseStats(...)`, `accumulatePhysicsKinematicMotorStats(...)`, and `accumulatePlayerPhysicsMovePlannerStats(...)`.
- Grep proof: `rg -n "buildPhysicsFrameStats|accumulatePhysics.*Stats|accumulatePlayerPhysicsMovePlannerStats" src/runtime src/app tests/unit`.

## Invariants

- Empty stats are ready, ok, and zero-counted.
- Each accumulation increments `sourcePacketCount`.
- Failed packets increment `failedPacketCount` and mark stats non-ok.
- First upstream reason is preserved once set.
- Broadphase max bucket uses maximum, not sum.
- Stats aggregation is observability only and must not mutate simulation facts.

## Tests / Proof Commands

- `rg -n "physics_frame_stats_tests" cmake tests/unit`.
- `rg -n "broadphaseCountersAccumulate|motorAndPlayerPlannerCountersAccumulate|broadphaseFailurePropagatesDeterministically" tests/unit/physics_frame_stats_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsDebugSnapshot.*` unless snapshot projection changes.
- `src/runtime/physics/PhysicsBroadphase.*` and `PhysicsKinematicMotor.*` unless source packet counters change.
- `src/runtime/player/PlayerPhysicsMovePlanner.*` unless player planner counters change.

## Update When

- Stats packet fields, accumulation sources, failure propagation, upstream reason policy, or counter aggregation semantics change.

## Do Not Update When

- A source system changes internal implementation but emits the same public result counters.
- App presentation changes without changing stats packet semantics.
