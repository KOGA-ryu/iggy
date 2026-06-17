# Batch 04: Scenario Expectations

## Goal
Allow TOML scenarios to declare simple expected final outcomes for the CLI to report.

## Current State
CLI tests own expected rows/counts in C++. TOML fixtures do not carry expectations.

## Slices
1. Add source-plan expectation facts for final rows and selected summary counts.
2. Parse a minimal `[expect]` TOML table or equivalent stable shape.
3. Validate expectation shape only. Do not affect gameplay conversion.
4. Have the CLI print expectation comparison status after running.
5. Add tests for matching and mismatching expectations.

## Verification
Focused source-plan, TOML reader, and CLI tests. Full verification at batch end.

## Hard Stops
No assertions that change runtime behavior. No broad golden framework. No JSON.

## Expected Result
Fixtures can state expected final rows/counts, and the CLI can tell whether the run matched them.
