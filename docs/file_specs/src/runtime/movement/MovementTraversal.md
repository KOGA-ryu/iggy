# Movement Traversal

File:

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversal.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversal.cpp`

Verified at: `69514d40`

## Owns

- Runtime traversal execution, intent, and preview request/result packets.
- Vault, clamber, and wire-walk execution over world entities and authored room slots.
- Traversal intent ordering and fallback policy for jump/interact triggers.
- Traversal preview status and HUD code mapping.

## Does Not Own

- Traversal slot discovery and selection internals.
- Room asset authoring/parsing.
- Player motor wire-walk ticking after a traversal enters wire-walk state.
- App input mapping or overlay drawing.

## Reads

- `WorldState` actor row and actor transform/active flag.
- `RoomAsset`, `SpatialSurfaceSet`, room world offset, traversal thresholds, and forward vector.
- Slot registries built by `MovementTraversalSlots`.

## Writes / Mutates

- Mutates actor transform through `WorldState::updateTransform` when traversal applies.
- Returns traversal, intent, and preview packets with slot, target, landing, rail, travel, status, reason, and HUD facts.

## Calls Out To / Wires Out To

- Calls `buildMovementTraversalSlotRegistry` and `selectMovementTraversalSlot`.
- Calls collision queries for landing ground, clearance, and blocker checks.
- Calls `computeMovementTravelFacts` for applied traversal observability.

## Called By / Entry Points

- `executeTraversalMechanic(...)`
- `executeTraversalIntent(...)`
- `previewTraversalCandidate(...)`
- Traversal status/name/HUD helper functions.
- `traversalApplied(...)`

## Invariants

- Invalid actor, inactive actor, bad forward vector, and bad thresholds are rejected before slot execution.
- Intent tries clamber, then vault, then wire-walk.
- No-candidate jump intent can allow fallback jump; consumed traversal failures do not.
- Successful execution updates world transform and returns travel facts.
- Preview never mutates world state.
- Landing and clearance checks use runtime collision surfaces, not render geometry.

## Tests / Proof Commands

- `rg -n "movement_traversal_tests|executeTraversalMechanic|executeTraversalIntent|previewTraversalCandidate" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `movement_traversal_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversalSlots.*`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionQuery.*`

## Update When

- Traversal request/result fields, intent ordering, world mutation policy, preview status/HUD codes, or landing/clearance validation changes.

## Do Not Update When

- Only slot discovery rules, room asset parsing, or app presentation changes.
