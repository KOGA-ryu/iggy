# Starter Menu Contract v1

## Objective

Define the durable starter menu before any more menu code is written.

The starter menu is the first product screen. It owns startup choice, save-slot
selection, child panels, and exit intent. It must not be implemented as another
branch chain inside `AppShell.cpp`.

## Owned Existing Files

- `src/app/frontend/StarterScreen.hpp`
- `src/app/frontend/StarterScreen.cpp`
- `src/app/frontend/SaveBrowser.hpp`
- `src/app/frontend/SaveBrowser.cpp`
- `src/app/frontend/ConfirmDialog.hpp`
- `src/app/frontend/ConfirmDialog.cpp`
- `src/app/frontend/MenuInput.hpp`
- `src/app/frontend/MenuInput.cpp`
- `src/app/frontend/FrontendState.hpp`
- `src/app/frontend/FrontendState.cpp`
- `src/app/iggy3d/ProductMenuTransitions.hpp`
- `src/app/iggy3d/ProductMenuTransitions.cpp`

## New Durable File Boundary

The starter menu should be routed through:

- `src/app/iggy3d/ProductFrontendRouter.hpp`
- `src/app/iggy3d/ProductFrontendRouter.cpp`

`AppShell.cpp` should call the router. It should not own starter menu decision
logic.

## No-Go Files

Do not implement starter menu logic in:

- `src/app/iggy3d/AppShell.cpp`, except for calling the router;
- renderer or Vulkan files;
- runtime/session logic;
- save codec/schema files;
- package visual demo compatibility smokes.

## Starter Rows

The starter menu rows are:

- `Continue`
- `New World`
- `Load Save`
- `Delete Save`
- `Settings`
- `Dev Tools`
- `Exit`

`Continue`, `Load Save`, and `Delete Save` depend on compatible save count.

`Load Save` and `Delete Save` use the reusable vertical faded selector defined
in `docs/plan_bucket/vertical_faded_selector_contract_v1.md`.

`Delete Save` opens the selector in delete mode. The user can scroll up or down
through save files, inspect the save title, snapshot image, and date/time, then
delete the focused save through a confirmation step.

`Exit` closes immediately for now. A later confirmation screen may be added
when the surrounding UX is more mature.

`New World` opens a world-creation setup screen. The first version may only
offer one default preset, but the screen is the durable ownership point for
future world setup choices. If the setup screen needs multiple presets, it
should also use the vertical faded selector.

## Child Panels

The starter menu can open:

- save browser;
- delete-save browser;
- delete confirmation;
- world-creation setup;
- settings;
- dev tools;

Child panels own input above starter rows. Gameplay input remains suppressed
while the starter or any starter child panel is active.

Parent-return rules:

- save browser returns to starter;
- delete-save browser returns to starter;
- delete confirmation returns to delete-save browser on cancel and starter
  after confirmed delete;
- world-creation setup returns to starter on back;
- settings returns to starter;
- dev tools returns to starter.

## Data Ownership

Starter menu owns presentation state:

- selected row;
- disabled row;
- disabled reason;
- child panel;
- confirmation intent;
- status string.

Save discovery is owned by the save bridge/save store. The starter menu reads
save summaries but does not parse save files directly.

Save summaries shown by the starter menu must contain:

- title;
- snapshot image rendered from a saved camera view;
- date/time.

The snapshot image is display data for save selection. The save system owns how
it is stored and loaded; the starter menu owns presenting it and routing
selection/deletion intent.

Snapshot generation and fallback behavior are defined in:

```text
docs/plan_bucket/save_snapshot_contract_v1.md
```

Starter must treat missing or corrupt snapshots as presentation fallback only.
The save remains selectable, deletable, and loadable when the save metadata is
otherwise compatible.

Runtime session creation is owned by product transitions/session creation, not
the starter menu model.

## Control Flow

Starter input resolves in this order:

1. Active child panel.
2. Starter row navigation.
3. Starter row execution.
4. Close/exit intent.

The starter router returns a result containing:

- accepted or ignored;
- next frontend state;
- requested transition;
- close requested;
- status string.

## State Semantics

Starter screen means:

- `frontend_screen=starter`;
- `starter_world_suppressed=true`;
- `gameplay_active=false`;
- `gameplay_input_suppressed=true`;
- `input_owner=starter`, unless a child panel owns input.

Starter settings means:

- `frontend_screen=starter`;
- `frontend_child_screen=settings`;
- `input_owner=settings`;
- gameplay input suppressed.

Starter dev tools means:

- `frontend_screen=starter`;
- `frontend_child_screen=starter_dev_tools`;
- `input_owner=dev_tools`;
- gameplay input suppressed.

## Proof Fields

Starter receipts should prove:

- `frontend_screen`;
- `frontend_child_screen`;
- `frontend_selected_action`;
- `frontend_status`;
- `starter_world_suppressed`;
- `input_owner`;
- `input_action_last`;
- `input_action_accepted`;
- `gameplay_input_suppressed`;
- `starter_row_count`;
- `starter_selected_row`;
- `starter_disabled_row`;
- `starter_disabled_reason`;
- `save_browser_snapshot_available`;
- `save_browser_snapshot_fallback`;
- `save_browser_selected_timestamp`;

New fields should be added only if they prove durable behavior and are not
duplicates of existing frontend fields.

## Test Plan

Unit tests should cover:

- row order;
- disabled save-backed rows with no saves;
- enabled save-backed rows with compatible saves;
- starter settings open/back;
- starter dev tools open/back;
- delete requires confirmation;
- delete browser can select or scroll through save files;
- save browser exposes title, snapshot, and date/time fields;
- exit requests immediate close intent;
- new world opens world-creation setup before session creation;
- child panel ownership suppresses gameplay input.

Smoke tests should remain no-window and receipt-based.

## Acceptance Gate

Acceptance should require:

- focused frontend unit tests;
- product starter/menu smoke;
- product automation menu smoke if automation drives the starter;
- `git diff --check`;
- `window_launch_count=0` for smoke proof.

## Stop Rules

Stop before implementation if the plan requires:

- runtime/session/save schema changes;
- renderer/Vulkan changes;
- package visual demo changes;
- editor automation;
- gameplay movement/action automation;
- a real window proof.

## Coding Method

Starter behavior should be implemented as a row table plus router, not as a
chain of starter-specific `if` branches in `AppShell.cpp`.

Recommended modules:

```text
src/app/frontend/StarterMenuModel.hpp
src/app/frontend/StarterMenuModel.cpp
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
```

Starter row records should include:

```text
id
label
action
enabled
disabled_reason
target_child_panel
requires_compatible_save
```

## File Ownership

| File area | Owns | Must not own |
| --- | --- | --- |
| `StarterMenuModel.*` / `StarterScreen.*` | row order, selected row, disabled reasons, child-panel request | session creation, save parsing, rendering |
| `SaveBrowser.*` | save summary rows and selector payloads | delete execution or runtime load |
| `ConfirmDialog.*` | confirmation state and confirm/cancel result | save deletion implementation |
| `ProductFrontendRouter.*` | action routing and route result | drawing or direct runtime mutation |
| `ProductMenuTransitions.*` | launch/load/return-to-title lifecycle transitions | row navigation |
| `AppShell.cpp` | call router and apply transition result | per-row decisions |

## Inputs And Outputs

Starter router input:

```text
FrontendState
MenuActionFrame
SaveSummarySet
ProductSessionPresence
```

Starter router output:

```text
ProductFrontendRouteResult
```

The route result must include the selected action, accepted/ignored state,
status string, requested child panel, requested product transition, and gameplay
input suppression.

## Long-Term Fit

The starter menu must survive future save browser, world creation, delete save,
settings, dev tools, and chapter selection work. Keeping rows and child panels
in model/router files lets product code add rows without touching app lifecycle
or renderer code.
