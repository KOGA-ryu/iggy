# File Spec

Files: `src/app/iggy3d/gameplay/ControllerTraversalProof.hpp`, `src/app/iggy3d/gameplay/ControllerTraversalProof.cpp`

Verified at: `f6abfbf4`

## Owns

- Product gameplay traversal proof mirrors.
- Generic traversal intent result copying and wall-jump-specific traversal proof construction.

## Does Not Own

- Traversal intent search, movement traversal mechanics, jump execution, collision surface query selection, HUD rendering, or receipt serialization.

## Reads

- `TraversalIntentResult` status, reason, mechanic, selected traversal, and start/final positions.
- `CollisionSurfaceView` id and runtime owner stable name for wall-jump proof.
- `ProductAppWindowState::gameplay.gameplayTraversal` as the write target.

## Writes / Mutates

- Clears or fills traversal requested/consumed/accepted/fallback/status/reason/mechanic/id/position proof fields.
- Does not mutate runtime traversal descriptors, collision surfaces, session state, or player transforms.

## Calls Out To / Wires Out To

- `traversalIntentStatusName(...)` and `traversalMechanicName(...)`.
- Uses collision surface identity only for wall-jump proof naming.

## Called By / Entry Points

- Jump action traversal intent path and wall-jump fallback path.
- Grep proof: `rg -n "clearProductTraversalProof|recordProductTraversalProof|recordProductWallJumpTraversalProof|gameplayTraversal|traversal_intent" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Clear state returns all traversal proof fields to `not_requested` or `none`.
- Null reason code maps to `unknown`.
- Traversal mechanic is `none` unless traversal was attempted.
- Empty traversal ids map to `none`.
- Wall-jump proof uses surface id when available and runtime owner stable name as target id when available.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplayTraversal|traversal_intent_applied|wall_jump|clamber" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/MovementTraversal.*` unless traversal result contracts change.
- `src/app/iggy3d/gameplay/ControllerJumpActions.*` unless traversal flow changes.
- `src/app/iggy3d/gameplay/ControllerWallQueries.*` unless wall-jump surface identity changes.

## Update When

- Traversal proof fields, default strings, wall-jump id mapping, or traversal result copying changes.

## Do Not Update When

- Only traversal search internals, movement execution, renderer layout, or receipt ordering changes without changing product traversal proof semantics.
