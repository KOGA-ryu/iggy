# Batch 21: Source-Plan Version Policy

Status: complete.

## Goal
Make supported source-plan `format_id` and `version` behavior explicit and tested.

## Current State
Source plans carry format/version facts, but compatibility policy should be tightened before broader authoring use.

## Slices
1. Audit current reader/validator behavior for format id and version.
2. Add explicit accepted version tests and unsupported version diagnostics.
3. Keep compatibility narrow: support current version only unless existing fixtures require more.
4. Update fixture README if behavior changes.

## Verification
Focused source-plan/TOML reader/CLI diagnostics tests. Full verification at batch end.

## Hard Stops
No migration framework unless explicitly needed.

## Expected Result
Unsupported source-plan versions fail clearly.

## Completed Coverage
- Source-plan validation now rejects non-empty `format_id` values other than `iggy:ascii-source-plan`.
- Source-plan validation now accepts only `version = 1` and reports unsupported versions with actual/supported values.
- TOML source locations now map unsupported `format_id` and `version` diagnostics back to root keys.
- Added regression-only fixtures for unsupported format and unsupported version.
- CLI and file-reader diagnostics now cover unsupported format/version fixtures.
- Fixture README now states canonical fixtures must use `format_id = "iggy:ascii-source-plan"` and `version = 1`.
