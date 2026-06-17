# Batch 09: Locked Door/Key Implementation

## Goal
Implement locked-door/key authored scenario support only if Batch 08 passes.

## Current State
Requires Batch 08 gate result.

## Slices
1. Add the smallest existing-compatible interaction/effect or inventory condition support identified by Batch 08.
2. Add source-plan/TOML authoring facts only for the accepted semantics.
3. Promote facts through converter without broad scripting.
4. Add a fixture: pickup key, interact door, final door state changes.
5. Add CLI golden test and docs.

## Verification
Focused tests per slice, full verification at batch end.

## Hard Stops
Do not proceed without Batch 08 approval. No generic condition/effect language.

## Expected Result
A key-door scenario works through existing or explicitly accepted minimal semantics.
