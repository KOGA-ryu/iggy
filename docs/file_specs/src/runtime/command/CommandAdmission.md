# Command Admission

File:

- `/Users/kogaryu/iggy3d/src/runtime/command/Command.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.cpp`

Verified at: `08cf3da7`

## Owns

- Runtime command packet shape: `CommandRecord`, `CommandPayload`, `CommandTarget`, command ids, sequences, and ticks.
- Command kind/source/admission/rejection enums and small shape helpers such as `requiresActor`.
- Central command admission gate: `admitCommand`, `acceptCommand`, and `rejectCommand`.
- Validation ordering for command shape, player slot, actor binding, clock mode, target state, targetability, reach, required items, retry source, and kind-specific checks.

## Does Not Own

- Command execution after admission.
- Command log storage or replay comparison.
- World, combat, inventory, targeting, or player roster state mutation.
- App menu/input routing into command construction.
- Ability runtime slot/cooldown checks performed after admission.

## Reads

- `CommandAdmissionContext` pointers for `WorldState`, `PlayerRoster`, `ClockState`, `CommandLog`, `RuntimeConfig`, `CombatState`, and `InventoryState`.
- `CommandAdmissionRequest::command` and `allowSessionControlWhilePaused`.
- Target entity state, actor slot binding, combat preview result, inventory contents, and reach query result.

## Writes / Mutates

- Does not mutate context state.
- Returns a copied `CommandRecord` with `Accepted` or `Rejected` admission and rejection reason.
- `rejectCommand` maps accidental `None` rejection into `InternalError`.

## Calls Out To / Wires Out To

- Calls `PlayerRoster::findSlot` and `slotControlsActor`.
- Calls `WorldState::findById`.
- Calls `queryReach` and `rejectionReasonForReach`.
- Calls `hasItem` for interact item requirements.
- Calls `previewAttack` for attack command validation without combat mutation.
- Reads `CommandLog::findById` to validate retry commands.

## Called By / Entry Points

- `admitCommand(...)`
- `acceptCommand(...)`
- `rejectCommand(...)`
- `Session` uses admission before queuing accepted commands and logging rejected commands.

## Invariants

- Invalid command kind, invalid id, or non-pending incoming admission rejects as `InvalidCommand`.
- Actor-required commands require a valid active actor; non-AI actor commands must be controlled by the player slot.
- AI commands that require an actor skip player-slot binding but still require a valid active actor.
- Paused sessions allow only the explicit paused/session-control command subset unless the request disallows paused control.
- `StepTacticalTick` requires paused clock mode.
- Retry validates the rejected source command by replaying the source intent as a pending command with the retry's id, slot, and tick fields.
- `Save`, `Load`, and `Reset` are admitted as unavailable here; product save/load flows live outside this runtime gate.
- Admission is validation only; execution and state mutation belong downstream.

## Tests / Proof Commands

- `rg -n "command_admission_tests|combat_command_tests|ability_command_tests|admitCommand|CommandRejectionReason" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `command_admission_tests`, `combat_command_tests`, and `ability_command_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/ability/AbilitySystem.*`

## Update When

- Command enum values, payload requirements, rejection reasons, admission ordering, retry semantics, paused command rules, or downstream validation delegation changes.

## Do Not Update When

- Only command execution, app input routing, save UI flows, or replay storage format changes.
