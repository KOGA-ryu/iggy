# Product Debug Overlay Position HUD Wiring

This note records the path used to wire the F3 position HUD into the real app
window. Use it when adding another small gameplay HUD/debug toggle so the route
does not have to be rediscovered.

## Current Path

1. SDL events are captured in `src/app/iggy3d/window/SdlWindow.cpp`.
   - `SdlWindowEventState` stores edge-style function-key facts such as
     `f1Pressed`, `f2Pressed`, and `f3Pressed`.
   - Function keys should be read from SDL events because platform keyboard
     polling can miss them.

2. Product input routing happens in `src/app/iggy3d/window/InputFrame.cpp`.
   - `productWindowFunctionKeyAction(...)` maps event facts to `InputAction`.
   - F3 maps to `InputAction::DevDebugOverlay`.
   - After consuming SDL event facts, also update `KeyboardInputState`
     edge guards with `recordProductWindowFunctionKeyKeyboardState(...)`.

3. Keyboard action naming lives in `src/app/input/KeyboardInput.*`.
   - Add neutral keys and parser names there when the action must also be
     reachable through non-SDL test paths.
   - F3 uses `NeutralInput::KeyF3` and `key.f3`.

4. The product window owns the debug overlay setting.
   - `ProductAppWindowState::settings.debugOverlayEnabled` is the live toggle.
   - `ProductGameplayProjectionRefresh` reads that setting when building
     product HUD models.
   - Receipts mirror the state with `settings_debug_overlay_enabled`.

5. The position HUD model is built in `src/app/iggy3d/debug/PositionHud.*`.
   - It is pure product/debug presentation state.
   - It should consume scene/player facts and settings; it must not own runtime
     movement or session truth.

6. SDL fallback drawing lives in `src/app/iggy3d/view/OpeningMenuView.cpp`.
   - `drawPositionHud(...)` handles the non-Vulkan/fallback gameplay panel path.

7. Vulkan gameplay UI drawing is bridged in
   `src/app/iggy3d/window/FramePresenter.*`.
   - `ProductVulkanGameplayFrame` owns the gameplay frame plus UI overlay data.
   - `buildProductVulkanGameplayFrame(...)` converts `PositionHud` lines into
     UI rect/text glyphs.
   - `refreshProductVulkanGameplayFrameInput(...)` passes the overlay through
     `FrameInput`.

8. Room-frame recording carries the overlay in the render path.
   - `FirstRoomFrameRecordInfo` includes UI overlay fields.
   - The room frame record path must receive the UI data, otherwise HUD models
     are correct but nothing appears in the Vulkan window.

## Gotcha

Do not let SDL event handling and held-key polling both toggle the same action.
The failure mode is a HUD that appears for one frame, then disappears:

- SDL `KEY_DOWN F3` toggles the overlay on.
- The next frame, `SDL_GetKeyboardState` still sees F3 held.
- If `KeyboardInputState::debugOverlayWasDown` was not marked consumed, polling
  treats it as a fresh press and toggles the overlay off.

The fix is to sync event-backed F-key presses into the corresponding keyboard
edge guard:

- F1/F2 set `KeyboardInputState::devToggleWasDown`.
- F3 sets `KeyboardInputState::debugOverlayWasDown`.

## Focused Verification

Run these checks after changing this path:

```sh
cmake --build build --target iggy3d_app product_window_input_frame_tests product_vulkan_room_frame_tests product_position_hud_tests product_window_renderer_lifecycle_tests
ctest --test-dir build --output-on-failure -R '^(product_window_input_frame_tests|product_vulkan_room_frame_tests|product_position_hud_tests|product_window_renderer_lifecycle_tests)$'
git diff --check
tools/check_branch_gate.py
```

Before committing, also run:

```sh
git diff --cached --check
tools/check_branch_gate.py --cached
```

After committing:

```sh
tools/check_branch_gate.py --diff HEAD~1..HEAD
```
