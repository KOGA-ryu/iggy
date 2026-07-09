# File Spec

Files: `src/app/iggy3d/gameplay/ControllerResetFall.hpp`, `src/app/iggy3d/gameplay/ControllerResetFall.cpp`

Verified at: `f6abfbf4`

## Owns

- Product gameplay reset-to-spawn and unsupported-ground fall-start helpers.
- Reset-zone detection, fall-out reset detection, reset proof recording, and jump-state cleanup after reset.

## Does Not Own

- Room authoring, active room loading, runtime movement planner, collision bake ownership, player-slot policy, input routing, or receipt rendering.

## Reads

- Primary player entity and position through product player access helpers.
- Active room anchors through `activeRoom(window).room.anchors`.
- `SpatialSurfaceSet` walkable floor and nearby-ground queries.
- Movement tuning coyote time for unsupported-ground fall start.

## Writes / Mutates

- May mutate player position through `setProductPlayerPosition(...)`.
- Writes `gameplayReset` proof fields, clears or starts jump state, sets player-position-changed flag after reset.
- Does not mutate active room assets or collision surfaces.

## Calls Out To / Wires Out To

- `productPlayerActor(...)`, `productPlayerEntity(...)`, and `setProductPlayerPosition(...)`.
- `activeRoom(...)` and room anchor data.
- `findLowestWalkableFloorY(...)`, `findHighestWalkableGroundAtOrBelow(...)`, and `playerHasNearbyGround(...)`.
- `recordProductJumpPosition(...)` and `clearProductJumpTiming(...)`.

## Called By / Entry Points

- Jump action updates and gameplay command execution after movement.
- Grep proof: `rg -n "resetProductPlayerToSpawn|applyProductGameplayResetIfNeeded|beginProductFallIfUnsupported|gameplay_reset|reset_zone" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Reset requires an existing player entity and a spawn anchor.
- Reset zones match by anchor kind and local horizontal/vertical proximity.
- Fall-out reset uses lowest walkable floor minus the fixed reset threshold.
- Reset records source anchor id when present, otherwise `none`.
- Unsupported-ground fall starts only for an existing player, available collision surfaces, and no nearby ground.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplay_reset_zone|gameplay_reset_fall_out|gameplay_jump_falling|gameplayReset|reset_zone" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ProductRoomStore.*` unless active room anchor access changes.
- `src/app/iggy3d/gameplay/ControllerGroundQueries.*` unless floor/ground query semantics change.
- `src/app/iggy3d/gameplay/ControllerPlayerAccess.*` unless player transform mutation changes.

## Update When

- Reset trigger rules, spawn/source anchor proof, fall-out threshold, unsupported-ground fall start, or jump cleanup after reset changes.

## Do Not Update When

- Only room fixture contents, renderer/receipt formatting, or unrelated movement command logic changes without changing reset/fall contracts.
