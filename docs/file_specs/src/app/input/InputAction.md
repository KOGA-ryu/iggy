# File Spec

Files: `src/app/input/InputAction.hpp`, `src/app/input/InputAction.cpp`

Verified at: `9da29fe8`

## Owns

- Cross-app input action identity enum.
- Input action group enum.
- Accessors for action name, action group, and action group name.

## Does Not Own

- Descriptor table contents, physical key/gamepad/mouse bindings, action-state recording, product controller action maps, menu routing, gameplay command execution, or creative editor behavior.

## Reads

- `InputActionRegistry` descriptor data for `inputActionName(...)` and `inputActionGroup(...)`.
- Local switch table for `inputActionGroupName(...)`.

## Writes / Mutates

- No mutation; all functions return action identity metadata.

## Calls Out To / Wires Out To

- Delegates action name/group lookup to `inputActionDescriptor(...)`.
- Used by action state, input bindings, mouse input, product controller routing, menu handlers, gameplay controllers, and editor controllers.

## Called By / Entry Points

- Included by generic input core and product input/menu/gameplay/editor paths.
- Grep proof: `rg -n "InputAction|InputActionGroup|inputActionName|inputActionGroup|inputActionGroupName" src/app tests/unit`.

## Invariants

- Enum values are shared input identities across generic input and product app layers.
- Action names are descriptor-owned strings from `InputActionRegistry.*`.
- Group names must stay stable receipt/debug labels.
- Adding an action requires descriptor/binding/routing/test updates outside this file.

## Tests / Proof Commands

- `rg -n "menu_input_tests|product_controller_action_map_tests|product_controller_action_routing_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "inputActionName|inputActionGroup|InputAction::" src/app tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/input/InputActionRegistry.*` unless descriptor data changes.
- `src/app/input/InputBindings.*` unless physical bindings change.
- `src/app/iggy3d/input/ControllerActionMap.*` unless product controller mapping changes.

## Update When

- Input action enum values, group values, group-name strings, or action metadata accessor contracts change.

## Do Not Update When

- Only a binding or product-specific action map changes without changing action identities.
