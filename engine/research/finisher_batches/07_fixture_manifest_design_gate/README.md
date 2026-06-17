# 07 Fixture Manifest Design Gate

Status: complete.

Goal: design a canonical fixture manifest source of truth.

Slices:
- Inventory canonical and regression fixtures.
- Propose manifest shape.
- Do not add directory scanning.
- Do not add a runtime feature.

Verification:
- Docs diff review.

Inventory source files:
- `engine/tests/fixtures/runtime/ascii_source_plan/README.md`
- `engine/tests/iggy_scenario_toml_runner_tests.cpp`
- `engine/tests/runtime_gameplay_ascii_source_plan_toml_file_reader_tests.cpp`
- `engine/tests/runtime_gameplay_toml_scenario_facade_tests.cpp`
- `engine/research/authoring_batches/32_canonical_fixture_manifest/README.md`

Canonical success fixtures:
- `moving_guard_room.toml`
- `multi_frame_guard_room.toml`
- `player_and_guard_room.toml`
- `player_interacts_guard_room.toml`
- `player_picks_up_item_room.toml`
- `mixed_mini_scenario.toml`
- `mixed_progression_room.toml`
- `locked_door_key_room.toml`
- `locked_door_without_key_room.toml`
- `npc_blocked_guard_room.toml`
- `npc_reservation_guard_room.toml`

Regression-only fixtures:
- `valid_guard_room.toml`
- `self_contained_guard_room.toml`
- `semantic_invalid_guard_room.toml`
- `corrupt_guard_room.toml`

Current duplication:
- Fixture README is the human-readable list.
- `iggy_scenario_toml_runner_tests.cpp` owns the canonical CLI golden table and
  separate failure-mode cases.
- `runtime_gameplay_ascii_source_plan_toml_file_reader_tests.cpp` repeats
  fixture names in parser/converter coverage.
- `runtime_gameplay_toml_scenario_facade_tests.cpp` repeats a smaller facade
  fixture subset.

Proposed manifest location:
- Add a test-only helper under `engine/tests/support/`, for example
  `runtime_ascii_source_plan_fixture_manifest.hpp`.
- Keep it C++ data, not JSON, TOML, or runtime content.
- Do not add directory scanning. Every fixture stays explicitly listed.
- Do not expose the manifest from runtime libraries.

Manifest shape:

```cpp
enum class RuntimeAsciiSourcePlanFixtureCategory {
	CanonicalSuccess,
	RegressionOnly,
};

enum class RuntimeAsciiSourcePlanFixtureExpectedFailure {
	None,
	TomlSyntax,
	SourcePlanValidation,
	Conversion,
};

struct RuntimeAsciiSourcePlanFixtureExpectations {
	std::size_t frameCount = 0;
	std::size_t acceptedCommandCount = 0;
	std::size_t pickedUpCount = 0;
	bool interactionChanged = false;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::vector<std::string> finalRows;
};

struct RuntimeAsciiSourcePlanFixtureManifestEntry {
	const char *filename = "";
	const char *label = "";
	RuntimeAsciiSourcePlanFixtureCategory category =
		RuntimeAsciiSourcePlanFixtureCategory::RegressionOnly;
	RuntimeAsciiSourcePlanFixtureExpectedFailure expectedFailure =
		RuntimeAsciiSourcePlanFixtureExpectedFailure::None;
	bool runDefaultCli = false;
	bool runTraceCli = false;
	bool runCheckCli = false;
	bool runLintCli = false;
	RuntimeAsciiSourcePlanFixtureExpectations expectations;
};
```

Initial policy:
- Canonical success entries must include summary counts and final rows.
- Regression-only entries may omit success expectations and instead declare the
  expected failure lane.
- `self_contained_guard_room.toml` can remain regression-only until a planner
  explicitly promotes or removes it.
- `mixed_progression_room.toml` should be the first canonical `--check` success
  entry because it already embeds `[expect]`.
- Failure cases such as missing files and temp TOML snippets stay outside this
  fixture manifest unless a later negative-fixture catalog packet adds explicit
  entries for them.

First tests to update when implementation is safe:
- Make the CLI canonical fixture loop consume canonical success manifest entries
  instead of a local table.
- Keep the CLI failure diagnostics matrix separate at first.
- Add one manifest sanity test that verifies each canonical success entry has
  non-empty final rows and at least one expected mode.
- Add one test-only helper for `FixturePath(entry.filename)` so tests do not
  infer paths by scanning directories.

Result:
- Design gate completed as docs only.
- No runtime manifest, package manifest, directory scanning, JSON, or TOML
  dependency added.
