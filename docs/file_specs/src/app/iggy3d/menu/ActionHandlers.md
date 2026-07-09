# File Spec

Files: `src/app/iggy3d/menu/ActionHandlers.hpp`, `src/app/iggy3d/menu/ActionHandlers.cpp`

Verified at: `6c79462b`

## Owns

- Stateful product menu command execution for starter, pause, load/save, new-world, settings, dev tools, delete confirm, system pause, and gameplay map-maker toggle.
- Action context packets and `ProductMenuActionResult`.
- Starter confirm behavior, including continue/load/new world/creative new/open/save-browser/settings/dev-tools/exit paths.
- Pause confirm behavior, including resume, save, save-and-exit, load/save browser, settings/dev-tools, title return, and exit.
- Mirroring save-browser mode between frontend state and window save-session state.

## Does Not Own

- Frontend action order, labels, or pure starter route modeling.
- Active-surface owner resolution or generic input acceptance.
- Draw-list layout, hit regions, or visual presentation.
- Runtime save codec, file-store primitives, session tick behavior, or creative document internals.

## Reads

- Frontend state, product options, save bridge result, settings tab, optional active session, world setup draft, window state, close-request flag, settings, and optional creative app.
- Save-slot selection state and map-maker/creative active predicates.

## Writes / Mutates

- `FrontendState`, `ProductAppWindowState`, save/session selection state, world setup draft, optional active session, close-request flag, and settings through action contexts.
- Triggers save/catalog refreshes through save bridge/slot operations.
- Can request app close or frontend transition.

## Calls Out To / Wires Out To

- Starter screen model and frontend state helpers.
- Save flow, save slot operations, product session launch/creation, creative world operations, settings/dev-tools/menu models, and frontend router predicates.
- Runtime/app seams through active session load/create/save operations.

## Called By / Entry Points

- `InputRouter.cpp` dispatches routed menu actions here.
- `window/InputFrame.cpp` and tests call system pause and specific menu handlers directly.
- Grep proof: `rg -n "applyProductPauseMenuAction|applyProductStarterMenuAction|applyProductLoadSaveMenuAction|applyProductSystemPauseMenuAction|confirmStarterCreativeNewWorld|confirmStarterCreativeOpenWorld" src tests cmake`.

## Invariants

- Handler results must report handled/action/status consistently for receipts and tests.
- Save-browser mode mirrors must stay in sync for starter and pause load/delete flows.
- CreativeDocument launch/open uses creative world operations and must not fall back to legacy map-maker.
- System pause owns global pause/back/escape behavior before surface-specific menu dispatch.
- This file is a choke point; do not add reusable kernels or renderer/layout code here.

## Tests / Proof Commands

- `rg -n "product_starter_menu_action_tests|product_window_input_frame_tests|product_creative_world_launch_tests|pause_menu_tests|product_save_delete_executor_tests" cmake tests`.
- `rg -n "applyProduct.*MenuAction|confirmStarter" src/app/iggy3d/menu tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/InputRouter.*` unless dispatch wiring changes.
- `src/app/iggy3d/save/*` unless save mutation contracts change.
- `src/app/iggy3d/creative/*` unless creative launch/save contracts change.
- `src/runtime/*` unless runtime command/session semantics change.

## Update When

- Any menu action context, state mutation, launch/save/delete behavior, pause behavior, or save-browser mirror rule changes.

## Do Not Update When

- Only visual row layout or semantic IDs change without handler behavior changes.
