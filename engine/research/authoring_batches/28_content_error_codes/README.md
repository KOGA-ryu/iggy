# Batch 28: Content Error Codes

## Goal
Stabilize user-facing authoring error codes across reader, validator, converter, and CLI layers.

## Current State
Diagnostics exist, but codes may be spread across enum names and details.

## Slices
1. Inventory current source-plan/TOML/converter/adapter CLI error codes.
2. Add stable printable code strings where useful.
3. Update CLI diagnostics to print those codes without losing details.
4. Add tests for representative code output.

## Verification
Focused diagnostics/CLI tests. Full verification at batch end.

## Hard Stops
No broad error framework. No localization.

## Expected Result
Authors get stable error identifiers for common failures.
