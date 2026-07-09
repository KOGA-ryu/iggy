# File Spec

Files: `src/app/iggy3d/automation/AutomationControl.hpp`, `src/app/iggy3d/automation/AutomationControl.cpp`, `src/app/iggy3d/automation/AutomationControlState.hpp`

Verified at: `67587af2`

## Owns

- Automation control-file execution loop for a single product frame/control pass.
- `ProductAutomationControlState` receipt/control fields for request, load, path, status, scope, line count, applied count, last key/action/owner/result.
- Failure handling that preserves loaded status for already-loaded/applied command streams where appropriate.

## Does Not Own

- Parsing command keys beyond calling `readProductAutomationCommands(...)`.
- Command registry or dispatch semantics.
- Per-command behavior.
- Receipt field emission for automation state.

## Reads

- Automation control path.
- Parsed command rows returned by `readProductAutomationCommands(...)`.
- Current owner callback and settings tab reference.

## Writes / Mutates

- Mutates `ProductAppWindowState.automationControl` status, last-command, loaded, and result fields.
- Writes `window.frontendShell.selectedSettingsTab` after command processing.
- Does not mutate source command files.

## Calls Out To / Wires Out To

- `readProductAutomationCommands(...)`.
- Caller-provided `applyCommand(...)` callback.
- Caller-provided `currentOwner(...)` callback for failed-command owner proof.

## Called By / Entry Points

- `AppKernel.cpp` constructs `ProductAutomationControlContext` and calls `applyProductAutomationControl(...)`.
- Focused proof: `rg -n "applyProductAutomationControl|ProductAutomationControlContext|ProductAutomationControlState" src/app tests`.

## Invariants

- Read failure or parse failure stops command processing.
- Command execution stops at the first failed command.
- A failed command records last key/action/owner/result when the handler did not already fill them.
- Empty loaded command files leave last result as `none`.
- This file must not know command-domain behavior; it only coordinates reading and applying rows.

## Tests / Proof Commands

- `rg -n "readProductAutomationCommands|duplicate_key|parse_error|command_failed" tests/unit/product_automation_command_registry_tests.cpp src/app/iggy3d/automation`.
- `rg -n "applyProductAutomationControl" src/app/iggy3d/AppKernel.cpp src/app/iggy3d/automation`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/automation/Automation.*` unless the command file format changes.
- `src/app/iggy3d/automation/AutomationDispatch.*` unless the apply callback shape changes.
- `src/app/iggy3d/receipt/*Automation*Fields.*` unless emitted receipt fields change.

## Update When

- Automation control file execution, failure preservation, state fields, or apply-callback contract changes.

## Do Not Update When

- Only command registry rows or domain handler behavior changes without changing control-loop semantics.
