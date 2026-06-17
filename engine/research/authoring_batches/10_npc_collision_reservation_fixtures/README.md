# Batch 10: NPC Collision/Reservation Fixtures

## Goal
Add authored fixtures proving NPC occupancy, blocking, and reservation behavior through the CLI.

## Current State
Scene NPC occupancy policy and reservation systems exist. TOML can author NPC controls and multi-frame movement.

## Slices
1. Add fixture for blocked NPC movement into occupied tile.
2. Add fixture for reservation ordering with two NPCs targeting the same tile.
3. Add fixture for capacity-allowed shared tile only if existing config can be authored or defaulted safely.
4. Add CLI golden tests for counts/final rows.
5. Update fixture README.

## Verification
Focused CLI and relevant NPC movement tests. Full verification at batch end.

## Hard Stops
No new faction/spirit/swarm semantics. No pathing algorithm changes.

## Expected Result
NPC collision/reservation behavior is visible in authored fixtures.
