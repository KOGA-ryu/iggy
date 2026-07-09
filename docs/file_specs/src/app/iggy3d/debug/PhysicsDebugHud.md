# File Spec

Files: `src/app/iggy3d/debug/PhysicsDebugHud.hpp`, `src/app/iggy3d/debug/PhysicsDebugHud.cpp`

Verified at: `e4855124`

## Owns

- Physics debug HUD packet and line model: `PhysicsDebugHud` and `PhysicsDebugHudLine`.
- Conversion from `DebugProjectionResult` physics facts into visible HUD text, tones, status, warning flag, line count, and gating reason.
- Gameplay/developer-tools/debug-overlay visibility gating for physics HUD lines.

## Does Not Own

- Runtime physics simulation, collision queries, or debug fact generation.
- Projection construction.
- SDL/Vulkan drawing, receipt serialization, or window input.

## Reads

- Optional `DebugProjectionResult`.
- Gameplay-active, developer-tools-enabled, and debug-overlay-enabled flags.
- Physics debug projection lines/status/warning facts.

## Writes / Mutates

- Returns `PhysicsDebugHud`; no external state.

## Calls Out To / Wires Out To

- Uses product feedback tones for line severity.
- Output is consumed by projection refresh, debug HUD view, Vulkan frame presenter, and receipt fields.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds/copies physics HUD facts into window state.
- `DebugHudView.cpp` and `FramePresenter.cpp` draw/append the HUD.
- Grep proof: `rg -n "buildPhysicsDebugHud|PhysicsDebugHud|appendPhysicsDebugHudUi|drawPhysicsDebugHud" src tests cmake`.

## Invariants

- Missing projection or inactive gates return explicit status/reason.
- Visible lines inherit HUD visibility.
- Warning tone/status facts stay observable for tests and receipts.
- This file remains presentation-packet assembly, not physics ownership.

## Tests / Proof Commands

- `rg -n "product_physics_debug_hud_tests|product_vulkan_room_frame_tests" cmake tests`.
- `rg -n "buildPhysicsDebugHud" src tests`.

## Nearby Files Usually Not Touched

- `src/projection/debug/DebugProjection.*` unless source debug facts change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless HUD copy/gating changes.
- `src/app/iggy3d/window/FramePresenter.*` unless HUD rendering changes.

## Update When

- Physics HUD fields, line/tone/status construction, or visibility gating changes.

## Do Not Update When

- Runtime physics math changes without changing projected HUD facts.
