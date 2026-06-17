# Batch 23: Fixtures From CLI Examples

Status: complete.

## Goal
Make docs/examples match the actual canonical fixtures and CLI output.

## Current State
Fixture README is example-driven, but examples can drift from real fixture files.

## Slices
1. Review README examples against canonical fixtures.
2. Replace hand-written snippets with references to exact fixture sections where possible.
3. Add a lightweight test or docs check only if local style supports it.
4. Keep docs short and practical.

## Verification
Docs diff check and any fixture tests if touched.

## Hard Stops
No docs generator unless it is trivial and local.

## Expected Result
Fixture documentation stays grounded in executable examples.

## Completed Coverage
- Updated fixture docs to point at exact checked-in TOML fixtures for common
  authored scenario examples instead of relying only on the long illustrative
  table sample.
- Documented that `RuntimeGameplayAuthoringPreviewModel` reads the same
  explicit TOML/package paths and exposes existing facade projections for
  non-CLI consumers.
- No docs generator or production behavior changes were added.
