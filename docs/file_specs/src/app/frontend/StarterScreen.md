# File Spec

Files: `src/app/frontend/StarterScreen.hpp`, `src/app/frontend/StarterScreen.cpp`

Verified at: `6c79462b`

## Owns

- Pure starter menu model: `StarterScreenModel`.
- Starter action labels, command IDs, enablement, disabled reasons, and compatible-save count facts.
- Pure `routeStarterAction(...)` results for Continue, Build Map, Creative World, Continue Creative, Existing Maps, Delete Map, Settings, Dev Tools, and Exit.
- Starter child-back route result.

## Does Not Own

- Launch execution, save loading, creative world creation, filesystem access, or app window mutation.
- Input polling, menu owner resolution, hit testing, or draw-list emission.
- Runtime session construction.

## Reads

- Compatible save count supplied by product save scanning.
- Selected `FrontendAction` supplied by caller/frontend state.
- Canonical starter action order from `FrontendState.cpp`.

## Writes / Mutates

- No external state.
- Returns model/result packets only.

## Calls Out To / Wires Out To

- Calls frontend route helpers from `FrontendRoute.*`.
- Calls `starterActionOrder()` and frontend action names/commands.
- Provides route decisions consumed by product action handlers and starter UI draw-list tests.

## Called By / Entry Points

- `DrawList.cpp` builds starter UI from `buildStarterScreenModel(...)`.
- Starter action tests call `routeStarterAction(...)`.
- Grep proof: `rg -n "buildStarterScreenModel|routeStarterAction|starterActionLabel|starterActionCommand" src tests cmake`.

## Invariants

- Continue, Load Save, and Delete are disabled when compatible save count is zero.
- Creative new world remains independent of compatible product-save count.
- Delete from starter routes to the save selector first; final deletion happens through delete confirm.
- Route results are pure descriptions and must not mutate frontend/window state.

## Tests / Proof Commands

- `rg -n "starter_screen_tests|product_ui_draw_list_tests|product_starter_menu_action_tests" cmake tests`.
- `rg -n "starter_creative_new_world|starter_delete_select_opened|no_compatible_save" src tests`.

## Nearby Files Usually Not Touched

- `src/app/frontend/FrontendState.*` unless action order or enum shape changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless accepted route execution changes.
- `src/app/iggy3d/menu/DrawList.*` unless starter model presentation changes.

## Update When

- Starter row set, labels, command IDs, enablement, disabled reasons, or route status values change.

## Do Not Update When

- Launch/save behavior changes behind an unchanged starter route result.
- Only visual styling changes in the product draw list.
