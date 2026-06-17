# Batch 20: Authoring Diff Report

## Goal
Add a small comparison helper for two scenario run results, useful for regression/debugging.

## Current State
The CLI can run one scenario and print final/trace output.

## Slices
1. Add a pure comparison helper over final rows and summary counts.
2. Add CLI-local or test-only usage if it stays narrow.
3. Add tests comparing two fixtures with expected differences.
4. Do not add a broad report framework.

## Verification
Focused comparison tests and CLI tests if touched. Full verification at batch end.

## Hard Stops
No gameplay semantics, no JSON output, no directory scanning.

## Expected Result
Developers can compare authored scenario outcomes deterministically in tests.
