# Batch 02: Fixture Contracts

## Goal
Make the canonical fixture set explicit and easy to maintain.

## Current State
Canonical TOML fixtures exist and are covered by CLI golden smoke tests.

## Slices
1. Audit fixture names and README descriptions under `engine/tests/fixtures/runtime/ascii_source_plan`.
2. Mark each fixture as either canonical or regression-only in the fixture README.
3. Normalize fixture naming if needed without deleting compatibility fixtures.
4. Add a small fixture contract section: required self-contained profile facts, expected CLI success, and no hidden C++ converter config.
5. Update CLI table-driven tests to use the canonical list from a local test table with clear names.

## Verification
Focused CLI tests and TOML file-reader tests. Full verification at batch end.

## Hard Stops
Do not add gameplay semantics. Do not delete fixtures used by config override or regression tests unless replacement coverage is obvious.

## Expected Result
The fixture directory explains which files are stable examples and which are lower-level regression fixtures.
