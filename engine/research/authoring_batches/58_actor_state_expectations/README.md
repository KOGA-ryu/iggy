# Batch 58: Actor State Expectations

Status: complete.

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

## Completed Coverage
- Added optional `[[expect_actor_states]]` facts for expected final NPC actor tile by actor id.
- Added optional `[expect_player_state]` facts for expected final player tile when a player start is authored.
- Source-plan validation rejects missing, duplicate, and unknown expected actor ids, missing actor tiles, missing player tiles, and player expectations without an authored player start.
- Check mode compares expected actor/player tiles against final `RuntimeGameplayState` without changing movement semantics.
- `player_and_guard_room.toml` now self-checks final guard and player tiles.
