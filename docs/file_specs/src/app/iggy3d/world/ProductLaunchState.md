# File Spec

Files: `src/app/iggy3d/world/ProductLaunchState.hpp`, `src/app/iggy3d/world/ProductLaunchState.cpp`

Verified at: `eebd1820`

## Owns

- Product gameplay launch cleanup after a failed launch/load/create path.
- Resetting active session, active room, active room collision, and gameplay active flags as one launch-state boundary.

## Does Not Own

- Frontend status text selection.
- Save/load failure reason calculation.
- Runtime session creation.
- Active room construction.

## Reads

- Optional active session and product window state.

## Writes / Mutates

- Clears `window.gameplay.gameplayActive`.
- Clears `window.gameplay.runtimeSessionCreated`.
- Resets active room and active room collision stores.
- Bumps active room revision.
- Resets the optional active session.

## Calls Out To / Wires Out To

- `activeRoom(window)`, `activeRoomCollision(window)`, and `bumpActiveRoomRevision(window)`.

## Called By / Entry Points

- Product session load failure paths.
- Product new-world initial-save failure path.
- Creative world launch/open failure paths.
- Focused proof: `rg -n "clearProductGameplayLaunchState" src/app tests/unit`.

## Invariants

- Failed launch cleanup must clear session and active room together.
- Active room revision must bump when the active room is cleared.
- This helper should stay small and not absorb launch failure policy.

## Tests / Proof Commands

- `rg -n "clearProductGameplayLaunchState|runtimeSessionCreated|gameplayActive" tests/unit src/app/iggy3d`.
- `rg -n "product_creative_world_launch_tests|product_window_input_frame_tests|product_starter_menu_action_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active room mutation contracts change.
- `src/app/iggy3d/world/ProductSessionLaunch.*` unless caller failure cleanup policy changes.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless creative launch cleanup changes.

## Update When

- Launch failure cleanup fields, active room/collision reset behavior, or session reset behavior changes.

## Do Not Update When

- Only launch status strings or frontend menu routing changes without changing cleanup semantics.
