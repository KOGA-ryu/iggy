# Batch 58: Actor State Expectations

## Goal
Extend authored expectations to verify final NPC/player tile or position facts for movement scenarios.

## Current State
Final rows show positions, but explicit actor/player expectations would give better diagnostics.

## Slices
1. Add expectation facts for actor id and expected tile/position using current final state data.
2. Add player expectation shape only if player state is present.
3. Validate referenced actor ids against promoted actors.
4. Compare after run and report mismatches without changing movement.

## Verification
Focused source-plan/TOML/CLI check tests.

## Hard Stops
No pathfinding changes, movement tolerance policy beyond existing deterministic positions, or animation state.

## Expected Result
Movement fixture failures identify which actor/player state mismatched.
