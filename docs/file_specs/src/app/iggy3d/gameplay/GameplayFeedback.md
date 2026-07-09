# File Spec

Files: `src/app/iggy3d/gameplay/GameplayFeedback.hpp`, `src/app/iggy3d/gameplay/GameplayFeedback.cpp`

Verified at: `08813cf2`

## Owns

- Product gameplay feedback view model built from gameplay proof mirrors.
- Feedback line construction for target, reach, command, result, and rejection rows.
- Tone selection for gameplay feedback statuses.

## Does Not Own

- Gameplay command execution, target/outcome proof generation, HUD surface visibility policy, SDL/Vulkan drawing, receipt serialization, or input routing.

## Reads

- `ProductAppWindowState::gameplay` active state, command proof, target proof, reach gate, rejection reason, attack/interaction execution flags, and reset command proof.

## Writes / Mutates

- Returns a `GameplayFeedback` packet containing visibility flags, status strings, and `GameplayFeedbackLine` rows.
- Does not mutate `ProductAppWindowState`, runtime session state, render frames, or receipts.

## Calls Out To / Wires Out To

- No runtime or renderer calls; this file is a product packet builder.
- Its output is consumed by projection, receipt, debug HUD, SDL view, Vulkan frame presenter, and render bridge paths.

## Called By / Entry Points

- `buildGameplayFeedback(...)`.
- Called from receipt building and gameplay projection refresh.
- Grep proof: `rg -n "GameplayFeedback|buildGameplayFeedback|GameplayFeedbackLine" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- `visible` follows `window.gameplay.gameplayActive`; downstream surfaces may further hide rows.
- Target feedback becomes relevant for discovered targets or attempted target commands.
- Command feedback is visible for submitted commands or non-default command status.
- Result status prefers executed attack, executed interaction, and accepted reset before falling back to command status.
- Rejection line is only visible when rejection reason is not `none`.

## Tests / Proof Commands

- `rg -n "product_gameplay_feedback_tests|product_vulkan_room_frame_tests|product_render_bridge_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "buildGameplayFeedback|GameplayFeedbackLine|feedback.lines|gameplay feedback" tests/unit/product_gameplay_feedback_tests.cpp tests/unit/product_vulkan_room_frame_tests.cpp tests/unit/product_render_bridge_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.*` unless proof field meanings change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless visibility policy changes.
- `src/app/iggy3d/window/FramePresenter.*` and `src/app/iggy3d/view/*` unless drawing behavior changes.

## Update When

- Feedback statuses, tone mapping, row composition, or source proof fields used by `buildGameplayFeedback(...)` change.

## Do Not Update When

- Only rendering coordinates, font/color choices, receipt field order, or command execution internals change without changing the feedback packet contract.
