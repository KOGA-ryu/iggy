# 08 Test Ceremony Trim Audit

Status: complete.

Goal: identify C++ fixture/test ceremony that is now redundant due to canonical
CLI golden tests.

Slices:
- Inventory candidates.
- Classify keep/remove-later.
- Do not remove behavior in this gate.

Verification:
- Read-only/docs.

Inventory source files:
- `engine/research/authoring_batches/13_fixture_ceremony_cleanup/README.md`
- `engine/tests/iggy_scenario_toml_runner_tests.cpp`
- `engine/tests/runtime_gameplay_ascii_source_plan_toml_file_reader_tests.cpp`
- `engine/tests/runtime_gameplay_toml_scenario_facade_tests.cpp`

Keep:
- `iggy_scenario_toml_runner_tests.cpp`: this is the current output/exit-code
  contract owner and should not be trimmed for ceremony until snapshot or
  manifest replacements exist.
- File-reader temp path tests:
  `TestEmptyPathDoesNotCallTextReader`, `TestMissingPathReportsMissingFile`,
  `TestDirectoryReportsNonRegularFile`, `TestValidFileReadsAndParses`, and
  `TestInvalidTomlFilePreservesNestedDiagnostics`. These cover file IO behavior
  that CLI goldens do not isolate.
- Fixture diagnostics tests:
  `TestCorruptFixturePreservesNestedParserDiagnostics` and
  `TestSemanticInvalidFixturePreservesNestedSourceValidation`. These lock nested
  parser/source diagnostics below the CLI's first-issue text.
- Explicit config override tests:
  `TestValidFixtureConvertsToValidatedProfileScenario`,
  `TestValidFixtureFeedsAuthoringAdapterThroughParsedSourcePlan`, and
  `TestValidFixtureRunsScenarioAndRendersFinalDebugRows`. `valid_guard_room.toml`
  intentionally needs C++ converter config and is not a canonical CLI success
  fixture.
- Facade tests in `runtime_gameplay_toml_scenario_facade_tests.cpp`. They assert
  facade status/data availability rather than CLI text; revisit only after a
  projection helper exists.

Keep, but simplify later:
- `TestMovingFixtureRunsScenarioAndMovesNpcFromAuthoredControl`: keep parser and
  converter assertions for authored control promotion; visible movement outcome
  is already covered by CLI goldens.
- `TestMultiFrameFixtureRunsScenarioAndMovesNpcAcrossFrames`: keep authored
  frame ordering/conversion assertions; CLI trace already covers visible rows
  and per-frame movement counts.
- `TestPlayerAndGuardFixtureRunsSharedFrameThroughScenario`: keep shared-frame
  conversion assertions; CLI goldens cover the visible final row and summary.
- `TestSelfContainedFixtureRunsWithEmptyConverterConfig`: keep empty-config and
  TOML profile-catalog assertions; final state/row assertions overlap with
  canonical CLI coverage and can be reduced once a manifest exists.
- `TestPlayerInteractionFixtureRunsScenarioAndTogglesTarget`: keep target/effect
  promotion and frame report assertions; final row/player/NPC idle assertions
  are candidates for removal.
- `TestPlayerPickupFixtureRunsScenarioAndPicksUpItem`: keep item drop,
  interaction target/effect, inventory stack, and consumed-drop assertions;
  repeated final rows and idle NPC/player details are candidates for removal.
- `TestLockedDoorKeyFixturesGateDoorToggleOnInventory`: keep required-item
  propagation and inventory/door state assertions; CLI trace already covers
  accepted counts, pickup count, interaction changed, and final rows.

Remove-later candidates:
- Duplicate final ASCII row checks in lower-level vertical tests when the same
  fixture is already covered by `TestCanonicalFixtures`.
- Duplicate run summary count checks in lower-level vertical tests when they are
  not asserting a converter- or report-specific contract.
- Repeated manual `read -> packet -> adapter -> run -> finalRows` setup once a
  test-only fixture manifest/helper exists.
- Repeated source path/name lookup helpers after a manifest helper centralizes
  fixture paths.

Do not remove:
- Parser, source-plan validator, conversion, explicit config, diagnostic, and
  inventory/interaction internal assertions that CLI textual goldens cannot see.
- Any tests around `valid_guard_room.toml` conversion-with-config behavior.
- Any CLI output or exit-code assertion without an explicit output-contract
  replacement.

Result:
- Audit completed as docs only.
- No tests, fixtures, behavior, output, or exit codes changed.
