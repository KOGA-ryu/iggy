# File Spec

File: `src/app/iggy3d/gameplay/CommandState.hpp`

Verified at: `0e08703f`

## Owns

- `ProductGameplayCommandState`, the app-side mirror packet for the most recent product gameplay command.
- Submitted/accepted/kind/status proof fields used by controller, automation, receipts, debug/presentation, and tests.

## Does Not Own

- Command admission, runtime command logs, command construction, gameplay action routing, session tick advancement, or rejection reason derivation.

## Reads

- No runtime data directly; this header defines the packet shape only.

## Writes / Mutates

- No functions mutate state here.
- Writers include controller action paths and scripted gameplay paths that assign `window.gameplay.gameplayCommand`.

## Calls Out To / Wires Out To

- No calls.
- Packet is wired through `GameplayStore::gameplayCommand`.

## Called By / Entry Points

- `ControllerCommandExecution.*` sets submitted/kind/accepted/status around `Session::submitCommand(...)`.
- `ControllerMoveActions.*`, `ControllerTargetActions.*`, and `ControllerResetActions.*` assign command proof for action-specific failures or resets.
- Grep proof: `rg -n "ProductGameplayCommandState|gameplayCommand|submitProductGameplayCommand" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- `submitted=false`, `accepted=false`, `kind="none"`, and `status="not_requested"` represent no command.
- `accepted` mirrors command acceptance, not downstream outcome success.
- `kind` and `status` are receipt/debug strings and must remain stable unless all receipt/smoke expectations are updated.
- Packet remains app observability; runtime command history stays in runtime/session state.

## Tests / Proof Commands

- `rg -n "gameplayCommand\\.accepted|gameplayCommand\\.submitted|gameplayCommand\\.status" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_gameplay_feedback_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.
- `rg -n "GameplayRuntimeMovementFields|gameplay_command|gameplayCommand" src/app/iggy3d/receipt src/app/iggy3d/automation tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.*` unless command API semantics change.
- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless command proof writing changes.
- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.*` unless receipt field names change.

## Update When

- Command proof fields, default command status, or writer/reader ownership changes.

## Do Not Update When

- Only an individual action's gameplay behavior changes while it still writes the same command packet contract.
