# File Spec

Files: `src/app/frontend/DevToolsMenu.hpp`, `src/app/frontend/DevToolsMenu.cpp`

Verified at: `d9665a0c`

## Owns

- Dev-tools menu model and category labels.
- Dev-tools parent-owner route decisions.
- Function-key hint label and category readout counts.

## Does Not Own

- Debug overlay data generation, debug HUD rendering, dev-tools input polling, product window state mutation, or receipt field emission.

## Reads

- Selected `FrontendDevToolsCategory`, parent `MenuOwner`, selected `FrontendAction`, and selected model state.

## Writes / Mutates

- No mutation; returns `DevToolsMenuModel` and `FrontendRouteResult`.

## Calls Out To / Wires Out To

- Uses `devToolsCategoryOrder()` from frontend state.
- Uses generic frontend route result constructors.
- Product menu draw-list, frame presenter, and route application paths consume category labels and route results.

## Called By / Entry Points

- `buildDevToolsMenuModel(...)`, `routeDevToolsAction(...)`, `devToolsCategoryLabel(...)`, `devToolsFunctionKeyHintLabel(...)`, and `devToolsReadoutCount(...)`.
- Grep proof: `rg -n "buildDevToolsMenuModel|routeDevToolsAction|devToolsCategoryLabel|devToolsReadoutCount" src/app tests/unit`.

## Invariants

- Valid dev-tools parents are starter, gameplay, and pause.
- Back closes to the parent owner with parent-specific screen/child routing.
- Apply selects the current category only when parent is valid and selected category is enabled.
- Category labels and readout counts are presentation model facts, not runtime debug data.

## Tests / Proof Commands

- `rg -n "dev_tools_menu_tests|product_frontend_router_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routeDevToolsAction|devToolsCategoryLabel|devToolsReadoutCount" tests/unit/dev_tools_menu_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/frontend/FrontendState.*` unless dev-tools categories change.
- `src/app/iggy3d/menu/DrawList.*` unless dev-tools drawing changes.
- `src/app/iggy3d/debug/*` unless debug content changes.

## Update When

- Dev-tools categories, labels, readout-count rules, parent-owner routing, or route statuses change.

## Do Not Update When

- Only the debug facts displayed inside an existing category change.
