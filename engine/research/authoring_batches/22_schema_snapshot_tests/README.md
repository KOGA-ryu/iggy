# Batch 22: Schema Snapshot Tests

## Goal
Snapshot the supported TOML table/key surface so accidental authoring format drift is visible.

## Current State
The TOML reader supports a narrow subset by hand-written parsing.

## Slices
1. Add a test-owned list of supported table names and keys.
2. Assert canonical fixtures use only the supported surface.
3. Add negative tests for unsupported tables/keys only where current behavior is intended to reject them.
4. Keep docs/example list aligned.

## Verification
Focused TOML reader and fixture tests. Full verification at batch end.

## Hard Stops
No full TOML schema language. No external schema dependency.

## Expected Result
Format surface changes are deliberate and reviewed.
