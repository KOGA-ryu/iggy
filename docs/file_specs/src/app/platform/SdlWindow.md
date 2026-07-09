# File Spec

Files: `src/app/platform/SdlWindow.hpp`, `src/app/platform/SdlWindow.cpp`

Verified at: `b50008a5`

## Owns

- RAII wrapper for SDL video subsystem lifetime and `SDL_Window` ownership.
- Window creation flags for resizable, high-DPI, and Vulkan-capable windows.
- Drawable/window extents, focus/minimize/restore/resize/quit event state, selected function-key state, and relative mouse mode result packets.

## Does Not Own

- Product input action mapping beyond raw event facts.
- Mouse capture policy decisions.
- Renderer lifecycle, swapchain lifecycle, Vulkan surface ownership, or SDL renderer drawing.
- Product frontend, gameplay, or creative state.

## Reads

- SDL window, keyboard, focus, resize, minimize, restore, and quit events.
- SDL window logical size and drawable pixel size.
- Caller-provided `SdlWindowCreateInfo`.

## Writes / Mutates

- Owns and mutates internal `SDL_Window*`, video initialization flag, and `SdlWindowEventState`.
- Sets SDL window title.
- Applies SDL relative mouse mode and returns `SdlMouseCaptureResult`.

## Calls Out To / Wires Out To

- SDL video/window/event APIs.
- `InputFrame.cpp` reads event state and calls relative mouse mode through this wrapper.
- `RendererLifecycle.cpp`, `FramePresenter.cpp`, and Vulkan smokes read drawable extents and native window handles.

## Called By / Entry Points

- `Loop.cpp` creates the app window, polls events, and reads drawable extents.
- Vulkan smoke tests create windows directly for backend proof.
- Focused proof: `rg -n "SdlWindow|SdlWindowCreateInfo|SdlWindowEventState|setRelativeMouseMode|pollEvents" src tests`.

## Invariants

- `isOpen()` requires a native window and no quit request.
- `isDrawable()` requires open, not minimized, and non-zero drawable extents.
- Move construction and move assignment transfer native ownership and clear the moved-from state.
- Relative mouse mode status and reason code must stay receipt-friendly strings.
- Event polling resets edge-triggered resize/restore/function-key facts before draining SDL events.

## Tests / Proof Commands

- `rg -n "SdlWindowEventState|productWindowFunctionKeyAction|setRelativeMouseMode" tests/unit/product_window_input_frame_tests.cpp src/app`.
- `rg -n "vulkan_.*_smoke|SdlWindowCreateInfo" cmake/iggy3d_tests.cmake tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/InputFrame.*` unless event state or mouse capture result semantics change.
- `src/app/iggy3d/window/MouseCapturePolicy.*` unless capture decision policy changes.
- `src/app/platform/SdlVulkanSurface.*` unless Vulkan native-window handoff changes.
- `src/render/*` unless backend lifecycle semantics change.

## Update When

- SDL window lifetime, event facts, drawable extent policy, relative mouse mode status, or native-window exposure changes.

## Do Not Update When

- Only product action routing, frontend behavior, or backend renderer resource logic changes while the SDL window contract stays stable.
