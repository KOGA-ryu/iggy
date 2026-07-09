# File Spec

Files: `src/app/iggy3d/gameplay/ControllerWallQueries.hpp`, `src/app/iggy3d/gameplay/ControllerWallQueries.cpp`

Verified at: `b60dce77`

## Owns

- Product-side wall-jump and wall-run surface queries over `SpatialSurfaceSet`.
- Wall-run side naming, wall-run proof normal normalization, movement-along-wall proof, and tangent direction derivation.

## Does Not Own

- Wall-run state machine decisions, jump command execution, collision surface baking, runtime physics, player transform mutation, or HUD/receipt formatting.

## Reads

- `CollisionSurfaceView` blocker/opening/role/bounds/normal/traversal-tag fields.
- `ProductGameplayMovementTuning` wall probe and wall-run normal limits.
- `ProductAppWindowState` movement debug, wall-run normal proof, and camera yaw.

## Writes / Mutates

- Writes output references for selected wall normal, distance comparison, and tangent direction.
- Does not mutate window state, surfaces, session state, or runtime world transforms.

## Calls Out To / Wires Out To

- Uses `SpatialSurfaceSet::surfaces()`, `Vec3` math helpers, `center(...)`, and `isValid(...)`.
- Calls `productManualFirstPersonDirection(...)` to convert move input to camera-relative desired direction.

## Called By / Entry Points

- Jump wall-jump path, wall-run evaluation, and move wall-run direction path.
- Grep proof: `rg -n "findWallJumpSurface|findWallRunSurface|wallRunSideName|productMovementDebugAlongWall|wallRunTangentDirection" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Wall-jump surfaces must be actor-blocking blockers, not openings, with `wall_jump` traversal tag.
- Wall-run surfaces must be actor-blocking blockers, not openings, with valid bounds and acceptable wall-normal Y.
- Best surface is the nearest candidate within wall probe distance.
- Wall tangent direction is valid only when move input sufficiently aligns with the wall tangent.
- Stored wall-run proof normals are normalized before use and invalid normals fail closed.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_movement_debug_hud_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "wall_run|wall jump|findWallRunSurface|wallRunTangentDirection|productMovementDebugAlongWall" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_movement_debug_hud_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.*` unless wall-run state decisions change.
- `src/app/iggy3d/gameplay/ControllerJumpActions.*` unless wall-jump command behavior changes.
- `src/runtime/collision/*` unless surface role/tag semantics change.

## Update When

- Wall surface eligibility, traversal tags, probe distance use, side naming, along-wall threshold, normal proof, or tangent-direction semantics change.

## Do Not Update When

- Only wall-run HUD labels, tuning default values, input bindings, or runtime collision bake internals change without changing query contracts.
