# Batch 19: Editor Preview Model Gate

## Goal
Decide whether an engine-only preview model should exist before UI/Edi integration.

## Current State
The CLI can render final rows and trace rows. UI/Edi is still out of scope.

## Slices
1. Review current final/trace result data and CLI formatting.
2. Identify the minimal engine data model a UI would need: rows, frames, statuses, issue list, counts.
3. Decide whether to add a backend-neutral preview model or keep CLI-only for now.
4. If approved, write the next implementation packet.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No UI/Edi implementation in this gate.

## Expected Result
A go/no-go for a reusable preview model.
