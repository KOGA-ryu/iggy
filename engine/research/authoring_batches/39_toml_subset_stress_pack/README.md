# Batch 39: TOML Subset Stress Pack

## Goal
Add small parser regression tests for supported TOML subset edges without becoming a full TOML implementation.

## Current State
The reader is dependency-free and intentionally narrow.

## Slices
1. Add tests for whitespace/comments, table ordering, repeated arrays, inline tables, escaped strings if currently supported, and clear rejection if unsupported.
2. Keep examples tiny and anchored to source-plan fields.
3. Confirm unsupported TOML fails with useful diagnostics.

## Verification
Focused TOML reader tests.

## Hard Stops
No TOML library, full compliance work, or file IO changes.

## Expected Result
The hand reader's supported subset is stable under common author edits.
