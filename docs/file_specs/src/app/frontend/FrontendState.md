# File Spec

Files: `src/app/frontend/FrontendState.hpp`, `src/app/frontend/FrontendState.cpp`

Verified at: `6c79462b`

## Owns

- Frontend state enums: `FrontendScreen`, `FrontendAction`, `FrontendDevToolsCategory`, and `FrontendSaveBrowserMode`.
- `FrontendState` flags for screen, child screen, selected/disabled action, save browser mode, boot/package/save readiness, launch and title-return requests, input ownership, and status text.
- Stable string names for frontend screens, actions, dev tool categories, and save browser mode.
- Canonical starter, pause, and dev-tools action ordering.
- Simple state transitions for boot completion, gameplay entry, pause open, dev overlay open, overlay close, and gameplay-input blocking.

## Does Not Own

- Product-specific routing from frontend state to active surface/input owner.
- Stateful menu command execution.
- Save catalog scanning or mutation.
- Draw-list generation, hit testing, window input polling, or renderer presentation.

## Reads

- Caller-supplied `FrontendState` values.
- Package-ready and save-scan-ready booleans passed into `completeFrontendBoot(...)`.
- Launch action passed into `enterFrontendGameplay(...)`.

## Writes / Mutates

- Caller-owned `FrontendState` references passed into transition helpers.
- No global state except static action/category order tables.

## Calls Out To / Wires Out To

- Does not call app/runtime subsystems.
- Provides enum names and action ordering consumed by frontend models, menu routing, draw lists, receipts, and tests.

## Called By / Entry Points

- `StarterScreen.cpp`, `PauseMenu.cpp`, menu action handlers, menu draw lists, hit testing, and product window/input tests consume action order and state helpers.
- Grep proof: `rg -n "starterActionOrder|pauseActionOrder|completeFrontendBoot|enterFrontendGameplay|frontendBlocksGameplayInput" src tests cmake`.

## Invariants

- Starter and pause action order are the canonical row order for routing and draw-list consumers.
- `enterFrontendGameplay(...)` clears frontend input ownership.
- Pause and dev overlay transitions keep `childScreen` anchored to gameplay.
- String-name functions must stay stable enough for receipts, semantic IDs, and tests.
- This layer remains product-agnostic and does not gain save, window, renderer, or runtime dependencies.

## Tests / Proof Commands

- `rg -n "frontend_state_tests|starter_screen_tests|pause_menu_tests" cmake tests`.
- `rg -n "frontendScreenName|frontendActionName|starterActionOrder|pauseActionOrder" src tests`.

## Nearby Files Usually Not Touched

- `src/app/frontend/StarterScreen.*` unless starter route shape changes.
- `src/app/frontend/PauseMenu.*` unless pause route shape changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless active-surface mapping changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless command behavior changes.

## Update When

- Frontend enums, action order, state fields, string names, or transition helper behavior change.

## Do Not Update When

- Only product action handling changes after the same frontend action is selected.
- Only visual layout or renderer presentation changes.
