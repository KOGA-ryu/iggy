# Batch 01: Trace Mode

## Goal
Add optional per-frame trace output to `iggy_scenario_toml_runner` so authored scenarios can be debugged frame by frame.

## Current State
The CLI prints stable final output sections: `status:`, `summary:`, and `final_rows:`. Canonical fixtures are CLI-runnable and final rows are golden-tested.

## Slices
1. Inspect `RuntimeGameplayProfileScenarioRunner` / orchestrated runner result shapes and confirm per-frame states are already preserved.
2. Add trace projection using `RuntimeGameplayAsciiSourcePlanFinalDebugRows` over each frame result state.
3. Add CLI flag support:
   - `iggy_scenario_toml_runner <path>`
   - `iggy_scenario_toml_runner --trace <path>`
4. Emit a stable `frames:` section only in trace mode, with frame id, accepted command count, pickup count, interaction-changed flag, NPC moved count, and rows.
5. Add trace smoke tests for multi-frame movement and mixed mini scenario.
6. Add tiny fixture README/API note if stale.

## Verification
Focused CLI runner tests after slices 2-5. Full verification at batch end.

## Hard Stops
No gameplay semantics, no new runner/report framework, no JSON output, no directory scanning.

## Expected Result
Normal CLI output remains unchanged. `--trace` adds stable per-frame rows and counts.
