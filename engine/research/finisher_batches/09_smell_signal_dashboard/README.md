# 09 Smell Signal Dashboard

Status: complete.

Goal: add a lightweight documented command set for tracking repo smells over
time.

Slices:
- Record greps/counts from smell audit.
- Do not add CI wiring yet.

Verification:
- Docs diff review.

Baseline date:
- 2026-06-17 on `codex/finisher-roadmap`.

Purpose:
- These commands are trend signals, not proof of a bug.
- Keep them manual for now; do not wire CI until a later packet defines
  thresholds and ownership.

Command set:

```sh
rg -n "TODO|FIXME|HACK|XXX" engine/src engine/tests engine/apps engine/research -S | wc -l
rg --files engine/src engine/tests engine/apps | rg 'Report|Reporter|Result|Runner|Facade|Adapter|Converter|Validator' | wc -l
rg --files engine/src/runtime | rg 'Report|Reporter' | wc -l
rg -n "RuntimeGameplay.*Report|Runtime.*Reporter|RunnerReport|FrameReport" engine/src/runtime engine/tests -S | wc -l
rg -n "std::filesystem" engine/src engine/apps engine/tests -S | wc -l
rg -n "recursive_directory_iterator|directory_iterator" engine/src engine/apps engine/tests -S | wc -l
rg -n "IggyScenarioTomlRunner|iggy_scenario_toml_runner" engine/src engine/apps engine/tests engine/research -S | wc -l
rg -n "const char \*ToString\(" engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp | wc -l
rg -n "std::cerr << \".*_issue:|std::cout << \"(summary|expectation|final_rows|frames):" engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp -S | wc -l
rg --files engine/tests/fixtures/runtime/ascii_source_plan | rg '\\.toml$' | wc -l
rg -n "FixturePath\(\".*\.toml\"\)" engine/tests -S | wc -l
```

Current counts:
- `TODO|FIXME|HACK|XXX`: 0
- Broad helper-family filenames: 97
- Runtime `Report|Reporter` filenames: 25
- Runtime/test report-family references: 521
- `std::filesystem` references in source/apps/tests: 314
- Directory iterator references in source/apps/tests: 2
- CLI runner name references: 28
- CLI-local `ToString` overload count: 14
- CLI-local output section/issue print sites: 13
- ASCII source-plan TOML fixture files: 15
- Hardcoded fixture TOML path call sites in tests: 31

Interpretation:
- Report/helper counts should trend down only after facade/projection contracts
  are stable; do not chase this during semantic work.
- Directory iterator count should stay near zero because package/directory
  scanning is out of scope.
- CLI-local `ToString` and print-site counts indicate extraction pressure for
  diagnostic/output projection, but output must remain locked before changes.
- Fixture file and hardcoded path counts should drop only after the test-owned
  fixture manifest lands.

Result:
- Smell dashboard documented as manual commands only.
- No CI wiring, thresholds, source changes, or behavior changes.
