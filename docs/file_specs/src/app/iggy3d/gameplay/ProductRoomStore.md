# File Spec

Files: `src/app/iggy3d/gameplay/ProductRoomStore.hpp`, `src/app/iggy3d/gameplay/ProductRoomStore.cpp`

Verified at: `f288fbd8`

## Owns

- Window-owned product room store aggregate for active room, active room revision, active room collision, and collision freshness result.
- Accessor functions that return mutable or const references into `ProductAppWindowState::room`.

## Does Not Own

- Active room construction, collision building, collision freshness policy, save/load, receipts, or gameplay behavior.
- Revision bump decisions; callers decide when active room replacement has occurred.

## Reads

- `ProductAppWindowState::room` and its `ProductRoomStore` fields.

## Writes / Mutates

- Accessors expose references that callers may mutate.
- This file itself only returns references and does not apply business logic.

## Calls Out To / Wires Out To

- Included by active room builders, launch flows, save bridge, receipt fields, input frame, projection refresh, creative bake refresh, tape runner, automation, and tests.

## Called By / Entry Points

- `activeRoom(...)`.
- `activeRoomRevision(...)`.
- `activeRoomCollision(...)`.
- `activeRoomCollisionFreshness(...)`.
- Grep proof: `rg -n "activeRoom\\(|activeRoomRevision|activeRoomCollision\\(|activeRoomCollisionFreshness" src tests/unit`.

## Invariants

- `activeRoomRevision` is window-owned and must not be overwritten by copying `ProductActiveRoomState`.
- Accessors must continue returning references to the same window store fields.
- Store aggregation must stay app/product owned; runtime modules must not include this file.

## Tests / Proof Commands

- `rg -n "product_active_room_state_tests|product_active_room_collision_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "activeRoomRevision|&iggy3d::activeRoom|&iggy3d::activeRoomCollision" tests/unit/product_active_room_state_tests.cpp tests/unit/product_active_room_collision_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.hpp` unless the room store field moves.
- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active room packet fields change.
- `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.*` unless freshness result storage changes.

## Update When

- Product room store fields, accessor names, reference ownership, or revision storage semantics change.

## Do Not Update When

- Only active room state contents, collision-building rules, or product receipts change.
