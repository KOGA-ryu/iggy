# File Spec

Files: `src/app/iggy3d/view/DebugHudView.hpp`, `src/app/iggy3d/view/DebugHudView.cpp`

Verified at: `3dab1aa4`

## Owns

- SDL fallback drawing functions for gameplay feedback, interaction mode, movement debug, NPC debug, physics debug, room editor HUD, position HUD, and camera heading.
- Immediate-mode panel/text drawing for HUD packets.

## Does Not Own

- HUD model construction, Vulkan overlay conversion, runtime/projection truth, or input state.
- App window state storage.

## Reads

- HUD packet structs and `SDL_Renderer`.
- Visibility flags, lines, tones, labels, and simple value strings.

## Writes / Mutates

- SDL renderer draw calls only.
- No product/window/runtime state mutation.

## Calls Out To / Wires Out To

- `SdlDraw` helpers for rectangles/text and SDL line rendering.
- Tone-to-color mapping local to this view.

## Called By / Entry Points

- `OpeningMenuView.cpp` calls the draw functions for SDL presentation.
- Grep proof: `rg -n "drawMovementDebugHud|drawNpcBehaviorDebugHud|drawPhysicsDebugHud|drawPositionHud|drawGameplayFeedback|drawRoomEditorHud" src/app/iggy3d tests/unit cmake CMakeLists.txt`.

## Invariants

- Null or invisible HUD pointers are no-op.
- Draw caps prevent overly large NPC/physics/room-editor/position panels.
- This is SDL-only behind `IGGY3D_HAS_SDL3`.
- Vulkan HUD drawing lives in `FramePresenter`, not here.

## Tests / Proof Commands

- `rg -n "OpeningMenuView|DebugHudView|product_vulkan_room_frame_tests" src/app/iggy3d tests/unit cmake`.

## Nearby Files Usually Not Touched

- HUD builders unless packet shape changes.
- `FramePresenter.*` unless Vulkan HUD layout must match.
- Runtime/projection files unless input packets change.

## Update When

- SDL HUD draw functions, packet dependencies, draw caps, or panel layout contracts change.

## Do Not Update When

- Vulkan-only HUD layout changes.
