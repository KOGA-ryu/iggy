# File Spec

Files: `src/app/iggy3d/window/InputFrame.hpp`, `src/app/iggy3d/window/InputFrame.cpp`

Verified at: `b38ddff1`

## Owns

- Per-frame product window input orchestration.
- Menu, controller, keyboard, mouse, creative UI, creative viewport pick, room editor, gameplay action, movement tuning, top-level toggle, and mouse-capture update routing.
- Headless/test click and pointer-lifecycle override seams.

## Does Not Own

- SDL event polling, frame presentation, renderer lifecycle, runtime gameplay kernels, save/load execution semantics, creative facade internals, or frontend route business rules.

## Reads

- `FrontendState`, save bridge/catalog state, app options, settings, active session, world setup draft, product window state, SDL window/event state, creative app state, creative UI draw list, and input frame state.
- Keyboard, mouse, gamepad, controller chord/routing, frontend blockers, active surface, interaction mode, room editor state, active room collision freshness, and creative document revision.

## Writes / Mutates

- Mutates frontend/menu state, save catalog copy through menu context, settings tab, active session through gameplay/creative calls, world setup draft, product window state, input-frame state, close-request flag, room editor preview/action proof, movement tuning proof, creative input proof, mouse-capture proof, and gameplay command/proof state through delegated calls.

## Calls Out To / Wires Out To

- Menu route/input: `routeProductOpeningMenuInput(...)`, `applyProductSystemPauseMenuAction(...)`, `applyProductGameplayMapMakerToggleAction(...)`.
- Gameplay: `applyProductGameplayActions(...)`, active-room collision freshness, camera actions, controller action routing.
- Creative: creative UI input/command, viewport pick, pointer lifecycle, creative tool dispatch, navigate fly, baked room refresh, undo snapshots.
- Platform/input: keyboard/mouse/gamepad polling and `SdlWindow::setRelativeMouseMode(...)`.

## Called By / Entry Points

- `processProductWindowInputFrame(...)`.
- Helper entry points include mouse hit dispatch, function-key routing, movement tuning input, controller sample processing, room editor mouse pick preview, and input-action application.
- Grep proof: `rg -n "processProductWindowInputFrame|applyProductWindowInputActions|dispatchProductOpeningMenuMouseHit|processProductControllerActionSample" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Menu/top-level toggles are processed before gameplay actions.
- Frontend-owned mouse clicks suppress creative UI/viewport downstream click handling.
- Creative document gameplay suppresses legacy starter hit bands unless frontend owns the mouse.
- Creative document input is split into UI command, viewport pick, pointer lifecycle, tool dispatch, revision/undo, and baked-room refresh phases.
- Mouse capture is recalculated after input handling from the resolved active surface and creative document/navigate state.

## Tests / Proof Commands

- `rg -n "product_window_input_frame_tests|product_creative_ui_input_frame_tests|product_creative_viewport_pick_frame_tests|product_creative_world_launch_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "processProductWindowInputFrame|dispatchProductWindowTopLevelToggleAction|applyProductWindowMovementTuningInput|productWindowFunctionKeyAction|normalizeProductWindowMenuClick" tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/Loop.*` unless frame ordering changes.
- `src/app/iggy3d/window/MouseCapturePolicy.*` unless capture decision rules change.
- `src/app/iggy3d/menu/*` unless frontend action semantics change.
- `src/app/iggy3d/creative/**` unless creative document input contracts change.

## Update When

- Input phase ordering, click ownership, creative UI/viewport routing, movement tuning routing, top-level toggle routing, controller routing, gameplay action handoff, or mouse-capture update wiring changes.

## Do Not Update When

- Only renderer drawing, runtime movement/combat internals, save catalog data model, or individual menu action handlers change without changing window input orchestration.
