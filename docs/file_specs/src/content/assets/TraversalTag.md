# File Spec

Files: `src/content/assets/TraversalTag.hpp`, `src/content/assets/TraversalTag.cpp`

Verified at: `33a36cc1`

## Owns

- Traversal tag enum catalog, stable string ids, parse/validate helpers, and tag category predicates.
- Structural, movement, and authoring-hint classification for traversal tags.

## Does Not Own

- Movement traversal slot building, room asset parsing, authored-room conversion, runtime movement execution, or editor UI policy.

## Reads

- Static traversal tag rows and tag arrays.

## Writes / Mutates

- No mutation; returns spans, ids, optional parsed tags, validation booleans, and classification booleans.

## Calls Out To / Wires Out To

- Used by room asset parser/exporter, editable-room bake, ASCII authored/room-asset conversion, movement traversal slots, creative room bake, and tests.

## Called By / Entry Points

- `allTraversalTags()`.
- `traversalTagId(...)`.
- `parseTraversalTag(...)`.
- `validTraversalTag(...)`.
- `isStructuralTraversalTag(...)`.
- `isMovementTraversalTag(...)`.
- `isAuthoringTraversalHintTag(...)`.
- Grep proof: `rg -n "TraversalTag|traversalTagId|parseTraversalTag|validTraversalTag|isMovementTraversalTag" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- `allTraversalTags()` order matches the catalog rows used for id lookup.
- String ids are stable content contracts.
- Unknown tag ids parse to empty optional and validate false.
- Structural tags are walkable/blocker/projectile/opening roles.
- Movement tags are clamber/vault/wire-walk.
- `ClamberCandidate` is an authoring hint, not a movement traversal tag.

## Tests / Proof Commands

- `rg -n "traversal_tag_catalog_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "allTraversalTags|parseTraversalTag|validTraversalTag|isStructuralTraversalTag|isMovementTraversalTag|isAuthoringTraversalHintTag" tests/unit/traversal_tag_catalog_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/MovementTraversalSlots.*` unless movement interpretation changes.
- `src/content/assets/RoomAsset.*` unless parser validation changes.
- `src/app/iggy3d/ascii_room/*` unless authored/exported tag usage changes.

## Update When

- Tags are added/removed/renamed, ids change, or category predicates change.

## Do Not Update When

- Only a producer or consumer changes how it uses existing stable tag ids.
