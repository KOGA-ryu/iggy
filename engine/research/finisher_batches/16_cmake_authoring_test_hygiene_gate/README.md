# 16 CMake Authoring Test Hygiene Gate

Status: complete.

Goal: assess whether authoring test registration and fixture compile
definitions can be simplified without renaming tests or changing labels.

Scope:
- Gate/design unless a very small CMake helper is obvious.

Guardrails:
- Do not rename tests.
- Do not change labels.
- Do not alter fixture paths or CLI output contracts.

Verification:
- CMake configure and focused tests if CMake changes.
- Docs diff review if design-only.

Audit:
- Test registration uses `iggy_add_test` in `engine/CMakeLists.txt`, which
  already preserves executable names, CTest names, and inferred labels.
- Authoring fixture compile definitions are repeated in
  `engine/cmake/iggy_runtime_tests.cmake` for:
  - `runtime_gameplay_authoring_diagnostics_tests`
  - `runtime_gameplay_authoring_preview_model_tests`
  - `runtime_gameplay_authoring_fixture_budget_tests`
  - `runtime_gameplay_ascii_source_plan_toml_file_reader_tests`
  - `runtime_gameplay_ascii_source_plan_toml_schema_snapshot_tests`
  - `iggy_scenario_toml_runner_tests`
  - `iggy_scenario_toml_runner_manifest_sweep_tests`
  - `runtime_gameplay_toml_scenario_facade_tests`
  - `runtime_gameplay_toml_scenario_package_facade_tests`
- The CLI runner tests also repeat `add_dependencies(... iggy_scenario_toml_runner)`
  and `IGGY_SCENARIO_TOML_RUNNER_PATH="$<TARGET_FILE:iggy_scenario_toml_runner>"`.
- Package-aware tests repeat
  `IGGY_TEST_PACKAGE_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/runtime/ascii_source_plan_packages"`.

No-change decision:
- Do not change CMake in this packet. The current duplication is visible but
  low risk, while a helper would touch test registration infrastructure and
  require configure plus focused test verification.
- Do not rename tests or add label arguments. Existing labels come from
  `IGGY_TEST_DEFAULT_LABELS` plus name inference, and the packet guardrail is
  to preserve those labels.
- Do not change compile definition names or fixture path values. The C++ tests
  and CLI output-contract tests depend on these exact macros.

Smallest future helper candidate:
- Add a local CMake helper in `engine/cmake/iggy_runtime_tests.cmake`, not the
  root `iggy_add_test`, for authoring fixture paths only:
  `iggy_target_authoring_fixtures(<target> [PACKAGES])`.
- Add a second local helper only if needed:
  `iggy_target_scenario_toml_runner(<target>)`, which adds the executable
  dependency and `IGGY_SCENARIO_TOML_RUNNER_PATH` compile definition.
- Apply helpers incrementally to one or two nearby tests first, then expand
  after confirming CTest names and labels are unchanged.

Required verification for future CMake change:
- `cmake -S engine -B engine/build`
- `ctest --test-dir engine/build -N` before/after comparison for touched test
  names and labels.
- Focused build/tests for all touched authoring and CLI targets.
