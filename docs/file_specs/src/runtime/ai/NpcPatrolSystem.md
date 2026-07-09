# File Spec

Files: `src/runtime/ai/NpcPatrolSystem.hpp`, `src/runtime/ai/NpcPatrolSystem.cpp`

Verified at: `9cd3a6b5`

## Owns

- Patrol route validity checks.
- Patrol cursor stepping for loop and ping-pong waypoint routes.
- Arrival and move-stop constants used by patrol movement wiring.
- `NpcPatrolStep` active/destination packet.

## Does Not Own

- Authored route extraction from content.
- Alert/combat gating, perception, investigation, route planning, or movement command creation.
- Session tick ordering.

## Reads

- `AiActorState` patrol waypoints, target index, direction flag, patrol mode, actor position, and arrival epsilon.

## Writes / Mutates

- Mutates actor patrol target index and ping-pong direction.
- Returns patrol destination packet.

## Calls Out To / Wires Out To

- Session code calls `npcStepPatrol(...)` when patrol owns the current low-alert behavior.
- Uses horizontal X/Z distance for arrival.

## Called By / Entry Points

- `Session.cpp` calls `npcStepPatrol(...)`.
- Patrol, investigate, stealth, and tuning tests include this surface.
- Grep proof: `rg -n "isValidPatrolRoute|npcStepPatrol|kPatrolArriveEpsilonMeters|kPatrolMoveStopMeters" src tests cmake`.

## Invariants

- Empty route is inactive and invalid for validation.
- Non-finite waypoints fail validation.
- Stale patrol target index is clamped before indexing.
- Loop wraps; ping-pong reverses at route ends.
- Patrol arrival ignores Y height.

## Tests / Proof Commands

- `rg -n "npc_patrol_system_tests|npc_investigate_system_tests|stealth_tuning_readout_tests|stealth_garden_tests|session_tick_tests" cmake tests`.
- `rg -n "npcStepPatrol|isValidPatrolRoute" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.cpp` unless patrol command wiring changes.
- `src/runtime/ai/AiState.*` unless patrol state fields change.
- Content/package route extraction files unless authored route loading changes.

## Update When

- Patrol route validation, cursor stepping, arrival constants, or destination semantics change.

## Do Not Update When

- Higher-level behavior gating changes while this patrol kernel stays stable.
