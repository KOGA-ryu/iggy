# File Spec

Files: `src/app/iggy3d/world/MovementTestLab.hpp`, `src/app/iggy3d/world/MovementTestLab.cpp`

Verified at: `10135e4e`

## Owns

- Product movement test lab room-id predicate.
- Descriptor table for authored movement lab objects.
- Conversion from ASCII room grid plus lab config to authored room object records.
- Movement lab object traversal/gameplay tag assignment.

## Does Not Own

- ASCII room parsing or grid construction.
- Built-in dungeon catalog rows.
- Runtime movement implementation.
- Creative/editor placement tools.

## Reads

- `AsciiRoomGrid`, tile size, center-on-origin flag, story index, and enabled flag.
- Traversal tag ids for durable tag vocabulary.

## Writes / Mutates

- Returns authored room object records.
- Does not mutate the input grid or app/window/runtime state.

## Calls Out To / Wires Out To

- `asciiRoomCellCenter(...)` for centered object placement.
- `traversalTagId(...)` for durable traversal tags.

## Called By / Entry Points

- ASCII room authoring injects movement lab objects when the built-in movement test lab room id is selected.
- Built-in dungeon conversion checks movement lab room id before enabling injection.
- Focused proof: `rg -n "productMovementTestLabRoomId|buildProductMovementTestLabObjects|movement_test_lab" src/app tests/unit`.

## Invariants

- Object generation is disabled unless config `enabled` is true and the grid is usable.
- Object ids are stable and prefixed with movement lab lane/object identity.
- Blocking flags come from the object-kind descriptor.
- Ledge objects carry durable clamber traversal tag.
- This file is authored test-lab content generation, not movement physics truth.

## Tests / Proof Commands

- `rg -n "product_builtin_dungeon_tests|product_ascii_room_authoring_tests|ascii_room_to_authored_room_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "movement_test_lab_v0_1|buildProductMovementTestLabObjects" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/BuiltinDungeon.*` unless lab room id/catalog wiring changes.
- `src/app/iggy3d/ascii_room/Authoring.*` unless injection point changes.
- `src/runtime/movement/*` unless runtime movement behavior changes.

## Update When

- Lab object descriptors, generated authored object fields, room-id gating, or traversal tag contracts change.

## Do Not Update When

- Only runtime movement tuning, renderer visualization, or unrelated built-in dungeon text changes.
