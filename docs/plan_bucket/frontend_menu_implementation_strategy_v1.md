# Frontend Menu Implementation Strategy v1

## Objective

Define the durable implementation strategy for product frontend menus before
more code is added. The goal is to make starter, pause, settings, dev tools,
selectors, save browsing, and future editor surfaces fit into one routing and
view model instead of adding temporary branches to `AppShell.cpp`.

This is a plan only. It does not authorize source edits.

## Durable File And Module Layout

Later implementation should prefer these product-owned files:

```text
src/app/frontend/FrontendRoute.hpp
src/app/frontend/FrontendRoute.cpp
src/app/frontend/FrontendActionTable.hpp
src/app/frontend/FrontendActionTable.cpp
src/app/frontend/VerticalFadedSelector.hpp
src/app/frontend/VerticalFadedSelector.cpp
src/app/frontend/SettingsScreenModel.hpp
src/app/frontend/SettingsScreenModel.cpp
src/app/frontend/DevToolsOverlayModel.hpp
src/app/frontend/DevToolsOverlayModel.cpp
src/app/frontend/StarterMenuModel.hpp
src/app/frontend/StarterMenuModel.cpp
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
src/app/iggy3d/ProductFrontendViewModel.hpp
src/app/iggy3d/ProductFrontendViewModel.cpp
src/app/iggy3d/ProductFrontendReceipt.hpp
src/app/iggy3d/ProductFrontendReceipt.cpp
```

Existing files should remain the long-term owners where they already match the
domain:

```text
src/app/frontend/FrontendState.*
src/app/frontend/MenuInput.*
src/app/frontend/StarterScreen.*
src/app/frontend/SettingsMenu.*
src/app/frontend/DevToolsMenu.*
src/app/frontend/SaveSlotModel.*
src/app/iggy3d/ProductMenuTransitions.*
src/app/iggy3d/ReceiptBuilder.*
src/app/iggy3d/AppShell.cpp
```

## AppShell Rule

`src/app/iggy3d/AppShell.cpp` must remain lifecycle and composition only.

It may:

- parse already-built product options;
- create/load the product session through existing helpers;
- create frontend state;
- collect device input;
- call `ProductFrontendRouter`;
- call product view/render/receipt builders;
- own the outer frame loop.

It must not:

- decide per-row menu behavior;
- contain starter/settings/dev-tools branch chains;
- parse save rows directly;
- mutate settings drafts directly;
- run selector navigation logic;
- own dev tools category behavior;
- duplicate keyboard, mouse, controller, and automation behavior paths.

## Ownership Map

| Domain | Owner | Reads | Writes / returns |
| --- | --- | --- | --- |
| App lifecycle | `AppShell.cpp` | options, platform state, session presence | calls routers/builders, exits |
| Frontend route state | `FrontendState.*` and `FrontendRoute.*` | current screen, child panel, parent | route result, status, input owner |
| Starter rows | `StarterMenuModel.*` or `StarterScreen.*` | save summaries, route state | row list, disabled reasons, selected row |
| Selector state | `VerticalFadedSelector.*` | item summaries | selected index/id, scroll status, confirm/back result |
| Settings draft | `SettingsScreenModel.*` or `SettingsMenu.*` | active settings, device capabilities | draft settings, dirty state, apply/restore result |
| Dev tools overlay | `DevToolsOverlayModel.*` or `DevToolsMenu.*` | runtime/debug/frontend/input summaries | selected category, read-only rows, requested debug toggles |
| Transitions | `ProductMenuTransitions.*` | route result, session presence | enter gameplay, return title, open/close overlay |
| Receipts | `ReceiptBuilder.*` plus optional `ProductFrontendReceipt.*` | route result, models, app state | deterministic key-value proof |
| Views | `OpeningMenuView.*` or future frontend view files | view model only | drawing commands, no gameplay mutation |

## Routing Semantics

Route ownership is a stack:

1. boot/status if present;
2. starter child panel;
3. starter menu;
4. pause child panel;
5. pause menu;
6. dev tools overlay;
7. editor overlay;
8. gameplay.

The router receives:

```text
FrontendState
FrontendInputFrame
ProductSaveSummarySet
FrontendSettings
ProductSessionPresence
ProductRouteContext
```

The router returns:

```text
ProductFrontendRouteResult
```

Required route result fields:

```text
accepted=true|false
input_owner=<owner>
next_screen=<screen>
next_child_screen=<screen-or-none>
requested_transition=<none|launch_gameplay|return_to_title|save|save_and_exit|exit>
status=<stable-status>
gameplay_input_suppressed=true|false
receipt_reason=<stable-reason>
```

The app shell applies transition requests through `ProductMenuTransitions.*`.
The router decides what is requested; the transition layer performs lifecycle
changes.

## Table-Driven Action Surfaces

Menus should use table/surface definitions instead of if/else chains.

Each row definition should include:

```text
id
label
action
enabled
disabled_reason
visible
parent_screen
child_screen_target
receipt_key_prefix
```

Each router should:

- ask the active model for its row table;
- move selection through reusable navigation helpers;
- execute the selected row by action id;
- return a route result;
- avoid per-device behavior.

## Input Wiring Semantics

All devices and automation must feed semantic actions.

Physical inputs:

- keyboard;
- mouse;
- PS5/DualSense;
- future Xbox/Joy-Con;
- automation/control file.

Device adapters produce normalized actions:

```text
menu.up
menu.down
menu.left
menu.right
menu.confirm
menu.back
menu.next_tab
menu.previous_tab
menu.page_up
menu.page_down
system.pause
system.quit_chord
dev.toggle
editor.toggle
```

Menus must never ask whether a physical device pressed a specific key or
button. They consume semantic actions only.

Mouse input must also become semantic actions:

- click focused row -> `menu.confirm`;
- wheel up/down -> `menu.up` / `menu.down` or selector scroll actions;
- hover may set focus only if the active view supports stable hit regions.

Automation must use the same semantic action names. Do not add a second
automation-only behavior path.

## Data Ownership

Save summaries:

- owned by save store/save bridge;
- read by starter and selector models;
- not parsed in the view.

Selector state:

- owned by `VerticalFadedSelector`;
- parent screens own what confirm/delete/load means.

Settings draft state:

- owned by settings model;
- apply copies draft into active frontend settings;
- restore defaults resets draft;
- back discards unapplied changes until a confirmation screen is planned.

Dev tools overlay state:

- owned by dev tools model;
- reads runtime/debug summaries;
- may request debug toggles;
- does not own runtime truth.

Frontend route result state:

- owned by the router for one input frame;
- copied into receipts for proof;
- applied by app shell/transition layer.

## Receipt And Proof Semantics

No-window receipts are the primary proof surface.

Required proof families:

```text
frontend_screen=<screen>
frontend_child_screen=<screen-or-none>
frontend_selected_action=<action>
frontend_status=<status>
input_owner=<owner>
input_action_last=<action-or-none>
input_action_accepted=true|false
gameplay_input_suppressed=true|false
selector_visible=true|false
settings_open=true|false
dev_tools_enabled=true|false
dev_overlay_visible=true|false
window_launch_count=0
```

Receipts must remain deterministic key-value text. Do not introduce JSON.

## Testing Strategy

Test order:

1. Unit tests for row tables and model state.
2. Unit tests for route results from semantic actions.
3. Unit tests for selector behavior.
4. Unit tests for settings draft/apply/restore.
5. Unit tests for dev tools overlay model.
6. Product no-window smokes for receipts.
7. Manual/window proof only after a packet explicitly asks for visual proof.

Default verification should keep:

```text
window_launch_count=0
```

## Migration Strategy

Do not add a temporary second router in `AppShell.cpp`.

Safe migration order:

1. Introduce model/action-table helpers with no behavior change.
2. Move starter row table into a model.
3. Move settings tab/row table into a model.
4. Move dev tools category rows into a model.
5. Add `ProductFrontendRouter` and route only one surface at a time.
6. Replace old `AppShell.cpp` branches with router calls after tests prove parity.
7. Add receipts from route result fields.
8. Remove stale branch code only when no tests depend on it.

Each phase should preserve existing no-window receipt behavior unless the packet
explicitly changes a field.

## Stop Rules

Stop before implementation if a plan requires:

- runtime/session/save schema changes;
- renderer/Vulkan changes;
- a real window launch;
- broad visual redesign;
- package visual shell changes;
- editor mutation beyond routing to existing editor commands;
- per-device menu behavior paths;
- JSON or another new machine-contract format.

## Long-Term Fit

This strategy supports:

- starter menu;
- pause menu;
- settings;
- dev tools;
- save browser;
- delete confirmation;
- world creation;
- future editor panels;
- controller/mouse/keyboard/automation parity.

The stable idea is one route pipeline, many surface models, semantic actions,
and deterministic receipts.
