# Batch 08: Locked Door/Key Gate

## Goal
Review whether locked-door/key gameplay can be expressed with existing systems before implementation.

## Current State
Interaction toggle effects and pickup/inventory exist. Key-required door semantics may not.

## Slices
1. Grep existing interaction, inventory, pickup, and effect systems for key/requirement semantics.
2. Document whether current systems can express:
   - pickup key
   - interact door
   - door only toggles if inventory has key
3. If existing systems do not support it, stop with evidence and proposed minimal design options.
4. If existing systems do support it, produce a build order for Batch 09.

## Verification
Read-only review unless docs are added. No full build needed for read-only review.

## Hard Stops
No code implementation in this gate unless the planner explicitly converts it into Batch 09.

## Expected Result
A clear go/no-go decision for locked-door/key semantics.
