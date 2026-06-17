# Batch 17: Package Runner Gate

## Goal
Review whether directory/package execution should be opened after the package layout proposal.

## Current State
The CLI intentionally reads exactly one TOML file. Batch 14 proposes package shape but does not implement scanning.

## Slices
1. Review the package layout proposal and canonical fixture needs.
2. Identify whether package mode needs `scenario.toml`, expected output files, or README-only docs.
3. Decide whether package mode should be a CLI feature, test helper, or not built yet.
4. Return a go/no-go and exact implementation packet if approved.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No implementation in this gate unless explicitly redirected.

## Expected Result
A clear package-runner decision before directory semantics are added.
