# Batch 46: Authoring Facade Mode Matrix

Status: complete.

## Goal
Extend the reusable authoring facade, if Batch 16 exists, so tools can request run, lint, check, and trace behavior without reimplementing CLI branching.

## Current State
The CLI owns mode parsing. Batch 16 extracts file-to-run. Batches 06, 05, and 01 define lint, check, and trace modes.

## Slices
1. Inspect the post-Batch-16 facade API and CLI mode code.
2. Add a small mode enum/config for run, lint, check, and trace only if those modes already exist.
3. Return nested read/adapt/run/expectation/trace data by value without formatting CLI text.
4. Update CLI to call the facade mode path without changing output.
5. Add focused facade mode tests over canonical fixtures.

## Verification
Focused authoring facade and CLI tests. Full verification at batch end.

## Hard Stops
No directory scanning, UI/Edi, new gameplay semantics, or broad report framework.

## Expected Result
Future tools use one engine API for existing authoring modes.

## Completed Coverage

- Added `RuntimeGameplayTomlScenarioFacadeMode` for run, lint, check, and trace requests.
- Check mode now returns facade-level pass/fail status from existing expectation comparison while still returning nested read/adapt/run/projection data by value.
- Trace mode captures existing per-frame projections through the facade; check mode can also request trace capture.
- The CLI now maps existing `--lint`, `--check`, and `--trace` options into facade mode/config without changing output or exit codes.
