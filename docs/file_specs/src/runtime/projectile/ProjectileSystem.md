# Projectile System

File:

- `/Users/kogaryu/iggy3d/src/runtime/projectile/ProjectileSystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/projectile/ProjectileSystem.cpp`

Verified at: `08cf3da7`

## Owns

- Runtime projectile motion packet shape: `ProjectileMotionParams`, `ProjectileState`, `ProjectileStepRequest`, and `ProjectileStepResult`.
- One-step projectile advancement with gravity, lifetime, max-distance, and surface collision handling.
- Projectile step status names and reason codes.
- Mapping from collision query hits to projectile impact results.

## Does Not Own

- Ability cast admission, cooldowns, resources, or damage application.
- Entity-hit detection after projectile motion.
- Building collision surface sets.
- Render/projectile overlay projection.
- App receipts or debug HUD presentation.

## Reads

- Incoming projectile state, motion params, collision surface pointer, and delta seconds.
- `SpatialSurfaceSet` through `querySegment` with `CollisionQueryKind::Projectile`.

## Writes / Mutates

- Does not mutate caller-owned state directly.
- Returns previous state and updated projectile state in `ProjectileStepResult`.
- Marks the returned projectile inactive on surface impact, expiration, inactive input state, lifetime expiry, or max-distance expiry.

## Calls Out To / Wires Out To

- Calls `querySegment(...)` from runtime collision to test projectile blockers.
- Uses core vector math for ballistic integration and distance measurement.
- Feeds `AbilitySystem` through `stepProjectile(...)`; ability code owns entity impact and combat damage after this step.

## Called By / Entry Points

- `projectileStepStatusName(...)`
- `stepProjectile(...)`
- `AbilitySystem::tickAbilityRuntime` is the current runtime ability consumer.

## Invariants

- Invalid state, invalid params, non-finite delta, or negative delta return `projectile_invalid_input`.
- Missing collision surfaces return `projectile_missing_collision_surfaces`.
- Inactive or already expired input state returns expired/inactive status without advancing.
- Gravity integrates position and velocity for the clamped step duration.
- Max lifetime clamps step duration; max distance clamps travel fraction.
- Projectile surface impact uses `CollisionQueryKind::Projectile`, copies collision hit metadata, and deactivates the returned state.
- Collision invalid input maps back to projectile invalid input.
- No persistent cache or cross-step ownership belongs here.

## Tests / Proof Commands

- `rg -n "projectile_system_tests|stepProjectile|ProjectileStepStatus|projectile_impact|projectile_expired" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `projectile_system_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/ability/AbilitySystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/CollisionQuery.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/SpatialSurfaceSet.*`
- `/Users/kogaryu/iggy3d/src/projection/scene/*`

## Update When

- Projectile params, integration math, lifetime/distance clamp policy, collision query kind, result fields, or reason codes change.

## Do Not Update When

- Only ability resource rules, entity impact damage, surface-set construction, or render overlay projection changes.
