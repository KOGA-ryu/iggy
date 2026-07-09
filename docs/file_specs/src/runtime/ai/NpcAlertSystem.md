# File Spec

Files: `src/runtime/ai/NpcAlertSystem.hpp`, `src/runtime/ai/NpcAlertSystem.cpp`

Verified at: `9cd3a6b5`

## Owns

- Graded NPC alert FSM over `AiActorState::alertLevel`.
- Alert profile validation, band mapping, behavior derivation, rise, decay, dead-time, grace-window, and high-water engagement tracking.
- Visual and heard-only stimulus handling through `NpcAlertStimulus`.

## Does Not Own

- Stimulus construction from perception/sound events.
- Target LOS, sound propagation, last-known memory, patrol/investigate/search decisions, or command emission.
- Save/load of actor state.

## Reads

- `AiActorState`, `AlertProfile`, `NpcAlertStimulus`, and current tick.
- Visual fields: perceived/proximity/valid-target/visual-confirmed.
- Sound fields: heard, audibility, alert units, and investigation position.

## Writes / Mutates

- Mutates actor alert level, alert timing counters, max alert band, grace state, and derived behavior.

## Calls Out To / Wires Out To

- Uses `AiBehaviorKind` naming/values from AI state.
- Session code constructs stimuli and calls `npcStepAlert(...)`.

## Called By / Entry Points

- `Session.cpp` calls `npcStepAlert(...)` in the live AI loop.
- Alert FSM tests, tuning readouts, stealth garden tests, save/load tests, and session tick tests include this surface.
- Grep proof: `rg -n "npcStepAlert|npcRaiseAlert|npcDecayAlert|alertBandIndex|alertBehaviorForLevel" src tests cmake`.

## Invariants

- Alert level stays clamped to the normalized 0..1 range.
- Behavior is derived from alert level, not set independently by callers.
- Combat band requires a valid target; nonvisual sound cannot raise into combat.
- Visual confirmation bypasses grace anti-spam.
- This kernel remains deterministic and headless.

## Tests / Proof Commands

- `rg -n "npc_alert_fsm_tests|stealth_tuning_readout_tests|stealth_garden_tests|session_tick_tests|save_load_tests" cmake tests`.
- `rg -n "heard|visualConfirmed|alertUnits|soundRiseScale" src/runtime/ai src/runtime/session tests`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.cpp` unless stimulus construction changes.
- `src/runtime/ai/NpcBehaviorProfile.*` unless alert tuning/profile mapping changes.
- `src/runtime/ai/NpcInvestigateSystem.*` unless memory response to alert band changes.

## Update When

- Alert profile fields, validation, band thresholds, rise/decay/grace behavior, sound stimulus behavior, or behavior derivation changes.

## Do Not Update When

- Only perception or sound event production changes while the stimulus contract stays stable.
