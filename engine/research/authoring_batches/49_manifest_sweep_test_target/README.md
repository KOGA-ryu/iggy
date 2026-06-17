# Batch 49: Manifest Sweep Test Target

## Goal
Use the canonical fixture manifest from Batch 32 to sweep all canonical fixtures through every supported safe mode.

## Current State
Fixture README and CLI tests list fixtures manually. Batch 32 adds a test-owned manifest.

## Slices
1. Build a table-driven sweep over manifest entries and mode expectations.
2. Run normal mode for every success fixture.
3. Run trace/check/lint only where the fixture declares support.
4. Keep regression-only fixtures in separate negative tests.

## Verification
Focused CLI manifest sweep tests.

## Hard Stops
No runtime directory scanning, CLI discovery command, or package runner behavior.

## Expected Result
Adding a canonical fixture automatically adds intended mode coverage.
