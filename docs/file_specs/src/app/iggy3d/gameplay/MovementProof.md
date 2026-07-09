# File Spec

Files: `src/app/iggy3d/gameplay/MovementProof.hpp`, `src/app/iggy3d/gameplay/MovementProof.cpp`

Verified at: `dd59395f`

## Owns

- Product movement proof packet copied from `ProductAppWindowState`.
- Movement state names/HUD labels and wall-run reason HUD labels used by debug HUD and receipt surfaces.

## Does Not Own

- Movement simulation, movement result recording, wall-run evaluation, tuning descriptor definitions, HUD row formatting, receipt field emission, or command execution.

## Reads

- `ProductAppWindowState::gameplay.gameplayMovement`, `gameplayJump`, and `gameplayWallRun`.
- Movement state descriptors and wall-run status descriptors from `MovementTuning.hpp`.

## Writes / Mutates

- Returns `ProductMovementProofPacket`.
- Does not mutate window state, session state, runtime movement state, renderer frames, or receipts.

## Calls Out To / Wires Out To

- `productGameplayMovementStateName(...)`.
- `productGameplayMovementStateHudLabel(...)`.
- `findProductWallRunStatusDescriptor(...)`.

## Called By / Entry Points

- `buildProductMovementProofPacket(...)`.
- Called by movement debug HUD and receipt builder paths.
- Grep proof: `rg -n "buildProductMovementProofPacket|ProductMovementProofPacket" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Packet is a copy/projection of product state, not a source of gameplay truth.
- Movement state name and HUD label are derived from the enum descriptor table.
- Unknown wall-run reason labels fall back to the raw key.
- Vertical velocity is copied from jump velocity.
- Wall-run candidate and active proof remain separated in the packet.

## Tests / Proof Commands

- `rg -n "product_movement_debug_hud_tests|product_gameplay_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "ProductMovementProofPacket|buildProductMovementProofPacket|wallRunReasonHudLabel|stateHudLabel" tests/unit/product_movement_debug_hud_tests.cpp src/app/iggy3d/ReceiptBuilder.cpp src/app/iggy3d/debug/MovementDebugHud.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerMovementProof.*` unless source movement proof fields change.
- `src/app/iggy3d/gameplay/MovementTuning.hpp` unless descriptor labels change.
- `src/app/iggy3d/debug/MovementDebugHud.*` unless HUD presentation changes.

## Update When

- Movement proof packet fields, descriptor-derived labels, wall-run label fallback, or copied source fields change.

## Do Not Update When

- Only runtime movement internals, command execution flow, or HUD/receipt layout changes without changing packet contents.
