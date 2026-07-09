# File Spec

Files: `src/app/iggy3d/window/Loop.hpp`, `src/app/iggy3d/window/Loop.cpp`

Verified at: `b38ddff1`

## Owns

- Product window loop orchestration for SDL-window mode and no-window fallback.
- Loop-local mutable save catalog copy returned with final window state.
- Per-frame ordering for SDL poll, title update, creative UI/wireframe frame build, input processing, gameplay projection, presentation, frame/hold/close termination, and renderer shutdown.

## Does Not Own

- Input routing internals, frame presenter layout, renderer resource implementation, gameplay projection logic, save operations, creative UI command handling, or runtime session simulation.

## Reads

- App options, world template, frontend state, active session, world setup draft, initial window state, settings, initial save catalog, and optional creative app state.
- SDL drawable/window event state when SDL is enabled.

## Writes / Mutates

- Mutates loop-local `ProductAppWindowState` and loop-local `ProductSaveBridgeResult`.
- Updates requested/created/drawable/window/present/startup/frame counters and renderer status fields.
- Returns final window and save catalog; does not mutate the caller's input catalog directly.

## Calls Out To / Wires Out To

- `createProductWindowRenderer(...)`, `shutdownProductWindowRenderer(...)`, and `finalizeProductWindowRendererStatus(...)`.
- `initializeProductWindowInputFrameState(...)`, `processProductWindowInputFrame(...)`, and `shutdownProductWindowInputFrameState(...)`.
- Creative UI window frame, creative wireframe frame, gameplay projection frame, and `presentProductWindowFrame(...)`.
- Mouse capture policy for no-window mode.

## Called By / Entry Points

- `runProductWindowLoop(...)`.
- Called by app kernel and targeted window/creative tests.
- Grep proof: `rg -n "runProductWindowLoop|ProductWindowLoopRequest|ProductWindowLoopResult" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- No-window mode records mouse capture policy and returns without creating SDL/renderer state.
- The returned save catalog is the loop-local catalog after in-window mutations.
- Window title follows the frontend surface that is actually drawn.
- Input is processed before gameplay projection and frame presentation.
- Creative UI input availability depends on renderer path, drawable state, drawable extent, gameplay active, and active session.

## Tests / Proof Commands

- `rg -n "product_creative_ui_window_frame_tests|product_window_renderer_lifecycle_tests|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "runProductWindowLoop|framesPresented|window_create_failed|sdl3_unavailable|creativeUiFirstFrame" tests/unit src/app/iggy3d`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/InputFrame.*` unless input frame ordering/context changes.
- `src/app/iggy3d/window/FramePresenter.*` unless presentation request/ordering changes.
- `src/app/iggy3d/window/RendererLifecycle.*` unless renderer creation/shutdown contracts change.

## Update When

- Loop request/result contract, save catalog return rule, frame ordering, no-window behavior, title gating, creative UI/wireframe build placement, renderer lifecycle, or termination behavior changes.

## Do Not Update When

- Only input internals, presentation layout, runtime gameplay logic, or individual save/menu operations change without altering loop orchestration.
