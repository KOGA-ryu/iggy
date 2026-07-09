# File Spec

Files: `src/app/iggy3d/gameplay/ControllerJumpActions.hpp`, `src/app/iggy3d/gameplay/ControllerJumpActions.cpp`

Verified at: `f288fbd8`

## Owns

- Product jump action submission and per-frame jump advancement.
- Jump arc state transitions for accepted, airborne, landed, buffered, coyote, wall-jump, traversal-consumed, missing-player, and mutation-failed paths.
- Integration of active room collision surfaces into jump landing, wall jump, clamber/traversal, reset/fall, and proof recording.

## Does Not Own

- Input binding, controller phase scheduling, collision surface construction, movement tuning definition, traversal kernel internals, or renderer/receipt formatting.
- Active room loading or collision freshness rebuild policy.

## Reads

- `Session`, `ProductAppWindowState`, gameplay movement tuning, jump/wall-run state, active room state, active room collision surfaces, player entity, camera yaw, and collision surface set.

## Writes / Mutates

- Mutates window gameplay jump/traversal/wall-run proof fields and player-position-changed flags.
- Mutates runtime player position through product player access helpers.
- Updates session state hash after accepted traversal movement.

## Calls Out To / Wires Out To

- `productActiveRoomCollisionSurfaces(...)`.
- Ground, wall, traversal, reset/fall, player access, target proof, and traversal proof helpers.
- Runtime movement traversal and `computeStateHash(...)`.
- Called from controller action phases.

## Called By / Entry Points

- `advanceProductJump(...)`.
- `submitProductJump(...)`.
- Grep proof: `rg -n "advanceProductJump|submitProductJump" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Traversal jump and wall jump get first chance to consume jump input before normal jump.
- Missing player or failed mutation must reject/stop jump explicitly.
- Collision-surface absence falls back only where the current jump path allows it; active room traversal requires loaded room and surfaces.
- Jump proof fields must explain accepted/rejected movement for no-window receipts.

## Tests / Proof Commands

- `rg -n "product_gameplay_controls_smoke|product_gameplay_controller_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplay_jump|wall_jump|clamber|active_room_collision" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless scheduling changes.
- `src/app/iggy3d/gameplay/ActiveRoomCollision.*` unless collision surface access changes.
- `src/runtime/movement/*` unless traversal kernel contracts change.

## Update When

- Jump submission/advance order, status/reason codes, traversal/wall-jump priority, collision surface usage, or proof fields change.

## Do Not Update When

- Only active room construction, collision freshness rebuild, input key mapping, or render HUD layout changes.
