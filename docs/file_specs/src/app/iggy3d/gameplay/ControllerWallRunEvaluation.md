# File Spec

Files: `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp`, `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp`

Verified at: `f6abfbf4`

## Owns

- Product wall-run candidate and active-state evaluation packets.
- Publishing wall-run candidate/active proof into `ProductAppWindowState`.
- Wall-run start, continuation, expiration, input-stop, jump-exit, landed, missing-surface, missing-player, low-speed, and input-away status decisions.

## Does Not Own

- Wall surface geometry queries, movement delta execution, jump command execution, movement tuning defaults, HUD/receipt formatting, or runtime collision baking.

## Reads

- `ProductAppWindowState` jump active state, movement speed/debug/tuning, current wall-run active proof, and camera yaw.
- Optional player position and optional `SpatialSurfaceSet`.
- Move axes and jump-pressed input facts supplied by the caller.

## Writes / Mutates

- `publishProductWallRunEvaluation(...)` mutates wall-run candidate/active proof fields on the product window state.
- `clearProductWallRunActiveProof(...)` clears active wall-run proof with a reason.
- `evaluateProductWallRun(...)` itself returns packets without mutating window state.

## Calls Out To / Wires Out To

- `findWallRunSurface(...)`, `productMovementDebugAlongWall(...)`, `wallRunSideName(...)`, and `wallRunTangentDirectionFromNormal(...)`.
- Reads `SpatialSurfaceSet` through wall query helpers.

## Called By / Entry Points

- Product gameplay action phase handling evaluates and publishes wall-run state each frame.
- Move and jump paths clear or exit active wall-run proof.
- Grep proof: `rg -n "evaluateProductWallRun|publishProductWallRunEvaluation|clearProductWallRunActiveProof|wall_run" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Candidate evaluation requires airborne jump state, minimum horizontal speed, collision surfaces, player position, wall contact, and movement along the wall.
- Active wall-run requires candidate availability, move input, tangent-aligned input, and remaining duration.
- Jump input forces wall-run exit after candidate/active evaluation.
- Active duration, gravity multiplier, and speed multiplier are clamped from tuning.
- Evaluation packets separate pure evaluation from publishing into window state.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_movement_debug_hud_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "wall_run_candidate|wall_run_active|wall_run_started|wall_run_expired|wall_run_input_stopped|wall_run_exit_jump|wall_running" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_movement_debug_hud_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerWallQueries.*` unless surface/tangent query semantics change.
- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless frame phase ordering changes.
- `src/app/iggy3d/gameplay/MovementTuning.hpp` unless wall-run tuning fields change.

## Update When

- Wall-run candidate rules, active-state rules, status/reason strings, publish fields, tuning clamps, or jump-exit behavior changes.

## Do Not Update When

- Only wall-run HUD labels, receipt field ordering, collision bake internals, or movement execution changes without altering wall-run evaluation contracts.
