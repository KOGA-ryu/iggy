# 04 CLI Output Contract Inventory

Status: complete.

Goal: inventory the current CLI output and exit-code contract before facade
extraction changes it.

Slices:
- Read CLI implementation and tests.
- Document modes, sections, exit codes, and locked tests in the finisher bucket
  packet or a roadmap appendix.
- Make no code changes unless they are docs only.

Verification:
- Docs diff review.
- `git diff --check`

Inventory source files:
- `engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp`
- `engine/tests/iggy_scenario_toml_runner_tests.cpp`
- `engine/tests/runtime_gameplay_toml_scenario_facade_tests.cpp`
- `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.hpp`
- `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.cpp`

Invocation contract:
- Usage text: `usage: iggy_scenario_toml_runner [--trace] [--check] [--lint] <path>`
- Recognized flags: `--trace`, `--check`, `--lint`.
- Flags may appear anywhere in the argument list because the parser checks flags
  before assigning the first non-flag path.
- The first non-flag argument is the path. A second non-flag argument is a usage
  error.
- Repeating a flag is accepted and has no additional effect.
- An unknown first argument is treated as the path, not as an unknown option.

Mode contract:
- Default run: read, convert, run, print status, summary, expectation comparison,
  and final rows.
- `--trace`: default run plus per-frame trace rows before the expectation block.
- `--check`: default run, then exit 5 if expectations are missing or mismatched.
  Matching expectations exit 0.
- `--lint`: read, convert, validate, and do not run. Successful lint prints only
  status and summary validation fields. It does not print final rows,
  expectations, accepted command counts, or trace frames.
- `--lint --trace`: trace capture is requested in config but lint returns before
  running, so output remains lint-only.
- `--lint --check`: check is ignored by the lint branch; lint status decides the
  exit code.

Output streams:
- Usage and failure diagnostics are currently printed to `stderr`.
- Successful run/lint output is currently printed to `stdout`.
- CLI tests merge both streams with `2>&1`, so they lock text presence and exit
  code but not stream placement.

Successful default/trace/check run sections:
- `status:`
- `result: ok`
- `summary:`
- `source_path: <path>`
- `frame_count: <count>`
- `accepted_command_count: <count>`
- `picked_up_count: <count>`
- `interaction_changed: true|false`
- `npc_moved_count: <count>`
- `npc_blocked_movement_count: <count>`
- Optional `frames:` when `--trace` is present.
- `expectation:`
- `present: true|false`
- `result: matched|mismatched|not_provided`
- Optional per-field expectation lines:
  `final_rows`, `frame_count`, `accepted_command_count`, `picked_up_count`,
  `interaction_changed`, `npc_moved_count`.
- `final_rows:` followed by final ASCII rows.

Trace frame section:
- `frame_index: <index>`
- `frame_id: <id>` or `frame_id: <none>`
- `accepted_command_count: <count>`
- `picked_up_count: <count>`
- `interaction_changed: true|false`
- `npc_moved_count: <count>`
- `rows:` followed by frame ASCII rows.

Successful lint section:
- `status:`
- `result: lint_ok`
- `summary:`
- `source_path: <path>`
- `frame_count: <count>`
- `adapter_status: converted`
- `profile_status: valid`

Failure sections and exit codes:
- Exit 1, usage error:
  `status:`, `result: usage_error`, usage line.
- Exit 2, read failure:
  `status:`, `result: read_failed`, `read_status`, `toml_status`,
  `issue_count`, then first `file_issue`, first `toml_issue`, and possibly
  `source_issue`.
- Exit 3, conversion failure:
  `status:`, `result: conversion_failed`, `adapter_status`,
  `source_plan_status`, `issue_count`, then first `adapter_issue`,
  `conversion_issue`, and possibly nested `source_issue`, `profile_issue`, or
  `scenario_issue`.
- Exit 4, lint validation failure:
  `status:`, `result: lint_failed`, `adapter_status`, `profile_status`,
  `validation_issue_count`, and first `profile_issue`.
- Exit 4, run validation failure:
  `status:`, `result: run_failed`, `run_status`,
  `validation_issue_count`, and first `profile_issue`.
- Exit 5, check failure:
  normal run output with `expectation:` showing either missing expectations or a
  mismatch.

Currently locked tests:
- `TestNoArgUsage` locks exit 1 and usage text.
- `TestCliFailureDiagnosticsMatrix` locks representative exit 2 and 3
  diagnostics for missing file, corrupt TOML, wrong type, source-plan semantic
  issue, conversion issue, and profile validation issue.
- `TestCanonicalFixtures` locks exit 0, no default `frames:` section, summary
  counters, blocked NPC counter, and final rows for canonical fixtures.
- `TestTraceMultiFrameGuardRoom`, `TestTraceMixedMiniScenario`,
  `TestTraceMixedProgressionRoom`, and `TestTraceLockedDoorKeyRooms` lock trace
  section shape and selected per-frame values.
- `TestExpectationComparisonReportsMatchAndMismatch` locks expectation match and
  mismatch text while preserving exit 0 without `--check`.
- `TestCheckModeUsesExpectationComparisonForExitStatus` locks check-mode exit 0
  for matched expectations and exit 5 for mismatched or missing expectations.
- `TestLintModeValidatesWithoutRunningScenario` locks lint success/failure
  output and verifies lint omits final rows, expectation comparison, and run
  counts.
- `runtime_gameplay_toml_scenario_facade_tests.cpp` locks facade statuses and
  data availability but does not lock CLI text.

Gaps before output-contract refactors:
- Stream placement is not test-locked because CLI tests merge `stdout` and
  `stderr`.
- Full ordering is only partly locked by substring blocks; there are no full
  stdout/stderr golden snapshots.
- `npc_blocked_movement_count` appears in run summary but is not currently an
  expectation comparison field.
- Trace frame output includes `npc_moved_count` but not
  `npc_blocked_movement_count`.
- Unknown-option behavior is implicit and not directly tested.

Result:
- Inventory completed as docs only.
- No CLI output, exit code, source, or test behavior changed.
