# File Spec

Files: `src/app/iggy3d/ProductCreativeBakedRoomRefresh.hpp`

Verified at: `3b586769`

## Owns

- Product-facing request and result packets for refreshing a creative document into the active baked room.
- Activation hook type used after a baked room is installed into a session.
- Receipt-facing baked-room refresh facts: accepted status, reason, document id, bake timing, source counts, active-room load state, and collision readiness.

## Does Not Own

- Creative document baking implementation.
- Active room installation implementation.
- Creative world launch/open orchestration.
- Receipt field emission.
- Runtime session, collision, or rendering behavior.

## Reads

- Type declarations from creative room bake adapter surfaces.
- `Session`, `RoomAsset`, and `creative::CreativeDocument` through the activation hook signature.

## Writes / Mutates

- No functions in this header mutate state.
- Producers fill `ProductCreativeBakedActiveRoomRefreshResult` during creative baked room refresh.

## Calls Out To / Wires Out To

- `creative/BakedActiveRoomRefresh.*` consumes the request and produces the result.
- Creative world operations mirror refresh results into launch/open result packets.
- Receipt recording reads refresh results for creative proof fields.

## Called By / Entry Points

- `ProductCreativeBakedActiveRoomRefreshRequest`.
- `ProductCreativeBakedActiveRoomRefreshResult`.
- Focused proof: `rg -n "ProductCreativeBakedActiveRoomRefresh|refreshProductCreativeBakedActiveRoom|bakedActiveRoomRefresh" src/app tests`.

## Invariants

- Default request targets the creative baked room id and creative document bake subset.
- Default result is a not-requested, not-accepted packet.
- Counts in this packet are observability/proof mirrors; creative document and active room remain the source owners.
- Activation hooks must not move reusable session logic into this packet header.

## Tests / Proof Commands

- `rg -n "product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "ProductCreativeBakedActiveRoomRefreshResult|bakeReceipt|collisionReady" tests/unit/product_creative_world_launch_tests.cpp tests/unit/product_creative_no_window_bake_scenario_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/BakedActiveRoomRefresh.*` unless refresh execution changes.
- `src/app/iggy3d/creative/adapters/RoomBake.*` unless bake receipt fields change.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless launch/open mirroring changes.
- `src/app/iggy3d/receipt/CreativeReceiptRecording.*` unless receipt fields change.

## Update When

- Baked active-room refresh request fields, result fields, defaults, activation hook contract, or receipt-facing semantics change.

## Do Not Update When

- Only creative bake internals, active room runtime behavior, or receipt formatting changes without changing this packet contract.
