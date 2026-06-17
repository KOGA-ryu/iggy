# Batch 25: Runtime Authoring Cleanup Gate

## Goal
Review runtime authoring wrappers now that CLI/facade paths exist, and identify safe cleanup targets.

## Current State
Runtime authoring/scenario/ascii families grew quickly. Some wrappers may now be test/tooling-only.

## Slices
1. Grep runtime authoring/scenario/ascii wrappers and tests.
2. Classify permanent API boundaries vs convenience/test wrappers.
3. Propose a cleanup batch only for low-risk duplication.
4. Do not remove code in this gate.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No wrapper deletion in this gate.

## Expected Result
A concrete cleanup plan based on current usage, not frustration.
