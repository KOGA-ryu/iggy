# Batch 34: Negative Fixture Catalog

## Goal
Make failure fixtures first-class regression assets for parser, source validation, conversion, and run validation failures.

## Current State
`corrupt_guard_room.toml`, `semantic_invalid_guard_room.toml`, and `valid_guard_room.toml` cover a few negative paths.

## Slices
1. Inventory current negative CLI and lower-level tests.
2. Add minimal negative fixtures for missing profile, bad command target, bad pickup/drop reference, bad table type, and unsupported version only where current validators support them.
3. Document each negative fixture in the fixture README as regression-only.
4. Add table-driven diagnostics tests with stable first-error expectations.

## Verification
Focused TOML reader, source-plan validator, converter, and CLI diagnostics tests.

## Hard Stops
No parser compliance expansion or semantic validation beyond existing safe checks.

## Expected Result
Failure behavior is covered by named fixtures instead of hidden inline setup.
