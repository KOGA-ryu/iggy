# 05 Authoring Projection Design Gate

Status: complete.

Goal: design a small behavior-preserving projection helper for CLI summary,
final rows, and expectation output without implementing until collision risk is
low.

Slices:
- Identify exact existing data sources.
- Propose helper API.
- List tests.
- Return to planner if implementation would touch active builder files.

Verification:
- Read-only/docs unless approved.

Collision check:
- Builder status showed active edits in:
  `engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp`,
  `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.cpp`, and
  `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.hpp`.
- Implementation would touch the same files, so this packet remains a design
  gate only.

Existing data sources:
- `RuntimeGameplayTomlScenarioFacadeResult::path` provides the printed
  `source_path`.
- `RuntimeGameplayTomlScenarioFacadeResult::run` provides run summary facts:
  `frameCount`, `scenario.runner.acceptedCommandCount`,
  `scenario.runner.pickedUpCount`, `scenario.runner.interactionChanged`,
  `npcMovedCount`, and `npcBlockedMovementCount`.
- `RuntimeGameplayTomlScenarioFacadeResult::finalRows` is already projected by
  `finalDebugRowsForAsciiSourcePlan(plan, state)` inside the facade.
- `RuntimeGameplayTomlScenarioFacadeResult::expectationComparison` provides the
  current expectation presence, aggregate match result, and per-field match
  flags for final rows, frame count, accepted commands, pickups, interaction
  changed, and NPC moved count.
- `RuntimeGameplayTomlScenarioTraceFrame` already projects trace frame rows and
  per-frame counts, but trace is outside this packet's first helper unless a
  later packet chooses to include it.

Proposed helper shape:
- Add a narrow runtime projection header/source after the facade settles, for
  example `RuntimeGameplayTomlScenarioRunProjection`.
- Keep it pure: no IO, no filesystem reads, no CLI argument parsing, no new
  validation, no execution.
- Input: a completed `RuntimeGameplayTomlScenarioFacadeResult` with
  `status == Ran`.
- Output structs:
  - `RuntimeGameplayTomlScenarioSummaryProjection`
  - `RuntimeGameplayTomlScenarioExpectationProjection`
  - `RuntimeGameplayTomlScenarioRunProjection`
- Summary fields:
  - `sourcePath`
  - `frameCount`
  - `acceptedCommandCount`
  - `pickedUpCount`
  - `interactionChanged`
  - `npcMovedCount`
  - `npcBlockedMovementCount`
- Expectation fields:
  - `present`
  - `matched`
  - ordered checks as `{ name, matched }`, preserving current CLI order:
    `final_rows`, `frame_count`, `accepted_command_count`, `picked_up_count`,
    `interaction_changed`, `npc_moved_count`.
- Run projection fields:
  - `summary`
  - `expectation`
  - `finalRows`

API sketch:

```cpp
namespace iggy::runtime {

struct RuntimeGameplayTomlScenarioExpectationCheckProjection {
	std::string name;
	bool matched = true;
};

struct RuntimeGameplayTomlScenarioExpectationProjection {
	bool present = false;
	bool matched = true;
	std::vector<RuntimeGameplayTomlScenarioExpectationCheckProjection> checks;
};

struct RuntimeGameplayTomlScenarioSummaryProjection {
	std::string sourcePath;
	std::size_t frameCount = 0;
	std::size_t acceptedCommandCount = 0;
	std::size_t pickedUpCount = 0;
	bool interactionChanged = false;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
};

struct RuntimeGameplayTomlScenarioRunProjection {
	RuntimeGameplayTomlScenarioSummaryProjection summary;
	RuntimeGameplayTomlScenarioExpectationProjection expectation;
	std::vector<std::string> finalRows;
};

[[nodiscard]] RuntimeGameplayTomlScenarioRunProjection
projectTomlScenarioRun(
	const RuntimeGameplayTomlScenarioFacadeResult &result);

} // namespace iggy::runtime
```

Implementation notes:
- Do not add `npc_blocked_movement_count` to expectation comparison in this
  helper; that would be a contract/semantics expansion.
- Do not move text rendering into the helper at first. Keep the helper as data
  projection, then update CLI printing to read these fields without changing
  output.
- Keep `finalRows` as a copied `std::vector<std::string>` from the facade result
  rather than recomputing it.
- Treat non-`Ran` input as an empty/default projection or add a focused
  precondition test; choose one policy before implementation.

First tests to add when implementation is safe:
- Projection from `mixed_mini_scenario.toml` preserves all summary counts and
  final rows.
- Projection from `npc_blocked_guard_room.toml` preserves
  `npcBlockedMovementCount`.
- Projection from a fixture with no expectations reports `present = false` and
  no checks.
- Projection from matching expectations emits checks in the current CLI order.
- Projection from a mismatched expectation preserves `matched = false` and the
  mismatched field.
- Existing `iggy_scenario_toml_runner_tests.cpp` remains green and continues to
  lock textual output.

Result:
- Design gate completed as docs only.
- Implementation deferred because builder is actively editing the likely target
  files.
