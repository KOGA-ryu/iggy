# Batch 49: Manifest Sweep Test Target

Status: complete.

## Goal
Use the canonical fixture manifest from Batch 32 to sweep all canonical fixtures through every supported safe mode.

## Current State
Fixture README and CLI tests list fixtures manually. Batch 32 adds a test-owned manifest.

## Slices
1. Build a table-driven sweep over manifest entries and mode expectations.
2. Run normal mode for every success fixture.
3. Run trace/check/lint only where the fixture declares support.
4. Keep regression-only fixtures in separate negative tests.

## Verification
Focused CLI manifest sweep tests.

## Hard Stops
No runtime directory scanning, CLI discovery command, or package runner behavior.

## Expected Result
Adding a canonical fixture automatically adds intended mode coverage.

## Completed Coverage

- Added `iggy_scenario_toml_runner_manifest_sweep_tests`, a focused CLI test target driven by `CanonicalAuthoringFixtures()`.
- The sweep runs every canonical success fixture through run, trace, and lint modes.
- The sweep runs check mode only for manifest entries that declare embedded expectations.
- Regression-only fixtures remain covered by existing targeted negative tests instead of joining the canonical sweep.
