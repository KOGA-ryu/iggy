# File Spec

Files: `src/app/iggy3d/gameplay/ControllerGroundQueries.hpp`, `src/app/iggy3d/gameplay/ControllerGroundQueries.cpp`

Verified at: `b60dce77`

## Owns

- Product-side walkable-ground queries over `SpatialSurfaceSet`.
- Ground contact tolerance constant used by gameplay controller ledge/fall/jump paths.
- Highest walkable ground below a position, nearby-ground predicate, and lowest walkable floor lookup.

## Does Not Own

- Collision surface baking, runtime movement resolution, player transform mutation, authored room geometry, physics broadphase, or renderer floor drawing.

## Reads

- `SpatialSurfaceSet::surfaces()`, `CollisionSurfaceView` role, bounds, normal, plane point, and position/max-height inputs.

## Writes / Mutates

- Writes output references for ground/floor height when a query succeeds.
- Does not mutate surfaces, window state, session state, world transforms, or collision caches.

## Calls Out To / Wires Out To

- Uses collision surface view data from `runtime/collision/SpatialSurfaceSet.hpp`.
- Uses local X/Z footprint containment and plane-height math.

## Called By / Entry Points

- Ledge-fall command execution, jump landing, reset/fall recovery, and nearby-ground checks.
- Grep proof: `rg -n "findHighestWalkableGroundAtOrBelow|playerHasNearbyGround|findLowestWalkableFloorY" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Null surfaces return false.
- Only `CollisionSurfaceRole::Walkable` surfaces with usable Y normals can provide ground.
- Query ignores non-finite plane heights.
- Highest-ground query rejects candidates above `maxY` plus contact tolerance.
- Nearby-ground predicate requires vertical difference within contact tolerance.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "findHighestWalkableGroundAtOrBelow|playerHasNearbyGround|no_walkable_ground|grounded_ledge_fall" src/app/iggy3d tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/runtime/collision/*` unless surface view semantics change.
- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless ledge/fall command handling changes.
- `src/app/iggy3d/gameplay/ControllerJumpActions.*` and `ControllerResetFall.*` unless landing/recovery behavior changes.

## Update When

- Ground tolerance, walkable filtering, plane-height math, nearby-ground logic, or lowest-floor behavior changes.

## Do Not Update When

- Only collision bake internals, movement input, renderer floor drawing, or room authoring changes without changing product ground-query contracts.
