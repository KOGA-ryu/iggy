# File Spec

Files: `src/runtime/ai/NpcInvestigateSystem.hpp`, `src/runtime/ai/NpcInvestigateSystem.cpp`

Verified at: `9cd3a6b5`

## Owns

- Last-known target/investigation memory helpers on `AiActorState`.
- Visual sighting recording and heard-only investigation memory refresh policy.
- Investigate step decision: inactive, move toward remembered spot, dwell at spot, or clear memory.

## Does Not Own

- Alert band calculation, perception/LOS, sound resolution, route planning, movement command emission, or combat precedence.
- Patrol cursor behavior.

## Reads

- Actor last-known target fields, dwell counter, actor position, stimulus position, alert band, visual-confirmed flag, arrival epsilon, dwell limit, and relocate epsilon.

## Writes / Mutates

- Mutates actor last-known target position/tick flag and investigate dwell ticks.
- Returns `NpcInvestigateStep`.

## Calls Out To / Wires Out To

- Session code records sightings/sounds and calls `npcStepInvestigate(...)` before building investigate movement.
- Uses horizontal X/Z distance as the ground behavior distance metric.

## Called By / Entry Points

- `Session.cpp` calls visual/nonvisual memory helpers and investigation stepping.
- `npc_investigate_system_tests`, `session_tick_tests`, stealth tests, and tuning readouts cover this surface.
- Grep proof: `rg -n "npcRecordSighting|npcRecordNonvisualInvestigationMemory|npcStepInvestigate" src tests cmake`.

## Invariants

- Visual sighting refreshes memory and resets dwell.
- Same-origin nonvisual stimuli do not restart dwell unless alert band increases; relocated sounds refresh memory.
- Below Searching band clears memory.
- Visual confirmation returns inactive because combat/recording own the current target.
- Arrival/dwell uses horizontal X/Z distance.

## Tests / Proof Commands

- `rg -n "npc_investigate_system_tests|session_tick_tests|stealth_tuning_readout_tests|stealth_garden_tests" cmake tests`.
- `rg -n "npcRecordNonvisualInvestigationMemory|kNonvisualInvestigateRelocateEpsilonMeters|kInvestigateDwellTicks" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/NpcAlertSystem.*` unless alert-band semantics change.
- `src/runtime/ai/NpcPatrolSystem.*` unless arrival epsilon policy changes.
- `src/runtime/session/Session.cpp` unless memory/stimulus call ordering changes.

## Update When

- Investigation memory refresh, dwell, clearing, horizontal distance, or active-step semantics change.

## Do Not Update When

- Command routing or movement execution changes after this kernel returns a destination.
