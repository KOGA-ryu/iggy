# Vertical Faded Selector Contract v1

## Objective

Define a reusable vertical selector for save files, chapters, and future world
setup choices.

The selector lets the user scroll up and down through entries. Entries fade out
near the top and bottom of the visible list so the eye understands there are
more choices beyond the current viewport.

## Use Cases

- Save browser.
- Delete-save browser.
- Chapter selector.
- World-creation preset selector.

## Display Model

The control is a vertical stack with one focused entry.

Visual zones:

- top fade zone;
- upper neighboring entries;
- focused center entry;
- lower neighboring entries;
- bottom fade zone.

The focused entry is fully opaque and slightly emphasized. Entries above and
below are smaller or dimmer. Entries entering the top or bottom fade zones fade
out smoothly.

The fade is a viewport mask, not data deletion. Scrolling never removes items
from the selector model.

## Save File Entry Content

Each save entry must display:

- title;
- snapshot image from the save camera view;
- date/time;
- optional short location or chapter label later.

The snapshot image is part of the save summary presentation. The selector does
not own snapshot generation or save parsing.

## Chapter Entry Content

Each chapter entry should display:

- chapter title;
- optional subtitle;
- optional thumbnail/snapshot;
- last played or completion date when available.

The same selector state model should support chapter entries and save entries.
Only the data payload differs.

## State Ownership

The selector owns:

- item count;
- selected index;
- scroll offset;
- focus state;
- whether wraparound is enabled;
- whether the current list is empty.

The selector does not own:

- save-file parsing;
- chapter unlock rules;
- snapshot generation;
- deletion;
- loading;
- world creation.

Parent screens own the action performed on the selected item.

## Input Semantics

Supported semantic inputs:

- `menu.up`: select previous item;
- `menu.down`: select next item;
- `menu.confirm`: activate focused item;
- `menu.back`: return to parent screen;
- `menu.page_up`: jump upward by visible page;
- `menu.page_down`: jump downward by visible page.

Controller mapping:

- D-pad up/down selects previous/next;
- left stick up/down can scroll with repeat delay;
- Cross confirms;
- Circle backs out;
- Options is reserved for pause/settings where applicable.

Keyboard mapping:

- Up/Down selects previous/next;
- PageUp/PageDown jumps by page;
- Enter confirms;
- Escape backs out.

Mouse/touchpad mapping:

- wheel or trackpad vertical scroll changes selected index;
- click focused or visible entry selects/activates according to parent screen
  policy.

## Scrolling Behavior

Selection should snap to entries, not stop between entries.

Slow input moves one entry at a time. Held input repeats after a short delay.

Fast scroll may advance multiple entries, but the final state still snaps to a
single selected index.

The selected item should remain near the vertical center except near the start
or end of a non-wrapping list.

## Empty State

If there are no items:

- the selector displays a stable empty-state row;
- confirm is ignored;
- back remains available;
- parent screen decides whether to offer New World, Back, or another action.

For save browser:

- `Continue`, `Load Save`, and `Delete Save` remain disabled or route to an
  empty save browser according to the starter menu contract.

## Visual Rules

The selector must not be card spam. The focused entry may be a framed row or
panel, but surrounding entries should remain compact.

Use stable dimensions:

- fixed snapshot aspect ratio;
- fixed row height;
- fixed visible item count;
- deterministic fade height;
- no text overflow.

Long titles must truncate or wrap in a controlled area. Date/time must remain
readable and aligned.

## Data Contract

Recommended entry model fields:

```text
id
kind
title
subtitle
snapshot_ref
timestamp
enabled
disabled_reason
```

`id` is the durable selection key. The selected index is view state; the id is
what parent screens use for load/delete/activate.

## Receipt Fields

Receipt proof should be parent-specific but can reuse selector concepts:

```text
selector_visible=true|false
selector_item_count=<integer>
selector_selected_index=<integer>
selector_selected_id=<id-or-none>
selector_selected_title=<title-or-none>
selector_empty=true|false
selector_scroll_direction=up|down|none
selector_confirmed=true|false
```

Save-specific proof may add:

```text
save_browser_visible=true|false
save_browser_selected_title=<title-or-none>
save_browser_selected_timestamp=<timestamp-or-none>
save_browser_snapshot_available=true|false
```

## Test Plan

Unit tests should prove:

- empty list behavior;
- single-item list behavior;
- previous/next selection;
- clamped first/last behavior for non-wrapping lists;
- selected id follows selected index;
- disabled focused item cannot confirm;
- page up/down clamps correctly;
- scroll state remains deterministic.

Smoke tests should prove through receipts:

- save browser opens;
- selected save changes after scroll down;
- selected save changes after scroll up;
- confirm acts on the focused save;
- snapshot availability is reported;
- `window_launch_count=0` for no-window proof.

## Stop Rules

Stop before implementation if the plan requires:

- renderer/Vulkan work;
- save codec/schema changes;
- real window proof;
- generated image assets;
- gameplay/runtime mutation;
- editor automation.

## Coding Method

The selector should be a reusable model and view-model builder. Parent screens
choose what selected items mean. The selector must not be implemented separately
inside each save browser, delete browser, chapter selector, or world preset
screen.

Recommended modules:

```text
src/app/frontend/VerticalFadedSelector.hpp
src/app/frontend/VerticalFadedSelector.cpp
src/app/frontend/VerticalFadedSelectorViewModel.hpp
src/app/frontend/VerticalFadedSelectorViewModel.cpp
```

Core model types:

```text
VerticalSelectorItem
VerticalSelectorState
VerticalSelectorInput
VerticalSelectorResult
VerticalSelectorViewItem
VerticalSelectorViewModel
```

## File Ownership

| File area | Owns | Must not own |
| --- | --- | --- |
| `VerticalFadedSelector.*` | item list, selected index, scroll movement, empty state | save parsing, load/delete actions |
| Parent screen model | selector item payloads and confirm/back meaning | selector scroll math |
| View model builder | opacity/scale/row positions | data loading |
| Receipt builder | selector proof fields | selection decisions |
| `AppShell.cpp` | call parent router only | selector navigation logic |

## Inputs And Outputs

Selector input:

```text
VerticalSelectorState
VerticalSelectorItems
MenuActionFrame
VerticalSelectorConfig
```

Selector output:

```text
VerticalSelectorResult
```

The result should include selected index/id, scroll direction, confirmed,
back_requested, ignored reason, and updated state.

## Long-Term Fit

One selector model should serve save files, delete save, chapters, world
creation presets, and future content lists. This keeps controller, keyboard,
mouse, and automation behavior consistent and keeps future UI polish separate
from data ownership.
