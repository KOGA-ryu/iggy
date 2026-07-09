# File Spec

Files: `src/app/frontend/SettingsMenu.hpp`, `src/app/frontend/SettingsMenu.cpp`

Verified at: `a6f36636`

## Owns

- Frontend settings data packet and settings tab/model enums.
- Settings tab order, tab navigation, default row model selection, and settings route handling.
- Settings defaults, restore defaults, and draft apply helper.
- Settings route parent-owner rules for starter and pause.

## Does Not Own

- Product settings UI drawing, persistent settings storage, renderer restart behavior, audio/backend implementation, product frontend route application, or receipt field emission.

## Reads

- Current/draft `FrontendSettings`, selected tab, selected row, dirty flag, parent owner, and selected frontend action.
- Settings parent owner to determine return screen.

## Writes / Mutates

- `restoreFrontendSettingsDefaults(...)` and `applyFrontendSettingsDraft(...)` mutate caller-owned settings.
- Route functions return `FrontendRouteResult` packets and do not mutate frontend state.

## Calls Out To / Wires Out To

- Uses `makeIgnoredFrontendRouteResult(...)` and `makeAcceptedFrontendRouteResult(...)`.
- Receipt and product UI paths read settings names, tab names, and row model facts.

## Called By / Entry Points

- Product menu/window paths route settings actions and update selected settings tab.
- Receipt fields read settings values and tab names.
- Grep proof: `rg -n "FrontendSettings|FrontendSettingsTab|SettingsRowModel|SettingsRouteContext|frontendInputBackendName|frontendSettingsTabName|settingsTabOrder|nextSettingsTab|previousSettingsTab|routeSettingsAction|routeSettingsBackToParent|defaultFrontendSettings|restoreFrontendSettingsDefaults|applyFrontendSettingsDraft" src/app tests/unit`.

## Invariants

- Settings parent owner is valid only for starter or pause.
- Audio row is disabled when audio is unavailable.
- Apply with no dirty draft is ignored with `settings_no_changes`.
- Applying a draft sets renderer-change pending by comparing draft renderer to default renderer.
- Settings values here are frontend/app configuration, not runtime save/hash state.

## Tests / Proof Commands

- `rg -n "settings_menu_tests|product_window_input_frame_tests|product_starter_menu_action_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routeSettingsAction|routeSettingsBackToParent|settingsTabOrder|defaultSettingsRowModel|applyFrontendSettingsDraft" tests/unit/settings_menu_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/OpeningMenuView.*` unless settings UI drawing changes.
- `src/app/iggy3d/menu/InputRouter.*` unless settings input application changes.
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp` unless receipt fields change.

## Update When

- Settings fields, tab names/order, row model semantics, route behavior, defaults, or draft-apply rules change.

## Do Not Update When

- Only product UI layout changes without changing settings model or route contracts.
