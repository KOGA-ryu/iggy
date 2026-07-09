# File Spec

Files: `src/app/iggy3d/automation/AutomationDispatch.hpp`, `src/app/iggy3d/automation/AutomationDispatch.cpp`

Verified at: `67587af2`

## Owns

- Top-level product automation command fan-out for app/window execution.
- Construction of `ProductAutomationDispatchContext` and `ProductAutomationAppContext` boundaries.
- Handler order for common, world setup, room editing, gameplay, save-browser, and system automation domains.
- App-context callbacks that route menu input, activate ASCII room preview, route editor input, return to title, request quit, and thread creative app state into starter actions.

## Does Not Own

- Command registry row definitions or primitive parsers.
- Per-domain command behavior inside gameplay, save-browser, room-editing, common/world setup, or system handlers.
- Menu action semantics outside the routing callback.
- Creative document creation, save/load, or facade internals.

## Reads

- `FrontendState`, save bridge result, product options, settings tab/settings, optional active session, world setup draft, product app window state, close flag, and optional `creative::CreativeAppState`.
- Static automation registry and dispatch specs from `Automation.*`.

## Writes / Mutates

- Mutates whatever the selected handler mutates through references in the context.
- Updates `window.automationControl.status` to `unknown_key` when no handler accepts a command.
- App-context callbacks may reset active session on return-to-title or set close requested on quit.

## Calls Out To / Wires Out To

- `applyProductCommonAutomationCommand(...)`.
- `applyProductWorldSetupAutomationCommand(...)`.
- `applyProductRoomEditingAutomationCommand(...)`.
- `applyProductGameplayAutomationCommand(...)`.
- `applyProductSaveBrowserAutomationCommand(...)`.
- `applyProductSystemAutomationCommand(...)`.
- `routeProductOpeningMenuInput(...)`, `activateProductAsciiRoomPreview(...)`, `enterProductGameplayTransition(...)`, `routeInputAction(...)`, and `returnProductToTitleTransition(...)`.

## Called By / Entry Points

- `AppKernel.cpp` calls `applyProductAutomationAppCommand(...)` from the automation control callback.
- `AutomationControl.cpp` receives an `applyCommand` callback that normally resolves here.
- Focused proof: `rg -n "applyProductAutomationAppCommand|applyProductAutomationCommand|ProductAutomationDispatchContext|ProductAutomationAppContext" src/app tests/unit`.

## Invariants

- Handler fan-out order is part of the automation contract; moving a domain earlier can change which command owns ambiguous keys.
- The static registry must be used before resolving command dispatch.
- Unknown keys fail with `unknown_key`, not silent success.
- Creative app state must be passed through app-context menu routing so `CreativeNewWorld` can reach the facade.
- ASCII room activation is the only world setup activation callback this dispatcher owns.

## Tests / Proof Commands

- `rg -n "product_automation_dispatch_tests|product_automation_command_registry_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creativeNewWorldLaunchesThroughAutomationAppContext|automation creative launch status|facade was threaded" tests/unit/product_automation_dispatch_tests.cpp`.
- `rg -n "applyProductAutomationAppCommand" src/app/iggy3d/AppKernel.cpp tests/unit/product_automation_dispatch_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/AppKernel.cpp` unless automation is threaded through app lifetime differently.
- `src/app/iggy3d/menu/InputRouter.*` unless menu routing callback semantics change.
- `src/app/iggy3d/creative/*` unless the creative app-state callback contract changes.
- Domain handler files unless adding or moving command ownership.

## Update When

- Automation fan-out order, context references, app-context callbacks, creative state threading, or unknown-key behavior changes.

## Do Not Update When

- Only a domain handler adds internal behavior behind an already routed command id.
