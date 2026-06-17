# 12 Authoring Test Support Extraction

Status: complete.

Goal: extract test-only helpers for repeated authoring fixture paths, package
paths, CLI invocation, final-row blocks, and common output assertions.

Likely files:
- New helper or helpers under `engine/tests/support`.
- Selected tests only:
  `iggy_scenario_toml_runner_tests.cpp`,
  `iggy_scenario_toml_runner_manifest_sweep_tests.cpp`,
  `runtime_gameplay_toml_scenario_package_facade_tests.cpp`, and
  `runtime_gameplay_authoring_preview_model_tests.cpp`.

Guardrails:
- Preserve every assertion.
- Preserve CLI output and exit-code contracts.
- No production code.
- No fixture renames.

Verification:
- Focused build/run for touched test targets.
- Full CTest at batch end if test support changes land.

Result:
- Added `engine/tests/support/AuthoringTestSupport.hpp` for test-only authoring
  fixture paths, package paths, CLI invocation, final-row blocks, fixture text,
  string containment, and common CLI output containment assertions.
- Updated selected authoring CLI, manifest sweep, package facade, and preview
  model tests to use the shared helper.
- Preserved existing assertions, expected output fragments, exit-code checks,
  fixture names, and production code.
