# File Spec

Files: `src/app/input/InputActionRegistry.hpp`, `src/app/input/InputActionRegistry.cpp`

Verified at: `6b1418a6`

## Owns

- Descriptor metadata for each `InputAction`.
- Action name, group, feature, owner, pre-menu handling flag, and keep-gameplay-active flag.
- Dense descriptor table indexed by `InputAction` byte value.

## Does Not Own

- The `InputAction` enum values, physical input bindings, action-state recording, menu/gameplay routing, product controller action map rows, or command execution.

## Reads

- `InputAction` and `InputActionGroup` identities.
- Static descriptor table built in this file.

## Writes / Mutates

- No runtime mutation; descriptors are constexpr/static lookup data.

## Calls Out To / Wires Out To

- `InputAction.cpp` delegates action name and group lookup here.
- Window/input paths read feature, owner, pre-menu, and keep-gameplay flags for input ordering and proof.

## Called By / Entry Points

- `inputActionDescriptor(...)`, `inputActionFeatureName(...)`, `inputActionOwnerName(...)`, `inputActionHandledBeforeMenu(...)`, and `inputActionKeepsGameplayActive(...)`.
- Grep proof: `rg -n "InputActionDescriptor|inputActionDescriptor|inputActionFeatureName|inputActionOwnerName|inputActionHandledBeforeMenu|inputActionKeepsGameplayActive" src/app tests/unit`.

## Invariants

- Every meaningful `InputAction` must have a descriptor row.
- Descriptor strings are receipt/debug-facing and should remain stable unless callers and tests update together.
- Pre-menu and keep-gameplay flags are input routing policy, not gameplay command behavior.
- Descriptor table indexing depends on `InputAction` fitting in an unsigned byte.

## Tests / Proof Commands

- `rg -n "product_window_input_frame_tests|menu_input_tests|product_controller_action_routing_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "inputActionFeatureName|inputActionOwnerName|inputActionHandledBeforeMenu|inputActionKeepsGameplayActive" tests/unit/product_window_input_frame_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/input/InputAction.*` unless action identities or group APIs change.
- `src/app/input/InputBindings.*` unless physical bindings change.
- `src/app/iggy3d/input/ControllerActionMap.*` unless product controller mappings change.

## Update When

- Descriptor rows, descriptor fields, pre-menu/keep-gameplay semantics, or descriptor accessors change.

## Do Not Update When

- Only physical bindings or command handlers change without changing descriptor metadata.
