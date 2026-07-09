# File Spec

Files: `src/app/iggy3d/automation/AutomationGameplay.hpp`, `src/app/iggy3d/automation/AutomationGameplay.cpp`

Verified at: `67587af2`

## Owns

- Gameplay-domain automation handler for movement axes, attack, interact, jump, player-position override, physics movement planner toggle, and controller input sequences.
- Owner/readiness gate requiring gameplay screen, active gameplay flag, and active runtime session before mutating gameplay state.
- Conversion from automation command values into `ActionState`, controller action samples, or explicit player transform mutation.

## Does Not Own

- Command registry metadata or dispatch resolution.
- Product menu/frontend routing.
- Runtime movement planner internals.
- Controller action map definitions.
- Receipt field emission beyond calling the physics movement planner proof reset helper.

## Reads

- `FrontendState.screen`.
- `ProductAppWindowState.gameplay`, active room collision freshness, controller action routing state, and automation-control state.
- Active `Session` and mutable session state for player position override.
- Collision surfaces from active room collision freshness.

## Writes / Mutates

- Mutates gameplay input proof fields through `applyProductGameplayActions(...)`.
- Mutates player transform and current state hash for `gameplay.player_position`.
- Mutates `window.gameplay.physicsMovementPlanner.enabled`.
- Mutates `window.inputDevice.lastInputAction` and `lastInputAccepted`.
- Mutates automation-control last command result through `markAutomationApplied(...)`.

## Calls Out To / Wires Out To

- `routeInputAction(...)`.
- `applyProductGameplayActions(...)`.
- `ensureActiveRoomCollisionFresh(...)` and `productActiveRoomCollisionSurfaces(...)`.
- `processProductControllerActionSample(...)`.
- `computeStateHash(...)`.
- `recordProductPhysicsMovementPlannerTickProof(...)`.

## Called By / Entry Points

- `AutomationDispatch.cpp` calls `applyProductGameplayAutomationCommand(...)` after common, world setup, and room-editing handlers.
- Focused proof: `rg -n "applyProductGameplayAutomationCommand|gameplay.physics_movement|gameplay.player_position|controller.input" src/app tests/unit tests/smoke`.

## Invariants

- Gameplay automation fails as owner unavailable when not on gameplay with active gameplay and active session.
- Axis/button/jump commands route through input routing before applying gameplay actions.
- Player position override updates state hash after transform mutation.
- Physics movement planner false records a disabled planner tick proof.
- Controller input sequences are local to one command invocation; no persistent controller routing state is owned here.

## Tests / Proof Commands

- `rg -n "gameplay.physics_movement|gameplay.player_position|controller.input|gameplay.jump" tests/unit/product_automation_command_registry_tests.cpp tests/smoke`.
- `rg -n "product_gameplay_controls_smoke|product_gameplay_tape_smoke|product_automation_dispatch_tests" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/Controller*.{hpp,cpp}` unless gameplay action application contracts change.
- `src/app/iggy3d/input/ControllerAction*.{hpp,cpp}` unless controller automation token semantics change.
- `src/runtime/session/*` unless player transform mutation or state hash ownership changes.

## Update When

- Gameplay automation commands, readiness gates, action routing, player-position override, physics planner toggle, or controller-input sequence behavior changes.

## Do Not Update When

- Only movement physics internals or controller mapping tables change without changing automation command behavior.
