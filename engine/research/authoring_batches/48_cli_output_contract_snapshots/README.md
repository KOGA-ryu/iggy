# Batch 48: CLI Output Contract Snapshots

Status: complete.

## Goal
Lock the textual CLI contract for normal, trace, lint, check, and failure modes after those modes exist.

## Current State
CLI output is stable enough for tests, but section drift can still happen ad hoc.

## Slices
1. Identify supported CLI modes after packets 01, 05, and 06 land.
2. Add small expected-output helpers or focused snapshot strings for section headers and key fields.
3. Cover one success, one trace, one lint/check success, one expectation mismatch, and one failure.
4. Keep snapshots narrow: section shape, not every incidental path string.

## Verification
Focused CLI runner tests.

## Hard Stops
No JSON/machine format, broad snapshot framework, or external golden files unless already accepted.

## Expected Result
CLI output remains stable for users and future tooling.

## Completed Coverage

- Added path-normalized CLI output contract snapshots for run, trace, lint, check success, check mismatch, and read failure modes.
- Snapshots lock section order and key field shape without external golden files or JSON/machine-format output.
- Existing broader CLI fixture, trace, check, lint, and failure assertions remain in place.
