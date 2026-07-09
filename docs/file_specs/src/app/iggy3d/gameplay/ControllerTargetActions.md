# File Spec

Files: `src/app/iggy3d/gameplay/ControllerTargetActions.hpp`, `src/app/iggy3d/gameplay/ControllerTargetActions.cpp`

Verified at: `74fcbc90`

## Owns

- Product target command submission for interact and attack actions.
- Target query, reach query, reach-gate proof, and target proof recording before command submission.
- Interaction/attack executed flags after accepted command and advanced tick.
- Interaction outcome before/after proof capture.

## Does Not Own

- Target query algorithms, reach query algorithms, command admission rules, interaction effects, combat damage rules, input phase ordering, or rendering.

## Reads

- `Session`, command kind, product window state, player actor, target query result, reach query result, interaction outcome snapshot, and optional collision surfaces.

## Writes / Mutates

- Mutates window target proof, reach gate, gameplay command status, input source, interaction outcome proof, `interactionExecuted`, and `attackExecuted`.
- Submits target command to the session through gameplay command execution.

## Calls Out To / Wires Out To

- `queryTarget(...)`.
- `queryReach(...)` and `rejectionReasonForReach(...)`.
- `recordProductTargetProof(...)`, `makeProductInteractionOutcomeSnapshot(...)`, and `recordProductInteractionOutcomeProof(...)`.
- `submitProductGameplayCommand(...)`.

## Called By / Entry Points

- `submitProductTargetCommand(...)`.
- `ControllerActionPhases.*`.
- Grep proof: `rg -n "submitProductTargetCommand|interactionExecuted|attackExecuted|gameplayReachGate" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- No-target actions set command status to `no_target` and do not submit a session command.
- Reach gate is recorded before command execution.
- Attack commands set attack damage locally before handoff.
- Interaction outcome proof is recorded only for interact commands.
- Executed flags require accepted command and advanced gameplay tick.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_ascii_package_smoke|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "no_target|interactionExecuted|attackExecuted|gameplayReachGate|reach" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_ascii_package_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/targeting/*` unless target/reach query contracts change.
- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless command handoff changes.
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.*` unless proof packet semantics change.

## Update When

- Target command selection, no-target behavior, reach proof, interact/attack command payload, executed flags, or interaction outcome proof changes.

## Do Not Update When

- Only movement, dash, reset, input binding, target-query internals, or UI receipt formatting changes.
