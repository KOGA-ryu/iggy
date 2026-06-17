# Batch 05: Golden Compare Mode

Status: complete.

## Goal
Add a CLI mode that returns nonzero when TOML-authored expectations do not match actual output.

## Current State
Batch 04 must be complete: expectations exist and CLI can report match/mismatch.

## Slices
1. Add `--check` or similarly named CLI flag using existing expectation comparison results.
2. Keep default runner mode unchanged.
3. Return zero on match, nonzero on mismatch or invalid scenario.
4. Add tests for success and mismatch exit status.
5. Update fixture README with one example.

## Verification
Focused CLI tests. Full verification at batch end.

## Hard Stops
No directory scanning. No watch mode. No external golden files unless separately accepted.

## Expected Result
One TOML file can be executed as a self-checking scenario.
