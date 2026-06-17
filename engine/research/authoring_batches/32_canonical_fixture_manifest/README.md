# Batch 32: Canonical Fixture Manifest

Status: complete.

## Goal
Add an explicit test-owned manifest of canonical authoring fixtures and expected modes so fixture sweeps do not rely on ad hoc hardcoded lists.

## Current State
The fixture README lists canonical fixtures, and CLI tests have a local table.

## Slices
1. Add a small C++ test fixture manifest helper or data file under tests, not a runtime feature.
2. Include fixture filename, category, expected success/failure, and modes expected to run.
3. Update CLI tests to consume the manifest where practical.
4. Keep regression-only fixtures out of the canonical sweep unless explicitly listed.

## Verification
Focused CLI tests and fixture tests.

## Hard Stops
No directory scanning, runtime package manifest, or external schema dependency.

## Expected Result
The canonical fixture pack has one test source of truth.

## Completed Coverage

- Added test-owned `CanonicalAuthoringFixtures()` metadata under `engine/tests/support`, including fixture filename, category, expected result, supported modes, summary counts, and final rows.
- Updated `iggy_scenario_toml_runner_tests` to consume the manifest for the canonical success sweep instead of carrying its own duplicate fixture table.
- Regression-only parser/converter/profile fixtures remain outside the canonical manifest and stay covered by targeted failure tests.
