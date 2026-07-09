# File Spec

Files: `src/app/iggy3d/receipt/ActiveRoomFields.cpp`

Verified at: `0fe74f9c`

## Owns

- Receipt field emission for active room state, authored-room counts, room asset counts, active room collision state, and active room collision freshness proof.
- Table-driven mapping from active room stores to receipt keys.

## Does Not Own

- Active room construction.
- Active room collision bake or freshness calculation.
- Runtime session or gameplay state.
- Room editor or CreativeDocument room mutation.

## Reads

- `ProductAppWindowState` active room, active room collision, and active room collision freshness stores through `ProductRoomStore` accessors.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate active room, collision, freshness, or window state.

## Calls Out To / Wires Out To

- `activeRoom(window)`.
- `activeRoomCollision(window)`.
- `activeRoomCollisionFreshness(window)`.
- `appendReceiptField(...)`.

## Called By / Entry Points

- `appendProductStartupWorldBuildoutFields(...)` calls `appendProductActiveRoomFields(...)`.
- Focused proof: `rg -n "appendProductActiveRoomFields|active_room_loaded|active_room_collision_ready|active_room_collision_freshness" src/app tests`.

## Invariants

- Active room and active room collision receipts are read from their stores, not recomputed.
- Authored-room and room-asset counts remain separate facts.
- Freshness result remains diagnostic proof, not a trigger to rebake.
- This appender must not call collision refresh or room activation logic.

## Tests / Proof Commands

- `rg -n "active_room_loaded|active_room_collision_ready|active_room_collision_freshness" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_active_room_state_tests|product_active_room_collision_tests|product_ascii_authoring_smoke|product_ascii_map_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active room proof fields change.
- `src/app/iggy3d/gameplay/ActiveRoomCollision.*` unless collision proof fields change.
- `src/app/iggy3d/gameplay/ProductRoomStore.*` unless active room accessors change.

## Update When

- Active room receipt keys, active room/collision/freshness source fields, or active-room receipt grouping changes.

## Do Not Update When

- Only room construction, collision bake internals, or renderer projection changes without changing emitted receipt fields.
