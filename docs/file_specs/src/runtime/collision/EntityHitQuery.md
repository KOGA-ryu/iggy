# Entity Hit Query

File:

- `/Users/kogaryu/iggy3d/src/runtime/collision/EntityHitQuery.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/collision/EntityHitQuery.cpp`

Verified at: `00e97746`

## Owns

- Runtime entity segment-hit query contract: `EntityHitQueryRequest`, `EntityHitQueryResult`, `EntityHitStatus`.
- First-hit testing from a segment against active `WorldState` entities.
- Candidate filtering for ignored entity, active flag, valid bounds, and optional attack-target support.
- Hit result packets with entity id, stable name, expanded world bounds, hit point, normal, distance, time, counts, and reason code.

## Does Not Own

- World entity storage or mutation; that belongs to `WorldState`.
- Combat damage application; that belongs to `CombatSystem`.
- Projectile stepping; that belongs to `ProjectileSystem`.
- Surface collision; that belongs to `CollisionQuery` or physics collision queries.

## Reads

- `WorldState::entities()`.
- `EntityState` id, stable name, transform, local bounds, active flag, and targeting flags.
- `EntityHitQueryRequest` segment endpoints, ignored entity, radius, and attack-target requirement.

## Writes / Mutates

- Does not mutate world state.
- Returns `MissingWorld`, `InvalidInput`, `NoHit`, or `Hit` result packets.

## Calls Out To / Wires Out To

- Uses `transformPointScaleTranslate` to convert local entity bounds to world-space bounds.
- Uses `isTargetActionSupported(..., TargetAction::Attack)` when `requireAttackTarget` is true.

## Called By / Entry Points

- `entityHitStatusName(...)`
- `queryFirstEntityHit(...)`
- `AbilitySystem` uses this surface to detect projectile entity impacts before applying combat damage.

## Invariants

- Missing world pointer returns `entity_hit_missing_world`.
- Non-finite or degenerate segments and negative/non-finite radius return `entity_hit_invalid_input`.
- Inactive entities, invalid ids, invalid bounds, ignored entity, and non-attack targets are not candidates.
- Radius expands candidate world bounds before testing.
- Earliest segment hit wins; equal-time candidates are tie-broken by stable name.

## Tests / Proof Commands

- `rg -n "entity_hit_query_tests|queryFirstEntityHit|EntityHitStatus" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `entity_hit_query_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/world/WorldState.*`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/projectile/ProjectileSystem.*`

## Update When

- Entity hit eligibility, hit tie-breaks, result fields, or reason codes change.
- Ability/projectile callers require new entity-hit proof fields.

## Do Not Update When

- Only surface collision, world insertion validation, or combat damage math changes.
