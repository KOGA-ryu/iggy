# File Spec

Files: `src/runtime/ai/NpcBehaviorSystem.hpp`, `src/runtime/ai/NpcBehaviorSystem.cpp`

Verified at: `9cd3a6b5`

## Owns

- NPC perception request/result contract, perception config validation, and status names.
- 3D target perception gates: actor/target validity, active/defeated state, radius, horizontal cone, vertical cone, and caller-supplied line of sight.
- Behavior decision status and intent choice from `AiActorState`, perception, config, and current tick.
- Behavior command construction for attack, chase, and return-home intents.

## Does Not Own

- LOS/occlusion bake or collider queries.
- Behavior profile resolution, alert FSM, patrol/investigate/search overlays, command admission, or session tick ordering.
- Runtime movement execution after command creation.

## Reads

- `WorldState`, `CombatState`, actor/target ids, actor facing direction, `NpcBehaviorConfig`, `NpcPerceptionResult::Los`, and `AiActorState`.
- Config values for perception radius, attack/chase distances, vision cones, eye heights, decision interval, attack cooldown, and attack damage.

## Writes / Mutates

- No external state.
- Returns perception, decision, and command result packets.

## Calls Out To / Wires Out To

- Reads world/combat entity facts through runtime state APIs.
- Emits `CommandRecord` values for downstream session command admission/execution.

## Called By / Entry Points

- `Session.cpp` calls `queryNpcPerception(...)`, `chooseNpcBehaviorIntent(...)`, and `buildNpcBehaviorCommand(...)` in the live AI loop.
- Unit tests call all three public entry points directly.
- Grep proof: `rg -n "queryNpcPerception|chooseNpcBehaviorIntent|buildNpcBehaviorCommand" src tests cmake`.

## Invariants

- Invalid pointers, ids, config, defeated actors, and inactive entities produce explicit statuses.
- Degenerate actor facing direction keeps perception omnidirectional.
- Caller-supplied LOS `Blocked` or `Unknown` prevents perception from becoming confirmed.
- Passive engagement policy disables hostile decision/command output.
- This file must stay a pure kernel and must not bake collision, resolve profiles, or mutate session state.

## Tests / Proof Commands

- `rg -n "npc_behavior_system_tests|session_tick_tests" cmake tests`.
- `rg -n "NpcPerceptionResult::Los|target_out_of_cone|TargetOccluded" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.cpp` unless live AI loop wiring changes.
- `src/runtime/ai/NpcBehaviorProfile.*` unless config/profile mapping changes.
- `src/runtime/ai/NpcAlertSystem.*` unless alert stimulus interpretation changes.

## Update When

- Perception gates, LOS interpretation, config validation, decision statuses, intent selection, or command construction changes.

## Do Not Update When

- Session overlays patrol/investigate/search differently without changing these public kernel contracts.
