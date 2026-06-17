# Batch 26: Minimal Editor Model

## Goal
Implement an engine-only authoring preview model only if Batch 19 passes.

## Current State
Requires editor preview model gate approval.

## Slices
1. Add backend-neutral preview data for source rows, final rows, frame trace rows, statuses, and issues.
2. Build it from existing authoring facade/runner results.
3. Add focused tests over canonical fixtures.
4. Do not touch UI/Edi.

## Verification
Focused preview model tests. Full verification at batch end.

## Hard Stops
Do not proceed without gate approval. No UI code.

## Expected Result
A reusable engine model that a future UI can read without owning runtime logic.
