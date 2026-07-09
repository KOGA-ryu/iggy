# File Spec

Files: `src/app/iggy3d/debug/PositionHud.hpp`, `src/app/iggy3d/debug/PositionHud.cpp`

Verified at: `e4855124`

## Owns

- Position HUD packet, request, and line model.
- Conversion from scene projection/player pose and yaw/pitch inputs into world position, grid position, layer, facing, status, line rows, and visibility fields.
- Gameplay/room-editor/developer-tools/debug-overlay gating for position display.

## Does Not Own

- Scene projection generation.
- Player movement/camera simulation.
- Grid/room editing behavior.
- SDL/Vulkan drawing or receipt serialization.

## Reads

- Optional `SceneProjectionResult`.
- Gameplay-active, room-editing-ready, developer-tools-enabled, debug-overlay-enabled flags.
- Yaw and pitch degrees supplied by caller.

## Writes / Mutates

- Returns `PositionHud`; no external state.

## Calls Out To / Wires Out To

- Output is copied by projection refresh and consumed by debug HUD view and Vulkan frame presenter.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds/copies position HUD facts.
- `DebugHudView.cpp` and `FramePresenter.cpp` draw/append position HUD rows.
- Grep proof: `rg -n "buildPositionHud|PositionHud|appendPositionHudUi|drawPositionHud" src tests cmake`.

## Invariants

- Missing scene/player data returns explicit status/reason.
- Room editor and gameplay gates must not silently show stale player position.
- Grid coordinates and facing strings are derived presentation facts, not simulation state.
- This file does not mutate camera/player state.

## Tests / Proof Commands

- `rg -n "product_position_hud_tests|product_vulkan_room_frame_tests" cmake tests`.
- `rg -n "buildPositionHud" src tests`.

## Nearby Files Usually Not Touched

- `src/projection/scene/SceneProjection.*` unless scene projection payload changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless copy/gating changes.
- `src/app/iggy3d/window/FramePresenter.*` unless overlay presentation changes.

## Update When

- Position HUD request/fields, projection mapping, line formatting, facing/grid math, or gating changes.

## Do Not Update When

- Player movement changes without changing scene projection/HUD contract.
