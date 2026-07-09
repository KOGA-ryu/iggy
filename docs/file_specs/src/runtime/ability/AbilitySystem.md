# Ability System

File:

- `/Users/kogaryu/iggy3d/src/runtime/ability/AbilitySystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/ability/AbilitySystem.cpp`

Verified at: `00e97746`

## Owns

- Runtime ability state, runtime projectile state, ability definitions, cast/tick requests, and cast/tick result packets.
- `ArcaneBolt` cast admission, resource spend, cooldown, recharge, projectile spawn, projectile ticking, and impact classification.
- Ability status/name helpers and active projectile visibility helpers.

## Does Not Own

- Command admission or command log policy.
- Projectile motion kernel internals.
- Entity hit-test implementation.
- Combat state rules beyond delegating impact damage through `applyAttack`.
- App HUD, receipts, or creative authoring.

## Reads

- `AbilityState`, `AbilityRuntimeState`, `AbilityCastRequest`, and `AbilityTickRequest`.
- `SpatialSurfaceSet` pointer for projectile surface collision.
- `WorldState` pointer for entity hit checks during projectile ticks.
- Optional mutable `CombatState` pointer for damage application.

## Writes / Mutates

- Mutates actor resource/cooldown/recharge state on accepted casts and recharge ticks.
- Mutates runtime projectile slot while casting, advancing, impacting, expiring, or resetting.
- Mutates `CombatState` only when an entity impact is valid and combat pointer is supplied.

## Calls Out To / Wires Out To

- Calls `stepProjectile(...)` for projectile motion and surface impact.
- Calls `queryFirstEntityHit(...)` for target entity impacts.
- Calls `applyAttack(...)` for combat damage and defeated state.

## Called By / Entry Points

- `findAbilityDefinition(...)`
- `inspectAbilityCast(...)`
- `castAbility(...)`
- `tickAbilityState(...)`
- `tickAbilityRuntime(...)`
- `resetAbilityRuntime(...)`
- `abilityProjectileVisible(...)`
- `abilityRuntimeHasActiveProjectile(...)`
- `abilityStateHasPendingRecharge(...)`

## Invariants

- Cast inspection is non-mutating; `castAbility` is the mutating cast entry point.
- Invalid ability, caster, origin, direction, missing surfaces, busy projectile slot, cooldown, and insufficient resource reject casts.
- Accepted cast spends resource, schedules recharge, sets cooldown, and spawns one ability-owned projectile slot.
- Tick with no active projectile reports no active projectile.
- Entity impact is attempted before final surface-impact handling when the stepped projectile segment is queryable.
- Runtime reset clears the ability projectile state; no persistent cache belongs here.

## Tests / Proof Commands

- `rg -n "ability_system_tests|ability_command_tests|castAbility|inspectAbilityCast|tickAbilityRuntime|tickAbilityState" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `ability_system_tests` and `ability_command_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/projectile/ProjectileSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/EntityHitQuery.*`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/command/*`

## Update When

- Ability definitions, cast admission, resource/cooldown/recharge rules, projectile impact mapping, or damage delegation changes.

## Do Not Update When

- Only command routing, projectile math internals, app receipts, or save codec layout changes.
