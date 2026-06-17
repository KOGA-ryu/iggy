# Batch 06: Lint Mode

Status: complete.

## Goal
Add CLI validation without scenario execution.

## Current State
The CLI can run scenarios and report failures after read/adapt/run.

## Slices
1. Add `--lint <path>` CLI mode.
2. Run TOML file reader, source-plan validation, authoring adapter conversion, and profile scenario validation as far as existing APIs permit without executing frames.
3. Print stable `status:` and diagnostics sections.
4. Return zero only when the authored scenario is convertible/valid.
5. Add tests for valid fixture, corrupt TOML, semantic issue, and conversion issue.

## Verification
Focused CLI tests. Full verification at batch end.

## Hard Stops
No gameplay execution in lint mode. No new validation semantics beyond existing safe checks.

## Expected Result
Authors can check a scenario file without running it.
