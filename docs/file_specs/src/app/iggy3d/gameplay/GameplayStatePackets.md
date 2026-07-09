# File Spec

Files: `src/app/iggy3d/gameplay/DashState.hpp`, `src/app/iggy3d/gameplay/GameplayMovementInfo.hpp`, `src/app/iggy3d/gameplay/JumpState.hpp`, `src/app/iggy3d/gameplay/OutcomeState.hpp`, `src/app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp`, `src/app/iggy3d/gameplay/ResetState.hpp`, `src/app/iggy3d/gameplay/TapeState.hpp`, `src/app/iggy3d/gameplay/TargetState.hpp`, `src/app/iggy3d/gameplay/TraversalState.hpp`, `src/app/iggy3d/gameplay/WallRunState.hpp`

Verified at: `0e08703f`

## Owns

- Header-only product gameplay mirror packets for movement, jump, dash, reset, traversal, wall-run, target, outcome, tape playback, and physics movement planner proof.
- Field defaults that define no-action/no-proof states for receipts, tests, automation, HUDs, and frame presentation.

## Does Not Own

- Movement simulation, wall-run candidate evaluation, jump/dash/traversal algorithms, reset placement, target lookup, tape execution, physics movement planning, session ticks, or UI rendering.

## Reads

- `GameplayMovementInfo.hpp` reads `MovementTuning.hpp` defaults and movement enum descriptors.
- Other headers read no runtime data directly; they define packet shapes.

## Writes / Mutates

- No functions mutate state here.
- Controller action files, scripted/tape runners, transitions, automation, window input, and session launch paths mutate fields through `window.gameplay.*`.

## Calls Out To / Wires Out To

- No calls from these headers.
- Packets are aggregated by `GameplayStore.hpp` and projected by receipts, debug HUDs, frame presenter, automation result checks, and no-window tests.

## Called By / Entry Points

- `GameplayStore.hpp` includes all packet headers.
- Grep proof: `rg -n "ProductGameplayDash|ProductGameplayJump|ProductGameplayTarget|ProductGameplayTraversal|ProductWallRunState|PhysicsMovementPlannerState|TapeState|OutcomeState|ResetState|GameplayMovementInfo" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- These packets are app observability and control mirrors, not runtime simulation truth.
- String defaults such as `"not_requested"`, `"none"`, and disabled statuses are receipt-facing contracts.
- Movement tuning defaults in `GameplayMovementInfo` must match `MovementTuning.hpp` construction.
- Wall-run candidate state and active wall-run state remain separate.
- Tape proof fields describe scripted playback progress and failures; they do not own tape parsing/execution.
- Physics movement planner proof reports app usage of the planner path; runtime movement planner ownership stays under runtime/player and runtime/movement.

## Tests / Proof Commands

- `rg -n "gameplayJump|gameplayDash|gameplayTraversal|gameplayWallRun|gameplayTarget|gameplayOutcome|gameplayTape|physicsMovementPlanner" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_gameplay_tape_runner_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.
- `rg -n "GameplayRuntimeMovementFields|GameplaySceneStateFields|MovementDebugHud|FramePresenter|AutomationGameplay" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/*` and `src/runtime/player/*` unless planner/runtime contracts change.
- `src/app/iggy3d/gameplay/Controller*.{hpp,cpp}` unless writers change packet meanings.
- `src/app/iggy3d/debug/*Hud.*` and `src/app/iggy3d/receipt/*` unless projections change.

## Update When

- Any grouped packet fields, defaults, or ownership semantics change.
- A grouped packet becomes complex enough to deserve its own file spec.

## Do Not Update When

- Only action implementation, runtime physics/movement internals, HUD layout, or receipt ordering changes without changing packet shape or meaning.
