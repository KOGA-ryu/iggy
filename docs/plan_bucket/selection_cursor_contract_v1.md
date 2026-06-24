# Selection And Cursor Contract v1

## Objective

Define how product selection works across first-person gameplay, tactical view,
editor view, menus, and dev tools without device-specific branches or renderer
mutation of runtime truth.

Selection is a product/app interpretation of camera, cursor, and hit testing.
Runtime systems remain authoritative for gameplay results.

This is a plan only. It does not authorize source edits.

## User Decision

Authoritative selection source depends on active camera or surface:

1. first-person gameplay uses center crosshair selection through a camera
   raycast;
2. tactical view, editor view, menus, and dev tools use a rendered mouse cursor
   with picking/hit testing;
3. active camera or frontend mode decides the selection source;
4. priority is first-person crosshair first, then tactical/editor/menu/dev
   tools cursor;
5. dev tools top-right inspector reads the current selected object from this
   shared selection state.

## Proposed Durable Files

Later implementation should prefer these product/frontend files:

```text
src/app/frontend/CursorState.hpp
src/app/frontend/CursorState.cpp
src/app/iggy3d/ProductSelectionController.hpp
src/app/iggy3d/ProductSelectionController.cpp
src/app/iggy3d/ProductSelectionSummary.hpp
src/app/iggy3d/ProductSelectionSummary.cpp
```

Existing owners should feed into this boundary:

```text
src/app/input/*
src/app/frontend/MenuInput.*
src/app/frontend/DevToolsMenu.*
src/app/iggy3d/ProductViewportState.*
src/app/iggy3d/ProductPrimitiveDrawList.*
src/app/iggy3d/ProductViewportFraming.*
src/app/iggy3d/ProductRenderBridgeFrame.*
```

## CursorState Ownership

`CursorState` owns frontend cursor presentation intent:

```text
visible=true|false
screen_x=<float>
screen_y=<float>
mode=hidden|menu|tactical|editor|dev_tools
icon_intent=default|select|inspect|move|resize|blocked
hovered_region=<id-or-none>
pressed_region=<id-or-none>
```

It must not own:

- gameplay command execution;
- runtime targeting truth;
- save/load state;
- renderer backend resources.

Mouse rendering is a frontend/render overlay concern. It is not gameplay logic.

## ProductSelectionController Ownership

`ProductSelectionController` owns product selection semantics:

- crosshair raycast request for first-person gameplay;
- mouse picking request for tactical/editor/dev/menu surfaces;
- priority resolution between crosshair and cursor;
- stable selected object summary for receipts and dev tools;
- hit-test status and selection source.

It does not mutate runtime state. It returns selection facts that other systems
may use to request commands through existing runtime admission paths.

## Selection Source Rules

| Active surface / mode | Authoritative source | Cursor visible | Notes |
| --- | --- | --- | --- |
| First-person gameplay | center crosshair raycast | false by default | crosshair selection feeds target/reach feedback |
| Tactical gameplay | mouse cursor picking | true | tactical overhead camera later |
| Editor | mouse cursor picking | true | editor tools may use grid/object hit regions |
| Starter/settings/menu selector | menu focus plus cursor hit regions | true when mouse active | keyboard/controller focus remains valid |
| Dev tools overlay | mouse cursor picking plus current gameplay selection | true | top-right inspector reads shared selected object |

If the active mode changes, stale selection must be either revalidated or
reported as unavailable. Do not keep a selected object from a different surface
without marking the source.

## Mouse Behavior Defaults

V1 defaults:

- hover highlights only if stable hit regions exist;
- click selects or activates according to the active surface;
- wheel scrolls the active selector or list;
- click outside active hit regions is ignored;
- no drag selection unless a later editor packet adds it;
- no mouse-only behavior that lacks a semantic action equivalent for tests.

## Crosshair Behavior Defaults

First-person crosshair selection:

- uses active gameplay camera origin and direction;
- raycast range should match existing target/reach policy where possible;
- result can be `none`, `target`, `interactable`, `blocked`, or `out_of_range`;
- does not execute interaction by itself;
- feeds product feedback and dev tools inspector.

The crosshair is a selection source, not a gameplay command.

## Hit Testing Boundaries

Hit testing may read:

- product view frame items;
- product render bridge summaries;
- frontend view hit regions;
- editor object bounds;
- runtime debug/projection summaries.

Hit testing must not:

- mutate runtime state;
- mutate save files;
- depend on renderer backend resources;
- use pointer address ordering;
- invent gameplay targeting rules outside runtime command admission.

## Route And Input Integration

The frontend router owns which surface receives input first.

Selection/cursor receives:

```text
FrontendState
ProductViewportState
CursorState
MenuActionFrame
MousePointerFrame
ProductRenderBridgeFrame
EditorHitRegionSummary
```

It returns:

```text
ProductSelectionSummary
```

The summary is read by:

- product feedback;
- dev tools top-right inspector;
- editor controller;
- receipt builder;
- runtime command request builder when an action is explicitly confirmed.

## Receipt Fields

Recommended fields:

```text
selection_source=crosshair|mouse_cursor|menu_focus|none
selection_active_mode=first_person|tactical|editor|menu|dev_tools|none
selection_selected_id=<id-or-none>
selection_selected_kind=<kind-or-none>
selection_hit_status=hit|miss|blocked|out_of_range|unavailable
selection_priority=crosshair|cursor
cursor_visible=true|false
cursor_mode=hidden|menu|tactical|editor|dev_tools
cursor_icon_intent=<intent>
cursor_hovered_region=<id-or-none>
cursor_click_accepted=true|false
```

Dev tools may also report:

```text
dev_selected_entity_id=<id-or-none>
dev_selection_source=<source>
```

## Test Strategy

Unit tests should prove:

- first-person chooses crosshair over cursor;
- menu/dev/editor surfaces choose cursor or focus according to route state;
- click outside stable regions is ignored;
- wheel scroll is routed to the active selector;
- stale selection is cleared or marked unavailable when surface changes;
- dev tools inspector reads the shared selected object summary.

Product no-window smokes should use semantic actions and synthetic hit
summaries. Real mouse rendering proof requires a later visual packet.

## Stop Rules

Stop before implementation if the plan requires:

- renderer/Vulkan changes;
- runtime gameplay rule changes;
- save schema changes;
- window launches by default;
- device-specific menu behavior;
- JSON or a new machine-contract format.

## Long-Term Fit

This keeps first-person gameplay, tactical view, editor tools, menus, and dev
tools on one selection contract. The product can grow from primitive draw lists
to true rendered objects without changing the rule that active surface decides
selection source and runtime remains gameplay authority.
