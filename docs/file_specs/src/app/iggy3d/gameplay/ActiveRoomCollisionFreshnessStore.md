# File Spec

Files: `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp`, `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`

Verified at: `a65a2f58`

## Owns

- Freshness guard for window-owned active room collision state.
- Provenance comparison between active room revision, session hash, and collision bake stamps.
- Choice between session-aware and sessionless active room collision rebuild.
- `ProductActiveRoomCollisionFreshnessResult` proof fields and rebake/skip reason codes.

## Does Not Own

- Active room geometry mutation, session ticking, command admission, collision surface construction policy, movement decisions, or persistence.
- Door activation semantics beyond using the current session hash as a coarse freshness input.

## Reads

- `ProductAppWindowState`, `activeRoomRevision(window)`, existing `activeRoomCollision(window)` stamps, optional `Session`, and `SessionState::currentStateHash`.

## Writes / Mutates

- Replaces `activeRoomCollision(window)` when room revision or session hash is stale.
- Writes `bakedFromRoomRevision` and `bakedFromSessionHash` stamps on rebuilt collision state.
- Returns freshness proof result.

## Calls Out To / Wires Out To

- `buildProductActiveRoomCollision(...)` with or without session state.
- `activeRoom(...)`, `activeRoomRevision(...)`, and `activeRoomCollision(...)` from `ProductRoomStore.*`.
- Called by window input, launch flows, automation, creative baked-room refresh, tape runner, and tests.

## Called By / Entry Points

- `ensureActiveRoomCollisionFresh(...)`.
- Grep proof: `rg -n "ensureActiveRoomCollisionFresh" src tests/unit`.

## Invariants

- Fresh collision state is skipped when both room revision and session hash match.
- `nullptr` session is valid and rebakes with session hash zero.
- Rebuilds must stamp the collision state with the observed room revision and session hash.
- Session hash is conservative: unrelated hashed session changes may rebake rather than risk stale runtime-owned surfaces.

## Tests / Proof Commands

- `rg -n "product_active_room_collision_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "ensureActiveRoomCollisionFresh|rebaked_room|rebaked_session|skipped_fresh" tests/unit/product_active_room_collision_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomCollision.*` unless collision build semantics change.
- `src/app/iggy3d/gameplay/ProductRoomStore.*` unless store accessors/stamps move.
- `src/runtime/session/*` unless state hash ownership changes.

## Update When

- Freshness predicates, bake stamps, rebake reason codes, sessionless behavior, or callers' freshness guarantees change.

## Do Not Update When

- Only active room packet fields, movement behavior, or render receipts change without freshness contract changes.
