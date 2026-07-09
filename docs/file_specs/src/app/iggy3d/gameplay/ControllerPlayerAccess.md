# File Spec

Files: `src/app/iggy3d/gameplay/ControllerPlayerAccess.hpp`, `src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp`

Verified at: `b60dce77`

## Owns

- Product gameplay helper access to the primary player actor and entity.
- Controlled product-side player transform mutation helper that refreshes session state hash.

## Does Not Own

- Player slot assignment policy, command admission, runtime movement planning, world storage, state hash algorithm, save serialization, or renderer/player projection.

## Reads

- `Session::state().players.actorForSlot(0)`.
- `WorldState::findById(...)` and current entity transform.

## Writes / Mutates

- `setProductPlayerPosition(...)` mutates session-owned world transform through `Session::mutableStateForOwnedSystems()`.
- Recomputes `SessionState::currentStateHash` after successful transform mutation.
- Does not append commands, tick session, mutate inventory/objectives, or write receipts.

## Calls Out To / Wires Out To

- `WorldState::updateTransform(...)`.
- `computeStateHash(...)` after successful mutation.

## Called By / Entry Points

- Move, dash, jump, action phase, command execution, and reset/fall gameplay controller paths.
- Grep proof: `rg -n "productPlayerActor|productPlayerEntity|setProductPlayerPosition" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Product gameplay uses player slot zero for the primary actor.
- Missing player actor/entity returns null or false; callers own user-facing status.
- Transform mutation must go through world update API, not direct entity mutation.
- Successful position mutation must refresh `currentStateHash`.
- Failed world update must not claim success.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "productPlayerEntity|setProductPlayerPosition|missing_player|playerPositionChanged" src/app/iggy3d tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.*` unless mutable-state API changes.
- `src/runtime/replay/StateHash.*` unless hash boundary changes.
- `src/runtime/world/WorldState.*` unless transform mutation semantics change.

## Update When

- Primary player selection, player entity lookup, transform mutation path, failure behavior, or hash refresh responsibility changes.

## Do Not Update When

- Only gameplay command construction, movement math, rendering, receipts, or tests around callers change without changing player-access semantics.
