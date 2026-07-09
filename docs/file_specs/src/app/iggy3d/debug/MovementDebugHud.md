# File Spec

Files: `src/app/iggy3d/debug/MovementDebugHud.hpp`, `src/app/iggy3d/debug/MovementDebugHud.cpp`

Verified at: `e4855124`

## Owns

- Movement debug HUD packet and line model.
- Conversion from `ProductMovementProofPacket` or `ProductAppWindowState` movement proof fields into status, blocked reason, movement state, wall-run facts, velocity/position facts, line rows, and feedback tones.
- Gameplay/developer-tools/debug-overlay visibility gating for movement HUD rows.

## Does Not Own

- Runtime movement simulation, collision/motor planning, or movement proof generation.
- Projection refresh orchestration, rendering, or receipt serialization.

## Reads

- Product window movement proof or caller-provided movement proof packet.
- Gameplay-active, developer-tools-enabled, and debug-overlay-enabled flags.

## Writes / Mutates

- Returns `MovementDebugHud`; no external state.

## Calls Out To / Wires Out To

- Uses `GameplayFeedback` tones.
- Output is consumed by receipt builder, projection refresh, debug HUD view, and Vulkan frame presenter.

## Called By / Entry Points

- `ReceiptBuilder.cpp`, `ProjectionRefresh.cpp`, `DebugHudView.cpp`, and `FramePresenter.cpp`.
- Grep proof: `rg -n "buildMovementDebugHud|MovementDebugHud|appendMovementDebugHudUi|drawMovementDebugHud" src tests cmake`.

## Invariants

- Inactive/developer-disabled/overlay-disabled states remain visible in status/reason fields.
- Blocked movement and wall-run facts keep separate labels/statuses.
- Builder overloads must stay equivalent for window-derived and proof-derived data.
- This file must not become a movement-system owner.

## Tests / Proof Commands

- `rg -n "product_movement_debug_hud_tests|product_vulkan_room_frame_tests" cmake tests`.
- `rg -n "buildMovementDebugHud" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/*` unless proof packet semantics change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless HUD copy/gating changes.
- `src/app/iggy3d/window/FramePresenter.*` unless HUD presentation changes.

## Update When

- Movement HUD fields, proof mapping, line formatting, tones, or visibility gating change.

## Do Not Update When

- Runtime movement behavior changes while the proof/HUD contract stays unchanged.
