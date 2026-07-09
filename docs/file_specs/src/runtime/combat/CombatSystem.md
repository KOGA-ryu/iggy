# Combat System

File:

- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.cpp`

Verified at: `00e97746`

## Owns

- Runtime combatant packet state: entity id, faction id, hit points, max hit points, defeated flag.
- Combat attack request/result/status contracts.
- Attack preview and attack application against `CombatState`.
- Combatant invariant validation and attack rejection reasons.

## Does Not Own

- AI decision to attack.
- Command admission/logging around attack commands.
- Ability projectile hit detection before damage.
- Save/load serialization of combat state.
- App feedback/HUD presentation.

## Reads

- `CombatState::combatants`.
- `CombatAttackRequest` attacker, target, damage, and command id.
- Combatant faction, hit points, max hit points, and defeated state.

## Writes / Mutates

- `previewAttack` does not mutate combat state.
- `applyAttack` mutates only the target combatant hit points and defeated flag after a successful validation.
- Returns `CombatAttackResult` with applied damage, target hit points, defeated flag, and mutation flag.

## Calls Out To / Wires Out To

- Uses core `EntityId` validity and vector scans only.
- Called by command/session paths and `AbilitySystem` damage application.

## Called By / Entry Points

- `previewAttack(...)`
- `applyAttack(...)`

## Invariants

- Every combatant must have a valid entity id, positive max hit points, hit points in range, and `defeated == (hitPoints == 0)`.
- Duplicate combatant entity ids invalidate the combat state.
- Missing attacker, missing target, defeated attacker, defeated target, friendly fire, and non-positive damage are rejected.
- Nonzero matching factions block friendly fire.
- Successful damage is clamped to the target's current hit points.

## Tests / Proof Commands

- `rg -n "combat_system_tests|combat_command_tests|previewAttack|applyAttack|CombatState" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `combat_system_tests` and `combat_command_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/command/*`
- `/Users/kogaryu/iggy3d/src/runtime/ability/AbilitySystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/ai/NpcBehaviorSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.*`

## Update When

- Combatant invariants, attack validation, damage application, faction policy, or result fields change.

## Do Not Update When

- Only AI chooses different targets, command admission changes, or app feedback copy changes.
