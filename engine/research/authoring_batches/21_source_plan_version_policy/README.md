# Batch 21: Source-Plan Version Policy

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
