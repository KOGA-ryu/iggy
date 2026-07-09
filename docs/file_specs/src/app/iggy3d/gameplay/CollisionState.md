# File Spec

File: `src/app/iggy3d/gameplay/CollisionState.hpp`

Verified at: `0e08703f`

## Owns

- `ProductGameplayCollisionState`, the app-side proof packet for whether gameplay command execution used active-room collision surfaces and how many were available.

## Does Not Own

- Active-room collision building, collision freshness, spatial surface storage, runtime physics queries, debug collision overlay state, or renderer collision visualization.

## Reads

- No runtime data directly; this header defines the packet shape only.

## Writes / Mutates

- No functions mutate state here.
- `ControllerCommandExecution.*` writes `surfacesUsed` and `surfaceCount` from the collision surface pointer used for command execution.

## Calls Out To / Wires Out To

- No calls.
- Packet is wired through `GameplayStore::gameplayCollision`.

## Called By / Entry Points

- Included by `GameplayStore.hpp`.
- Written by `submitProductGameplayCommand(...)`.
- Read by `GameplaySceneStateFields.*` for receipts.
- Grep proof: `rg -n "ProductGameplayCollisionState|gameplayCollision|GameplaySceneStateFields|submitProductGameplayCommand" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- `surfacesUsed=false` and `surfaceCount=0` represent command execution without a collision surface span.
- Packet proves command-path surface usage, not global active-room collision freshness.
- Active-room collision ownership stays in `ActiveRoomCollision.*`, `ActiveRoomCollisionFreshnessStore.*`, and `ProductRoomStore.*`.

## Tests / Proof Commands

- `rg -n "gameplayCollision\\.surfacesUsed|gameplayCollision\\.surfaceCount" src/app/iggy3d tests/unit`.
- `rg -n "product_active_room_collision_tests|product_gameplay_controller_tests|GameplaySceneStateFields" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomCollision.*`.
- `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.*`.
- `src/runtime/physics/*`.

## Update When

- Gameplay command collision proof gains fields or changes meaning.
- Collision proof moves out of `GameplayStore`.

## Do Not Update When

- Active-room collision baking, runtime physics queries, or debug overlay behavior changes without changing this packet.
