# Batch 10: NPC Collision/Reservation Fixtures

Status: complete.

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

## Completed Coverage

- `npc_blocked_guard_room.toml` proves authored NPC movement into an occupied tile reports `npc_blocked_movement_count = 1` and leaves final rows unchanged.
- `npc_reservation_guard_room.toml` proves current default authored same-destination movement behavior: both NPCs move to the target tile, producing `npc_moved_count = 2` and an inspectable shared destination in final runtime state, with the final ASCII projection showing the last overlaid actor glyph.
- The CLI summary now prints existing runtime `npc_blocked_movement_count` so blocked movement is visible without changing gameplay semantics.

No capacity/reservation config authoring was added; the packet stays within existing TOML facts and runtime defaults.
