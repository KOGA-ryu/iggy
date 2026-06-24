# Product Frontend Router Contract v0.1

## Objective

Define the product frontend router before more menu behavior moves out of
`AppShell.cpp`.

The router answers one question per input frame:

```text
given current frontend state, product summaries, and semantic input,
which surface owns this input, what route result is produced, and what
transition is requested?
```

The router is a traffic controller. It does not execute saves, loads, world
creation, session replacement, renderer work, app exit, or settings persistence.

This is a plan only. It does not authorize source edits.

## Current Baseline

The repo already has several frontend routing pieces:

```text
src/app/frontend/FrontendState.hpp
src/app/frontend/FrontendState.cpp
src/app/frontend/FrontendRoute.hpp
src/app/frontend/FrontendRoute.cpp
src/app/frontend/MenuInput.hpp
src/app/frontend/MenuInput.cpp
src/app/frontend/StarterScreen.hpp
src/app/frontend/StarterScreen.cpp
src/app/frontend/SettingsMenu.hpp
src/app/frontend/SettingsMenu.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
src/app/frontend/VerticalFadedSelector.hpp
src/app/frontend/VerticalFadedSelector.cpp
src/app/frontend/FrontendReceipt.hpp
src/app/frontend/FrontendReceipt.cpp
src/app/iggy3d/ProductMenuTransitions.hpp
src/app/iggy3d/ProductMenuTransitions.cpp
src/app/iggy3d/FrontendActionExecutor.hpp
src/app/iggy3d/FrontendActionExecutor.cpp
src/app/iggy3d/AppShell.cpp
```

Existing useful pieces:

- `FrontendScreen` and `FrontendAction` already name product screens/actions;
- `MenuInputAction` already names semantic menu actions;
- `MenuOwner` already names input owners;
- `chooseMenuOwner` and `menuOwnerBlocksGameplay` already exist;
- `FrontendTransitionRequest` and `FrontendRouteResult` already exist;
- `routeStarterAction` and `routeStarterBackFromChild` already exist;
- `routeSettingsAction` and `routeSettingsBackToParent` already exist;
- `SaveBrowserModel` and `VerticalFadedSelector` already model save selector
  presentation;
- `ProductMenuTransitions` already applies some high-level screen/lifecycle
  transitions;
- `FrontendReceipt` and `ReceiptBuilder` already emit frontend/menu proof
  fields.

Current limitations:

- no `ProductFrontendRouter.*` coordinator exists;
- `AppShell.cpp` still owns direct menu routing branches;
- `AppShell.cpp` still has `routeOpeningMenuInput` and
  `applyOpeningMenuAction` branch logic;
- product automation currently routes through `AppShell.cpp` helpers;
- `FrontendActionExecutor` mutates `FrontendState` directly and predates the
  route-result model;
- starter/settings have route helpers, but no product owner-selection pipeline
  delegates to them consistently;
- save selector, world setup, dev tools, and pause route helpers are not all
  promoted to the same contract yet.

## Source-Fit Notes For Builders

These notes describe current source constraints. Later implementation packets
must fit them instead of inventing hidden routing semantics.

- `MenuOwner` currently has `None`, `Starter`, `Pause`, `Settings`, `DevTools`,
  `Editor`, and `Gameplay`. It does not have `SaveSelector`, `WorldSetup`, or
  `ConfirmDialog` owners.
- `chooseMenuOwner` currently checks `starter`, then `pause`, then `settings`,
  then `devTools`, then `editor`, then `gameplay`. That helper is useful but is
  not sufficient for product child-surface priority by itself.
- Slice 1 must not edit `MenuInput.*` just to add future owners. If finer
  product surfaces are needed, define them inside `ProductFrontendRouter.*` as a
  product-local active-surface/model name while mapping to the closest existing
  `MenuOwner` for input ownership and receipts.
- Current starter/settings route helpers already return `FrontendRouteResult`.
  The router should delegate to those helpers later; it must not duplicate their
  disabled-row, parent-return, or status rules.
- Current `AppShell.cpp` has direct input branches and automation helpers.
  Slice 1 must leave those branches untouched and prove only pure owner/surface
  decisions in unit tests.
- Current source still has a legacy `MenuInputSource::Codex` enum value. New
  product routing docs and receipts should prefer `automation`/`scripted`
  language, and should not add new Codex-branded product surfaces.

## Coding Method

The router must use table-driven, pure, semantic routing.

### Table-Driven Surfaces

Menu surfaces expose row/action tables:

```text
starterActionOrder()
pauseActionOrder()
settingsTabOrder()
VerticalSelectorItem[]
```

Why:

- row order is data, not branch order;
- disabled reasons are testable;
- receipts can prove selected row and action;
- adding a row should not require editing a giant conditional chain.

### Pure Route Functions

Surface route functions return route results without mutating runtime state:

```text
routeStarterAction(...)
routeSettingsAction(...)
routeSaveSelectorAction(...) later
routeWorldSetupAction(...) later
routeDevToolsAction(...) later
routePauseAction(...) later
```

Why:

- unit tests can construct exact state;
- keyboard, controller, mouse, and automation share behavior;
- no-window receipts can prove behavior;
- AppShell migration can happen last.

### One Product Coordinator

Add a product-owned coordinator later:

```text
routeProductFrontend(...)
```

Why:

- `AppShell.cpp` should compose lifecycle, not own per-menu behavior;
- only one surface should consume each input frame;
- owner selection stays auditable.

### Semantic Input Only

The router consumes semantic input:

```text
menu.up
menu.down
menu.left
menu.right
menu.confirm
menu.back
menu.next_tab
menu.previous_tab
system.pause
system.quit_chord
dev.toggle
```

It must not branch on physical buttons or keys.

Why:

- keyboard, mouse, PS5 controller, future controllers, and automation all map
  into the same menu behavior;
- device-specific code stays in input adapters.

### Value Types For Context And Result

Use small structs:

```text
ProductFrontendRouteContext
ProductFrontendRouteOwnerResult
ProductFrontendRouteFrame
FrontendRouteResult
SettingsRouteContext
```

Why:

- data ownership is visible;
- tests do not need global state;
- receipts can copy fields deterministically.

### Enums With Name Functions

Use enums in code and stable string conversion for proof:

```text
FrontendScreen
FrontendAction
FrontendTransitionRequest
MenuOwner

frontendScreenName()
frontendActionName()
frontendTransitionRequestName()
menuOwnerName()
```

Why:

- code stays type-safe;
- receipts stay grep-friendly;
- unsupported cases are explicit.

### Transition Requests, Not Side Effects

Router returns transition requests:

```text
requestedTransition=launch_gameplay
requestedTransition=create_world
requestedTransition=load_save
requestedTransition=save
requestedTransition=soft_delete_save
```

It does not execute them.

Why:

- router is traffic control;
- product save/world/session systems perform side effects;
- runtime truth is not mutated by menu row code.

## Proposed Durable Files

Likely new files:

```text
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
tests/unit/product_frontend_router_tests.cpp
```

Existing files to reuse:

```text
src/app/frontend/FrontendState.hpp
src/app/frontend/FrontendState.cpp
src/app/frontend/FrontendRoute.hpp
src/app/frontend/FrontendRoute.cpp
src/app/frontend/MenuInput.hpp
src/app/frontend/MenuInput.cpp
src/app/frontend/StarterScreen.hpp
src/app/frontend/StarterScreen.cpp
src/app/frontend/SettingsMenu.hpp
src/app/frontend/SettingsMenu.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
src/app/frontend/PauseMenu.hpp
src/app/frontend/PauseMenu.cpp
src/app/frontend/DevToolsMenu.hpp
src/app/frontend/DevToolsMenu.cpp
src/app/iggy3d/ProductMenuTransitions.hpp
src/app/iggy3d/ProductMenuTransitions.cpp
src/app/iggy3d/ReceiptBuilder.hpp
src/app/iggy3d/ReceiptBuilder.cpp
```

Existing files to treat as migration targets, not first-slice homes:

```text
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/FrontendActionExecutor.hpp
src/app/iggy3d/FrontendActionExecutor.cpp
```

`FrontendActionExecutor` may be retired, narrowed, or wrapped later. Do not
build new product routing around direct state mutation if a route-result helper
can express the behavior instead.

## No-Go Files

Do not touch these in the router foundation slices:

```text
src/runtime/**
src/render/**
src/render/vulkan/**
docs/vulkan/**
fixtures/** package schemas
apps/iggy3d_visual_demo/**
```

Do not add JSON or any new external machine-contract format.

## Data Ownership

| Domain | Owns | Must not own |
| --- | --- | --- |
| Input adapters | physical key/button/mouse/controller mapping to semantic actions | menu row behavior |
| `MenuInput.*` | semantic input names, owner names, owner blocking facts | product route delegation |
| `FrontendState.*` | current screen, child screen, selected action, flags | save/load/world side effects |
| Surface models | rows, selected row, disabled reasons, field state | app lifecycle |
| Surface route helpers | pure per-surface route result | owner selection for unrelated surfaces |
| `ProductFrontendRouter.*` | owner decision and delegation to active surface route helper | session creation, save writing, load execution |
| `ProductMenuTransitions.*` | applying approved screen/lifecycle transitions | choosing row semantics |
| Product save/world systems | save/load/create execution after explicit transition request | menu input ownership |
| `AppShell.cpp` | outer loop composition, calling router, applying transition requests | per-row branching |
| Receipts | deterministic proof fields | control decisions |

## Router Context

Proposed context:

```text
ProductFrontendRouteContext
  frontend_state
  menu_input_action
  menu_input_source
  starter_model
  pause_model
  settings_context
  save_browser_model
  world_setup_model
  dev_tools_model
  has_active_session
  has_compatible_save
  selected_save_id
```

V0.1 may use pointers or optional references for models that do not exist yet.
Absent models must produce `accepted=false` with a stable unavailable reason,
not crash.

The context must not contain renderer backend resources.

## Router Output

The primary output should reuse or wrap `FrontendRouteResult`.

Required output fields:

```text
accepted
input_owner
next_screen
next_child_screen
selected_action
requested_transition
close_requested
gameplay_input_suppressed
status
receipt_reason
```

Future product-specific fields may include:

```text
target_save_id
target_world_id
world_create_requested
save_operation_requested
```

These fields should be request data only. They do not mean the router executed
the request.

## Owner Priority

Only one owner consumes a semantic input frame.

Priority:

```text
boot_status
confirm_dialog
save_selector
world_setup
settings
dev_tools
pause
starter
gameplay
none
```

Owner mapping:

```text
boot_status -> none or starter boot handling
confirm_dialog -> current parent owner
save_selector -> starter or pause parent, depending opener
world_setup -> starter
settings -> settings
dev_tools -> dev_tools
pause -> pause
starter -> starter
gameplay -> gameplay
```

Because current `MenuOwner` lacks `save_selector`, `world_setup`, and
`confirm_dialog`, Slice 1 should represent this as two pieces:

```text
input_owner=<existing MenuOwner>
active_surface=starter|pause|settings|dev_tools|gameplay|save_selector|world_setup|confirm_dialog|none
```

For example, `screen=starter child=new_world` should report:

```text
input_owner=starter
active_surface=world_setup
gameplay_input_suppressed=true
```

Do not expand `MenuOwner` in Slice 1 unless the builder also updates all owner
name tests and proves no receipt drift.

If the active child screen is more specific than the parent screen, the child
owns input first.

Examples:

```text
screen=starter child=new_world -> world_setup owns input
screen=starter child=load_save -> save_selector owns input
screen=settings child=pause -> settings owns input, parent is pause
screen=pause child=gameplay -> pause owns input
screen=gameplay child=gameplay -> gameplay owns input
```

## Parent Return Rules

Parent return must be explicit and receipt-visible.

Rules:

```text
world_setup from starter -> back returns starter
settings from starter -> back returns starter
settings from pause -> back returns pause
save_selector from starter -> back returns starter
save_selector from pause -> back returns pause
delete_confirm from starter -> back returns starter or save selector parent
dev_tools from starter -> back returns starter
dev_tools from pause/gameplay -> back returns pause/gameplay parent
pause -> resume returns gameplay
```

Invalid parent:

```text
accepted=false
status=invalid_parent
receipt_reason=invalid_parent
```

## Transition Vocabulary

Existing transition requests:

```text
none
launch_gameplay
return_to_title
save
save_and_exit
exit
```

Needed future transition requests:

```text
create_world
load_save
soft_delete_save
recover_save
permanent_delete_save
open_settings
open_dev_tools
resume_gameplay
```

Do not add all future transitions in the first slice. The contract should name
them so later enum expansion is deliberate.

## Surface Delegation

Starter:

```text
routeStarterAction(starter_model, action)
routeStarterBackFromChild(child_screen)
```

Settings:

```text
routeSettingsAction(settings_context, action)
routeSettingsBackToParent(parent_owner)
```

Save selector later:

```text
routeSaveSelectorAction(save_browser_model, action, parent_owner)
```

World setup later:

```text
routeWorldSetupAction(world_setup_model, action)
```

Pause later:

```text
routePauseAction(pause_model, action)
```

Dev tools later:

```text
routeDevToolsAction(dev_tools_model, action, parent_owner)
```

The product router should choose the active surface and call exactly one of
these helpers.

## Control Flow

Intended product flow:

```text
physical input
  -> input adapter
  -> semantic menu action
  -> ProductFrontendRouter
  -> one surface route helper
  -> FrontendRouteResult
  -> AppShell applies route state and transition request
  -> ProductMenuTransitions / ProductSaveBridge / ProductWorldCreation execute later
  -> receipts
```

`AppShell.cpp` should eventually:

- collect input;
- build route context;
- call router;
- copy route result to state/window receipts;
- apply transition request through product systems;
- draw.

It should not decide per-row behavior.

## Gameplay Input Blocking

Default suppression:

```text
starter=true
pause=true
settings=true
save_selector=true
world_setup=true
dev_tools=true for v0.1
editor=true
gameplay=false
none=false
```

Dev tools may later support non-blocking overlays, but v0.1 should block
gameplay input while the dev tools UI owns navigation.

## Failure Semantics

Failure routes must be deterministic.

Disabled row:

```text
accepted=false
status=<disabled_reason>
receipt_reason=<disabled_reason>
```

Unsupported action:

```text
accepted=false
status=not_<surface>_action
receipt_reason=not_<surface>_action
```

Missing model:

```text
accepted=false
status=<surface>_model_unavailable
receipt_reason=<surface>_model_unavailable
```

Invalid parent:

```text
accepted=false
status=invalid_parent
receipt_reason=invalid_parent
```

Transition not executable yet:

```text
accepted=false
status=transition_unavailable
receipt_reason=transition_unavailable
```

## Receipt Fields

Receipts must remain deterministic key-value text.

Router proof fields:

```text
frontend_route_owner=<owner>
frontend_route_active_surface=<surface>
frontend_route_accepted=true|false
frontend_route_input_action=<action>
frontend_route_input_source=<source>
frontend_screen_before=<screen>
frontend_child_before=<screen-or-none>
frontend_screen_after=<screen>
frontend_child_after=<screen-or-none>
frontend_route_selected_action=<action>
frontend_transition_request=<request>
frontend_route_close_requested=true|false
frontend_route_gameplay_input_suppressed=true|false
frontend_route_status=<status>
frontend_route_reason=<reason>
```

Parent proof fields:

```text
frontend_route_parent_owner=<owner-or-none>
frontend_route_returned_to_parent=true|false
```

Missing model proof:

```text
frontend_route_model_available=true|false
frontend_route_model_name=<name-or-none>
```

## Test Plan

Unit tests should prove:

- owner priority chooses child surface before parent;
- gameplay owns input only when no blocking surface is open;
- starter route delegates to `routeStarterAction`;
- settings route delegates to `routeSettingsAction`;
- settings from starter returns to starter;
- settings from pause returns to pause;
- missing model returns unavailable status;
- disabled row returns ignored route and reason;
- unsupported action returns ignored route and reason;
- gameplay input suppression matches owner;
- route result fields are deterministic.

No-window smoke tests later should prove:

- product app starter routes through router receipts;
- automation and keyboard/controller inputs produce the same route result;
- pause/settings/dev tools route ownership is receipt-visible;
- no window launch is required.

## First Implementation Slice

Builder-safe first slice:

```text
Product Frontend Router / Slice 1 - Owner Decision
```

Allowed files:

```text
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
tests/unit/product_frontend_router_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Allowed behavior:

- define `ProductFrontendRouteContext`;
- define owner decision helper;
- map current `FrontendState` screen/child screen to route owner;
- report product-local active surface/model name when it is more specific than
  the existing `MenuOwner`;
- expose parent owner where applicable;
- unit tests for priority and suppression;
- no AppShell migration.

Explicitly not allowed in Slice 1:

- editing `AppShell.cpp`;
- executing save/load/world creation;
- changing `FrontendActionExecutor`;
- changing runtime/session/save code;
- changing renderer/Vulkan;
- changing automation behavior;
- adding window proof;
- adding JSON.

Slice 1 acceptance:

```text
cmake --build build --target iggy3d_app
cmake --build build --target product_frontend_router_tests
ctest --test-dir build --output-on-failure -R '^product_frontend_router_tests$'
git diff --check
```

## Later Implementation Order

1. Owner decision context and tests.
2. Route result wrapper/receipt fields.
3. Delegate starter through product router.
4. Delegate settings through product router.
5. Add save selector route helper and delegate through router.
6. Add world setup route helper and delegate through router.
7. Add pause route helper and delegate through router.
8. Add dev tools route helper and delegate through router.
9. Replace `AppShell.cpp` menu branches with router calls.
10. Retire or narrow `FrontendActionExecutor`.

Do not combine owner decision, AppShell migration, and save/world execution in
one slice.

## Acceptance Gate

This contract is builder-slice ready when:

- existing route helpers are named as reusable pieces;
- missing product router coordinator is identified;
- owner priority is explicit;
- parent return rules are explicit;
- transition request vocabulary is explicit;
- coding method is table-driven and pure;
- first slice is owner-decision only;
- stop rules prevent AppShell branch expansion and runtime mutation.

## Stop Rules

Stop before implementation if a slice requires:

- broad `AppShell.cpp` rewrite;
- router executing session/save/load/world creation;
- physical key/controller branching inside router logic;
- renderer/Vulkan changes;
- package fixture/schema changes;
- window launch by default;
- JSON or another external machine-contract format;
- changing save/world contracts before their model slices exist.

## Open Questions

No blocking user decisions remain for owner-decision Slice 1.

Deferred decisions:

- whether dev tools can become non-blocking while visible;
- final router receipt field placement in `ReceiptBuilder` versus a dedicated
  `ProductFrontendReceipt`;
- exact migration timing for `FrontendActionExecutor`;
- whether confirm dialogs become a generic model or stay per-surface first.

## Long-Term Fit

The product frontend router lets the app grow without turning `AppShell.cpp`
into a menu engine.

The stable split is:

- input adapters produce semantic actions;
- models describe surfaces;
- route helpers decide surface behavior;
- product router chooses the owner and delegates;
- transition systems execute side effects;
- receipts prove the result.
