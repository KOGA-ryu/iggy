# Batch 47: Authoring Diagnostic Entries

Status: complete.

## Goal
Add a compact projection helper that flattens existing read/TOML/source/converter/run diagnostics into ordered user-facing entries.

## Current State
The CLI prints first useful issue details from nested results. Batch 28 stabilizes error-code strings.

## Slices
1. Inventory existing issue fields and CLI first-error output.
2. Add a small projection struct with source layer, code, table/key/index/line, id, and detail where already available.
3. Project existing nested diagnostics without changing validation behavior.
4. Update CLI/facade tests to check representative projected entries.

## Verification
Focused diagnostics projection and CLI failure tests.

## Hard Stops
No localization, warning channel unless Batch 35 approves it, or new validation semantics.

## Expected Result
CLI/editor/facade consumers can show consistent diagnostics without traversing every nested result shape.

## Completed Coverage

- Added `RuntimeGameplayAuthoringDiagnosticEntry` and `projectRuntimeGameplayAuthoringDiagnostics` to flatten existing read, TOML, source-plan, adapter, converter, profile, and scenario diagnostics.
- `RuntimeGameplayTomlScenarioFacadeResult` now carries projected diagnostics on read, conversion, lint, and run failures without changing validation behavior or CLI output.
- Focused diagnostics tests cover representative read/source-plan, conversion, and nested profile failures.
