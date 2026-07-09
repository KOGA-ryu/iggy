# File Spec

Files: `src/app/iggy3d/gameplay/ControllerResetActions.hpp`, `src/app/iggy3d/gameplay/ControllerResetActions.cpp`

Verified at: `74fcbc90`

## Owns

- Product reset action submission.
- Session baseline reset handoff and product command proof fields for reset.

## Does Not Own

- Baseline creation, session reset internals, input phase scheduling, save/load recovery, or rendering.

## Reads

- `Session`, product window state, source string, and `SessionResetResult`.

## Writes / Mutates

- Calls `Session::resetToBaseline(...)`.
- Clears target/outcome proof.
- Writes gameplay input source and reset command kind/submitted/accepted/status fields.

## Calls Out To / Wires Out To

- `Session::resetToBaseline(...)`.
- `clearProductTargetProof(...)` and `clearProductOutcomeProof(...)`.
- Called by action phases on reset intent.

## Called By / Entry Points

- `submitProductReset(...)`.
- `ControllerActionPhases.*`.
- Grep proof: `rg -n "submitProductReset|resetToBaseline|gameplayCommand.kind = \"reset\"" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Reset always marks command submitted.
- Reset accepted status mirrors `SessionResetResult::reset`.
- Target and outcome proof are cleared before reset command proof is published.
- This file does not decide whether reset input is allowed; action phases own scheduling.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "reset|resetToBaseline|gameplayCommand.kind" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.*` unless reset semantics change.
- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless reset scheduling changes.
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.*` unless cleared proof fields change.

## Update When

- Reset command proof fields, target/outcome clearing, reset acceptance mapping, or session reset handoff changes.

## Do Not Update When

- Only baseline generation, input binding, save/load behavior, or UI feedback text changes.
