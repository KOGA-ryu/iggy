# File Spec

Files: `src/app/iggy3d/gameplay/ControllerCommandExecution.hpp`, `src/app/iggy3d/gameplay/ControllerCommandExecution.cpp`

Verified at: `74fcbc90`

## Owns

- Product gameplay command submission wrapper over `Session::submitCommand(...)`.
- Accepted-command tick advancement and product command proof fields.
- Move-command collision proof, movement debug recording, physics movement planner tick proof, movement blocked/moved/tick-failed status, and ledge-fall fallback.

## Does Not Own

- Action-specific command creation, runtime command admission rules, movement planner algorithms, collision surface freshness, input phase ordering, or renderer/receipt field writing.

## Reads

- `Session`, `ProductAppWindowState`, command record, optional collision surfaces, player position before/after command, movement debug mirrors, physics movement planner enabled flag, and session transient movement stats.

## Writes / Mutates

- Submits command to session and may advance session tick.
- Writes gameplay command submitted/kind/accepted/status, rejection/reach gate, collision surface proof, tick proof, movement status, player-position-changed flag, and physics planner proof.
- May mutate player position for ledge-fall move fallback.

## Calls Out To / Wires Out To

- `Session::submitCommand(...)`, `Session::tick(...)`, and `Session::tickWithOptions(...)`.
- `recordProductMovementDebug(...)`, `recordProductPhysicsMovementPlannerTickProof(...)`, and movement proof helpers.
- Ground query and reset/fall helpers for ledge/fall handling.

## Called By / Entry Points

- `submitProductGameplayCommand(...)`.
- Move, dash, target, and scripted gameplay paths.
- Grep proof: `rg -n "submitProductGameplayCommand|gameplayTickAdvanced|recordProductPhysicsMovementPlannerTickProof|grounded_ledge_fall" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Command proof fields are updated before and after session submission.
- Accepted commands advance one gameplay tick through the configured movement planner path.
- Move command status is derived from tick success, runtime movement debug, actual position change, and fallback handling.
- Physics movement planner proof must say whether planner was enabled, collision surfaces were present, and movement frame stats were observed.
- Ledge-fall fallback applies only for eligible blocked move reasons while the jump system is not active.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplayTickAdvanced|gameplayMovement.status|physicsMovementPlanner|grounded_ledge_fall|submitProductGameplayCommand" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerMoveActions.*` unless move command creation changes.
- `src/runtime/session/Session.*` unless command/tick APIs change.
- `src/runtime/movement/*` unless movement result/debug semantics change.

## Update When

- Command submission proof, tick advancement, movement status derivation, physics planner proof, collision proof, or ledge-fall fallback changes.

## Do Not Update When

- Only input binding, action phase ordering, target selection, or receipt field formatting changes.
