# File Spec

Files:

- `src/runtime/movement/MovementSystem.hpp`
- `src/runtime/movement/MovementSystem.cpp`

Verified at: `f5f0a691`

## Owns

- Movement request validation, distance limits, movement blocked reason names, and movement execution entry points.
- Legacy collision-aware movement and physics-planned movement adapter.
- `MovementSystemContext`, including optional same-tick `precomputedSurfaceBake` forwarding.
- Movement result debug packets and travel/slope facts.

## Does Not Own

- Session command admission or tick orchestration.
- Player physics planner internals.
- Physics collider bake creation except through planner fallback.
- Cross-tick bake cache or surface revision ownership.

## Reads

- `WorldState`, `RuntimeConfig`, optional collision surfaces, movement requests, and context flags.
- Precomputed bake only when the planner request uses default-equivalent surface bake config.

## Writes / Mutates

- Actor transforms in `WorldState` when movement succeeds.
- Returned `MovementResult` facts: blocked reason, final position, slope/travel facts, physics frame stats, debug AABB packets, hits, and source surface ids.

## Calls Out To / Wires Out To

- `planPlayerPhysicsMove(...)` for physics-planned player movement.
- `sampleSurfaceHeightAtOrBelow(...)`, `sampleSlope(...)`, and collision query helpers for ground checks.
- `computeMovementTravelFacts(...)` for movement observability.

## Called By / Entry Points

- `SessionTick.cpp` calls `executeMovement(...)`.
- Tests and runtime callers use `movementRequestFromAcceptedCommand(...)`, `validateMovementRequest(...)`, and movement helpers.
- Grep proof: `rg -n "executeMovement|MovementSystemContext|precomputedSurfaceBake" src/runtime/session src/runtime/movement src/runtime/player tests/unit`.

## Invariants

- Precomputed bake forwarding is optional and only used when config matches default bake settings.
- No same-tick bake is stored beyond the planner request.
- Movement result debug geometry is observability, not physics truth storage.
- Kinematic and physics-planned movement remain separate execution paths.

## Tests / Proof Commands

- `rg -n "player_physics_move_planner_tests|session_tick_tests" cmake tests/unit`.
- `rg -n "bakePhysicsAabbCollidersFromSpatialSurfaces" src/runtime/session src/runtime/session/SessionTick.cpp src/runtime/movement src/runtime/player src/runtime/ai/ReasoningGraph.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/player/PlayerPhysicsMovePlanner.*` unless planner request/result semantics change.
- `src/runtime/session/SessionTick.*` unless movement context handoff changes.
- `src/runtime/physics/*` unless physics query or bake contracts change.

## Update When

- Movement context shape, planner handoff, result debug facts, or blocked reason semantics change.
- Same-tick bake forwarding rules change.

## Do Not Update When

- Pure player planner internals change without changing movement adapter behavior.
