# Batch 03: CLI Diagnostics Matrix

Status: complete.

## Goal
Cover every intended CLI failure class with stable tests and useful first-error output.

## Current State
The CLI prints first useful issue details for file, TOML, source-plan, conversion, and profile validation failures.

## Slices
1. Inventory existing CLI failure tests and diagnostics.
2. Add or normalize fixtures/inline temp files for:
   - missing file
   - corrupt TOML
   - wrong TOML type
   - source-plan semantic issue
   - conversion issue
   - profile validation issue
3. Ensure output uses `status:` and stable issue fields where possible: table, index, key, line, code/detail.
4. Make failure tests table-driven and avoid brittle incidental wording.

## Verification
Focused CLI tests. Full verification at batch end.

## Hard Stops
No new parser compliance work. Do not introduce a TOML library.

## Expected Result
Authoring failures are predictable from the CLI and regression-tested.
