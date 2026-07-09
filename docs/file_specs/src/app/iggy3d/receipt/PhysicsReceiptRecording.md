# File Spec

Files: `src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp`

Verified at: `0ba40cb9`

## Owns

- Recording physics movement planner tick proof into `ProductAppWindowState.gameplay.physicsMovementPlanner`.
- Mapping requested/collision-surface/stats-available facts to planner status and reason code.

## Does Not Own

- Physics movement planning.
- Collision surface bake or availability.
- Movement stats production.
- Receipt field emission for planner proof.

## Reads

- Caller-provided requested flag, collision-surface availability flag, and movement physics stats availability flag.

## Writes / Mutates

- Mutates `window.gameplay.physicsMovementPlanner.requested`.
- Mutates `window.gameplay.physicsMovementPlanner.used`.
- Mutates `window.gameplay.physicsMovementPlanner.status`.
- Mutates `window.gameplay.physicsMovementPlanner.reasonCode`.

## Calls Out To / Wires Out To

- No external module calls beyond local proof helper.

## Called By / Entry Points

- Automation gameplay, gameplay tape runner, and controller command execution paths record planner proof.
- Focused proof: `rg -n "recordProductPhysicsMovementPlannerTickProof|physics_movement_planner_status" src/app tests`.

## Invariants

- Disabled planner records disabled, not requested, and not used.
- Missing collision surfaces records requested but not used.
- Available movement physics stats records requested and used.
- Reason code mirrors status.
- This file records proof only and must not run planner logic.

## Tests / Proof Commands

- `rg -n "physics_movement_planner_status|recordProductPhysicsMovementPlannerTickProof" tests/unit tests/smoke src/app`.
- `rg -n "product_gameplay_controls_smoke|product_gameplay_tape_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless planner proof call sites change.
- `src/app/iggy3d/gameplay/TapeRunner.*` unless tape planner proof recording changes.
- `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp` unless emitted planner fields change.

## Update When

- Planner proof status mapping, source flags, or target proof fields change.

## Do Not Update When

- Only physics movement planning internals or collision bake internals change without changing proof recording.
