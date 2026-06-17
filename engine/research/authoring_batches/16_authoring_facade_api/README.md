# Batch 16: Authoring Facade API

Status: complete.

## Goal
Extract the CLI's file-to-run pipeline into a reusable engine/runtime helper that future tools can call without duplicating CLI code.

## Current State
`iggy_scenario_toml_runner` owns the direct sequence from TOML file reader to authoring adapter, profile scenario runner, final debug rows, and summaries.

## Slices
1. Identify the exact CLI pipeline code that is pure and reusable.
2. Add a narrow runtime helper for one-file execution, returning read/adapt/run/final-row results by value.
3. Update CLI to call the helper without changing output.
4. Add focused tests for helper success/failure using canonical fixtures.
5. Keep CLI tests green.

## Verification
Focused helper and CLI tests. Full verification at batch end.

## Hard Stops
No directory scanning, no UI/Edi, no new gameplay semantics, no save/load.

## Expected Result
CLI and future tools share one engine-level file-to-run facade.

## Completed Coverage

- `RuntimeGameplayTomlScenarioFacade` now owns the one-file TOML source-plan pipeline: file reader, authoring adapter, optional lint validation, profile scenario runner, final row projection, trace frame projection, and expectation comparison.
- `iggy_scenario_toml_runner` calls the facade while preserving existing output sections and exit codes.
- Focused facade tests cover run success, trace capture, lint-only validation, read failure, and conversion failure.
