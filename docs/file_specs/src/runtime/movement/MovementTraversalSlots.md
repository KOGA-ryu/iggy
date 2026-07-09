# Movement Traversal Slots

File:

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversalSlots.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversalSlots.cpp`

Verified at: `69514d40`

## Owns

- Runtime traversal affordance and slot registries for vault, clamber, and wire-walk.
- Conversion from authored room traversal tags/static mesh roles/surfaces into traversal slots.
- Slot selection by kind, range, facing, ledge height, usable width, and deterministic tie-breaks.
- Traversal slot status/name helpers.

## Does Not Own

- Executing traversal or mutating world transforms.
- Collision validation for landing or clearance.
- Authoring traversal tags or room asset parsing.
- App traversal preview rendering.

## Reads

- `RoomAsset::staticMeshes` and `RoomAsset::spatialSurfaces`.
- Traversal tags, mesh roles, mesh bounds, surface roles, surface points, blocker flags, and world offset.
- Slot selection request actor position, forward vector, range, height, width, and facing thresholds.

## Writes / Mutates

- Builds new affordance/slot registry vectors.
- Does not mutate `RoomAsset` or world state.

## Calls Out To / Wires Out To

- Calls traversal tag parsing helpers from content assets.
- `MovementTraversal` consumes registries and selections for preview and execution.

## Called By / Entry Points

- `buildMovementTraversalAffordanceRegistry(...)`
- `buildMovementTraversalSlotRegistry(...)`
- `selectMovementTraversalSlot(...)`
- `selectedTraversalSlot(...)`
- `candidateTraversalSlot(...)`
- `movementTraversalSlotKindName(...)`
- `movementTraversalSlotSelectionStatusName(...)`

## Invariants

- Authored traversal tags take precedence over legacy name fallback affordances.
- Clamber slots require same-mesh walkable top and actor-blocking/front helper geometry.
- Selection keeps both selected slot and nearest candidate facts when available.
- Equal-range slot choices are tie-broken by stable slot id.
- Invalid positions, forward vectors, or thresholds return `slot_invalid_input`.

## Tests / Proof Commands

- `rg -n "movement_traversal_slots_tests|buildMovementTraversalSlotRegistry|selectMovementTraversalSlot" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `movement_traversal_slots_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversal.*`
- `/Users/kogaryu/iggy3d/src/content/assets/TraversalTag.*`
- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.hpp`

## Update When

- Traversal affordance discovery, slot packet fields, selection policy, or slot status/reason contracts change.

## Do Not Update When

- Only traversal execution, landing collision, or app preview drawing changes.
