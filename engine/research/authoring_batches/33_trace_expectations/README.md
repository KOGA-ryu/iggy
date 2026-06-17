# Batch 33: Trace Expectations

## Goal
Extend authored expectations beyond final rows to optional per-frame trace expectations, using existing `--trace` output and no new execution behavior.

## Current State
Batch 04/05 cover final expectations and check mode. Batch 01 provides trace output.

## Slices
1. Add optional expected frame ids/counts/rows facts only if Batch 04 expectation shape exists.
2. Validate expectation shape without affecting scenario conversion.
3. Compare expected trace facts in CLI check mode or a focused helper.
4. Add one multi-frame fixture expectation test.

## Verification
Focused expectation, trace, and CLI tests.

## Hard Stops
No new runtime semantics, broad golden framework, or external golden files unless explicitly accepted.

## Expected Result
Multi-frame scenarios can self-check progression, not only final state.
