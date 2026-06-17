# Batch 59: Authoring Run Summary Projection

## Goal
Extract current CLI summary counts into a pure projection helper shared by CLI, facade, tests, and future preview models.

## Current State
The CLI prints summary directly from nested run results. Batch 26 may add preview model later.

## Slices
1. Identify fields currently printed in `summary:` and expectation comparison.
2. Add a small runtime projection struct/helper over run result plus final rows.
3. Update CLI to print from the helper without changing output.
4. Add focused helper tests over canonical fixture results.

## Verification
Focused projection and CLI tests.

## Hard Stops
No broad report/ledger additions, new execution semantics, or UI/Edi.

## Expected Result
Summary counts have one source of truth for CLI/facade/preview work.
