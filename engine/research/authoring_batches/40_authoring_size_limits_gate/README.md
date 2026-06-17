# Batch 40: Authoring Size Limits Gate

## Goal
Decide whether source plans need explicit size/count limits before larger content appears.

## Current State
Source-plan vectors can grow for rows, cells, controls, targets, drops, commands, and regions.

## Slices
1. Review current parse/validate/convert complexity by row count, cell count, target/drop count, and frame count.
2. Decide whether limits should be documentation-only, lint warnings, or hard validation errors.
3. Produce an implementation packet only if hard limits are justified.

## Verification
Read-only unless docs are updated.

## Hard Stops
No arbitrary hard caps without evidence; no wall-clock flaky tests.

## Expected Result
Scale policy is explicit before authored scenarios grow much larger.
