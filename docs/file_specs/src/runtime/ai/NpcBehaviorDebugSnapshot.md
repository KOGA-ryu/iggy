# File Spec

Files: `src/runtime/ai/NpcBehaviorDebugSnapshot.hpp`, `src/runtime/ai/NpcBehaviorDebugSnapshot.cpp`

Verified at: `3dab1aa4`

## Owns

- `NpcBehaviorDebugSnapshotRequest`, result/status packets, actor debug rows, and `buildNpcBehaviorDebugSnapshot(...)`.
- Read-only projection of NPC world rows, AI mirrors, combat status, profile resolution, target facts, and aggregate counts.

## Does Not Own

- AI decision truth, perception truth, alert state updates, combat updates, or world mutation.
- Debug HUD layout or render colors.
- Save/hash persistence.

## Reads

- `WorldState` entities, `AiState` actors, `CombatState` combatants, and `NpcBehaviorProfileCatalog`.
- AI mirrors including radius/cone/LOS/perceived fields and cooldown/next-decision fields.

## Writes / Mutates

- Returned snapshot rows, counts, status, and reason code only.
- No mutation of world, AI, combat, or profile catalog.

## Calls Out To / Wires Out To

- `resolveNpcBehaviorProfile(...)` for row profile status.
- Local world/AI/combat lookup helpers for row construction.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds snapshots for debug projection.
- `DebugProjection.cpp` appends snapshot rows and HUD lines.
- Grep proof: `rg -n "NpcBehaviorDebugSnapshot|appendNpcBehaviorDebugSnapshot|npc_behavior_debug_snapshot_tests" src tests cmake CMakeLists.txt`.

## Invariants

- Disabled or missing-input requests return status/reason without building rows.
- Rows include NPC world entities first, then AI actors without world NPC rows, sorted by actor id.
- `lastTargetHasLineOfSight`, `lastPerceived`, and `lastLos` are copied as mirrors, not recomputed.
- `maxActors` caps emitted rows.

## Tests / Proof Commands

- `rg -n "npc_behavior_debug_snapshot_tests" cmake tests/unit`.
- `rg -n "lastPerceived|lastLos|targetHasLineOfSight" tests/unit/npc_behavior_debug_snapshot_tests.cpp src/runtime/ai/NpcBehaviorDebugSnapshot.*`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/AiState.hpp` unless source mirror fields change.
- `src/projection/debug/DebugProjection.*` unless projection row/HUD consumption changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless snapshot build site changes.

## Update When

- Snapshot request/result shape, row fields, status handling, ordering, caps, or copied AI mirrors change.

## Do Not Update When

- AI behavior changes but snapshot inputs and row contracts stay stable.
