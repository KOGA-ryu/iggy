# File Spec

Files: `src/app/iggy3d/gameplay/ControllerDashActions.hpp`, `src/app/iggy3d/gameplay/ControllerDashActions.cpp`

Verified at: `a65a2f58`

## Owns

- Product dash submission from move axes, camera yaw, and movement tuning.
- Dash request/accept/reject proof fields.
- Dash destination command creation and handoff to product gameplay command execution.

## Does Not Own

- Dash cooldown ticking, phase ordering, input binding, movement planner internals, collision surface construction, or player entity lookup policy outside the current submission.

## Reads

- `Session`, `ProductAppWindowState`, move axes, camera yaw, movement tuning, dash cooldown state, player entity, and optional collision surfaces.

## Writes / Mutates

- Mutates window gameplay input source, dash requested/accepted/status/reason, dash speed/distance/cooldown/direction, and movement profile proof fields.
- Submits a local-player move command to the session through product command execution.

## Calls Out To / Wires Out To

- `productPlayerEntity(...)`.
- `productManualFirstPersonDirection(...)`.
- `rejectProductDash(...)`.
- `submitProductGameplayCommand(...)`.

## Called By / Entry Points

- `submitProductDash(...)`.
- `ControllerActionPhases.*` dash phase.
- Grep proof: `rg -n "submitProductDash|gameplay_dash" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Cooldown rejects before player lookup and command submission.
- Missing player rejects explicitly.
- Accepted dash records proof fields before command handoff.
- Dash destination keeps the actor's current Y coordinate and uses first-person camera direction.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplay_dash|dash" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless dash scheduling changes.
- `src/app/iggy3d/gameplay/ControllerJumpDashState.*` unless cooldown/proof state changes.
- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless command handoff changes.

## Update When

- Dash acceptance/rejection, destination math, proof fields, cooldown handling at submit time, or command handoff changes.

## Do Not Update When

- Only cooldown advancement, input mappings, collision bake freshness, or rendering changes.
