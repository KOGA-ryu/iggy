# File Spec

Files: `src/app/iggy3d/automation/AutomationSystem.hpp`, `src/app/iggy3d/automation/AutomationSystem.cpp`

Verified at: `67587af2`

## Owns

- System/settings automation handler for frontend execute, settings apply, settings restore defaults, settings back, return to title, pause, and quit command ids.
- Bool parsing and false-value ignore behavior for system-level automation commands.
- Bridging system commands to menu input routing or app lifecycle callbacks.

## Does Not Own

- Settings model persistence.
- Full frontend action handling.
- Pause menu rendering or input routing internals.
- App lifetime outside the provided return-to-title and quit callbacks.

## Reads

- `ProductAutomationCommandDispatchSpec.commandId` and command value.
- `FrontendState` and `ProductAppWindowState.automationControl`.
- Current owner callback.

## Writes / Mutates

- Mutates `FrontendState.status` for settings apply/defaults command results.
- Mutates automation-control last command result through `markAutomationApplied(...)`.
- Invokes lifecycle callbacks that may mutate active session or close-request state outside this file.

## Calls Out To / Wires Out To

- `resolveProductAutomationBool(...)`.
- `markAutomationApplied(...)`.
- `routeInput(...)` for menu confirm, menu back, and system pause.
- `returnToTitle(...)` callback.
- `requestQuit(...)` callback.

## Called By / Entry Points

- `AutomationDispatch.cpp` calls `applyProductSystemAutomationCommand(...)` after save-browser handling.
- Focused proof: `rg -n "applyProductSystemAutomationCommand|frontend.return_to_title|system.pause|system.quit|settings.restore_defaults" src/app tests/unit`.

## Invariants

- Invalid bool values fail with `invalid_value`.
- False bool values for routed/lifecycle commands are ignored, not executed.
- Return-to-title and quit behavior must stay behind callbacks owned by the app dispatcher.
- Settings apply/defaults only set frontend status and automation proof; they do not persist settings here.

## Tests / Proof Commands

- `rg -n "settings.apply|settings.restore_defaults|frontend.return_to_title|system.pause|system.quit" tests/unit/product_automation_command_registry_tests.cpp tests/smoke`.
- `rg -n "applyProductSystemAutomationCommand" src/app/iggy3d/automation src/app/iggy3d`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/Transitions.*` unless return-to-title behavior changes.
- `src/app/frontend/SettingsMenu.*` unless settings status semantics change.
- `src/app/iggy3d/window/Loop.*` unless close-request plumbing changes.

## Update When

- System automation command ids, lifecycle callbacks, settings status writes, or routed system input behavior changes.

## Do Not Update When

- Only menu rendering, settings UI layout, or app shutdown implementation changes behind the same callbacks.
