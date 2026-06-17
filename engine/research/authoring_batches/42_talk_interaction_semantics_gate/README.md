# Batch 42: Talk Interaction Semantics Gate

## Goal
Decide whether `kind = "talk"` should be report-only, text inspection, or a future dialogue hook.

## Current State
`Talk` target kind exists in the source-plan enum, but no dialogue system should be invented casually.

## Slices
1. Review existing interaction target/effect and report surfaces.
2. Decide the smallest useful behavior for talk targets, or keep them as validated-but-unexecuted metadata.
3. If approved, write a narrow implementation packet.

## Verification
Read-only gate unless docs are updated.

## Hard Stops
No dialogue trees, branching scripts, UI, localization, or quest state.

## Expected Result
Talk targets have a deliberate boundary before implementation.
