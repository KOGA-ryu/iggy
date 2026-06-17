# Batch 18: Package Runner Implementation

Status: complete.

## Goal
Implement directory/package execution only if Batch 17 passes.

## Current State
Requires Batch 17 approval.

## Slices
1. Add the minimal accepted package lookup, likely `scenario.toml` in a supplied directory.
2. Keep one-file CLI behavior unchanged.
3. Add package success/failure tests using a checked-in package fixture.
4. Update docs with one usage example.

## Verification
Focused CLI/package tests. Full verification at batch end.

## Hard Stops
Do not proceed without Batch 17. No recursive directory scanning or discovery.

## Expected Result
CLI can run one explicit package directory according to the accepted shape.

## Completed Coverage
- Added `RuntimeGameplayTomlScenarioPackageFacade`, a narrow package wrapper
  that reads `package.toml`, validates `format_id`, `version`, and safe
  relative `main`, then delegates to `RuntimeGameplayTomlScenarioFacade`.
- Added the checked-in `moving_guard_room_package` fixture.
- Updated `iggy_scenario_toml_runner` so explicit package directories or
  `package.toml` paths use the package facade while one-file TOML paths keep
  existing behavior and output sections.
- Added focused facade and CLI tests for package success and package manifest
  failures.
