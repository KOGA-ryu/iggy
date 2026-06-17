# Batch 09: Locked Door/Key Implementation

Status: complete.

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

## Completed Semantics

Batch 09 uses the Batch 08 approved narrow required-item path:

- `[[interaction_targets]]` may declare `required_item_id = "item:key"`.
- The requirement is promoted as frame-level runtime authoring metadata, not saved interaction state.
- Player interaction effect application checks the current runtime inventory before applying effects for that target.
- If the item is present, existing effects such as `toggle_target` apply normally.
- If the item is missing, the accepted interact command produces no interaction mutation and the target remains unchanged.

This is intentionally not a generic condition language, scripting system, event bus, range/pathing extension, save/load change, or UI/Edi feature.
