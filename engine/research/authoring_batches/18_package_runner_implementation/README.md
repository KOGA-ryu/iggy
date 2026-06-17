# Batch 18: Package Runner Implementation

## Goal
Implement directory/package execution only if Batch 17 passes.

## Current State
Requires Batch 17 approval.

## Slices
1. Add the minimal accepted package lookup, likely `scenario.toml` in a supplied directory.
2. Keep one-file CLI behavior unchanged.
3. Add package success/failure tests using a checked-in package fixture.
4. Update docs with one usage example.

## Verification
Focused CLI/package tests. Full verification at batch end.

## Hard Stops
Do not proceed without Batch 17. No recursive directory scanning or discovery.

## Expected Result
CLI can run one explicit package directory according to the accepted shape.
