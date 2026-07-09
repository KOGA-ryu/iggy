# File Spec

Files: `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`, `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`

Verified at: `791c40db`

## Owns

- Product creative baked active-room refresh execution.
- `refreshProductCreativeBakedActiveRoom(...)` admission, bake invocation, active room installation, collision freshness refresh, and result mirroring.
- No-renderable-object clear path when `clearOnNoRenderable` is requested.
- Default activation hook selection for creative reasoning graph activation.

## Does Not Own

- Creative document mutation or object descriptor policy.
- Low-level document-to-room bake rules.
- Runtime collision query kernels.
- Receipt field serialization.
- Product world launch/open command routing.

## Reads

- `ProductCreativeBakedActiveRoomRefreshRequest`.
- Active runtime session optional, `ProductAppWindowState`, and `creative::CreativeAppState`.
- Current `CreativeDocument` id, revision, validity, and object count.
- Existing active room and active room collision stores.

## Writes / Mutates

- Writes `ProductCreativeBakedActiveRoomRefreshResult` with bake receipt, counts, status, reason, timing, active-room, and collision mirrors.
- Mutates product active room state after an accepted bake or accepted no-renderable clear.
- Bumps active room revision and refreshes active room collision freshness.
- Records baked-room freshness for the creative document revision.

## Calls Out To / Wires Out To

- Calls `creative::buildRoomAssetFromCreativeDocument(...)`.
- Calls `buildProductActiveRoomFromPackageRoom(...)`, `activeRoom(...)`, `bumpActiveRoomRevision(...)`, and `ensureActiveRoomCollisionFresh(...)`.
- Calls `activateCreativeReasoningGraph(...)` by default through the activation hook.
- Called from creative world launch/open operations and creative input-frame mutations that need a refreshed active room.

## Called By / Entry Points

- `refreshProductCreativeBakedActiveRoom(...)`.
- Grep proof: `rg -n "refreshProductCreativeBakedActiveRoom|ProductCreativeBakedActiveRoomRefresh|bakedActiveRoomRefresh" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Refresh requires Creative interaction mode, an active session, and a valid creative document.
- A rejected bake must not install stale active-room state unless the explicit no-renderable clear path accepts it.
- Accepted installs must refresh collision freshness against the same active session.
- Result mirrors are proof fields; they must stay derived from bake and active-room state, not independent truth.
- The default reasoning activation hook is replaceable by request for tests and specialized callers.

## Tests / Proof Commands

- `product_creative_world_launch_tests`.
- `product_creative_no_window_bake_scenario_tests`.
- `rg -n "product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBake.*` unless bake packet semantics change.
- `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.*` unless collision freshness rules change.
- `src/app/iggy3d/CreativeReasoningActivation.*` unless reasoning activation ownership changes.
- `src/app/iggy3d/receipt/CreativeReceiptRecording.*` unless receipt emission changes.

## Update When

- Creative bake admission, active room install/clear behavior, collision refresh, result proof fields, or activation hook wiring changes.

## Do Not Update When

- Only document editing, UI command routing, or renderer presentation changes without changing baked active-room refresh behavior.
