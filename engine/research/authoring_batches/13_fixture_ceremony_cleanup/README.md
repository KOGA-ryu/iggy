# Batch 13: Fixture Ceremony Cleanup

## Goal
Remove redundant C++ fixture ceremony made obsolete by self-contained TOML and CLI golden tests.

## Current State
Canonical CLI tests cover authored scenario acceptance. Lower-level parser/converter tests still provide targeted coverage.

## Slices
1. Grep for repeated fixture setup helpers and C++ scenario construction in runtime TOML/source-plan tests.
2. Classify each as:
   - keep: parser/converter/config override coverage
   - remove/simplify: duplicate end-to-end acceptance now covered by CLI
3. Trim only clearly redundant assertions/helpers.
4. Keep explicit config override, conflict, terrain, and diagnostics tests.

## Verification
Focused touched tests and CLI golden tests. Full verification at batch end.

## Hard Stops
Do not delete lower-level coverage just because a CLI scenario passes.

## Expected Result
Less test ceremony without losing behavior or diagnostics coverage.
