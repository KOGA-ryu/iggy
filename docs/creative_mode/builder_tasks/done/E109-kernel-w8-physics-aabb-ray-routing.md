# E109: Kernel W8 - Route Physics AABB Raycasts Through Core Ray/AABB

## Objective

Route the private physics AABB ray slab test through the core `Ray3` /
`AabbRayHit` primitive without changing physics query behavior.

This is a kernel-use card, not a feature card. The goal is to retire duplicated
ray/AABB intersection math from `runtime/physics` while keeping physics-owned
policy local.

## Current Seam

Known duplicate path:

- `src/runtime/physics/PhysicsCollisionQueries.cpp`
  - private `SlabRayHit`
  - private `raycastAabb(...)`
  - `raycastPhysicsAabbs(...)`
  - `sweepPhysicsAabb(...)`

Core primitive already exists:

- `src/core/math/Ray3.hpp`
- `src/core/math/Aabb3.hpp/.cpp`
  - `AabbRayHit`
  - `intersectsRay(const Aabb3&, Ray3, float)`

Existing focused tests:

- `tests/unit/aabb_ray_tests.cpp`
- `tests/unit/physics_collision_queries_tests.cpp`

## Required Ownership Boundary

Core owns:

- ray/AABB hit/miss;
- hit distance;
- hit point;
- start-inside detection;
- invalid ray/bounds/max-distance rejection.

Physics keeps owning:

- collider request validation;
- sensor filtering;
- collider/body id result shape;
- result sorting/tie policy;
- swept AABB expansion;
- hit normal semantics (`normalFromColliderToRay` and
  `normalFromColliderToMovingAabb`);
- ground-check policy.

If the current core primitive cannot preserve physics behavior, stop and move
this card to `blocked/` with the exact mismatch. Do not add a second private
slab helper under a different name.

## Implementation Guidance

1. First add or strengthen guard tests in
   `tests/unit/physics_collision_queries_tests.cpp` if needed:
   - nearest hit distance/point still match current behavior;
   - normal on an axis hit remains the existing physics normal;
   - start-inside hit keeps zero distance, origin point, `startInside=true`,
     and zero normal;
   - swept AABB still reports the same fraction/normal for an expanded target;
   - non-unit ray directions are still reported in meters, not raw parameter
     units.
2. Replace the private physics slab distance/point/start-inside math with
   `intersectsRay(...)`.
3. Keep only the minimum physics-local wrapper needed to derive the physics hit
   normal. If normal derivation still needs axis-slab logic, isolate it as
   normal policy and make clear in code that core intersection remains the
   source of hit distance/point.
4. Do not change public APIs or receipt/status strings.
5. Do not widen into broad physics, kinematic motor, render, RoomBake, creative,
   save/load, or standalone behavior.

## Required Behavior

- `raycastPhysicsAabbs(...)` returns the same hit count, ordering, distances,
  points, normals, body ids, sensor flags, and start-inside flags as before.
- `sweepPhysicsAabb(...)` keeps the same hit ordering, distance, fraction,
  center, point, normal, sensor, and initial-overlap behavior.
- The core `AabbRayHit` path is used for shared intersection math.
- No duplicate private full ray/AABB slab algorithm remains in physics.

## Do Not

- Do not change `AabbRayHit` semantics unless a failing guard proves a core
  contract bug and the fix is tightly scoped.
- Do not add an app-local or physics-local replacement kernel.
- Do not stage, commit, push, launch a window, or run broad CTest.
- Do not touch docs outside this task card and `PRIORITY.md`.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d aabb_ray_tests physics_collision_queries_tests physics_kinematic_motor_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(aabb_ray_tests|physics_collision_queries_tests|physics_kinematic_motor_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Kernel routing shape:
- Physics policy kept local:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/runtime/physics/PhysicsCollisionQueries.cpp`
  - `tests/unit/physics_collision_queries_tests.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E109-kernel-w8-physics-aabb-ray-routing.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Kernel routing shape:
  - `intersectPhysicsRayAabb(...)` now calls core
    `intersectsRay(bounds, Ray3{origin, normalizedDirection}, maxDistance)`.
  - Core `AabbRayHit` owns hit/miss, meter distance, point, start-inside, and
    invalid input handling for physics raycasts and non-zero sweeps.
  - Removed the old private `SlabRayHit` name and private full slab hit routine.
- Physics policy kept local:
  - Request validation, collider validation, sensor filtering, sorting/tie
    policy, swept AABB expansion, ground-check behavior, and result shape remain
    in physics.
  - Physics-local `normalFromColliderToRay(...)` preserves the legacy face-normal
    tie policy. It uses the core hit distance/point as truth and only derives the
    normal field that core does not expose.
- Behavior preserved:
  - Existing raycast hit ordering, start-inside zero-distance/zero-normal, axis
    normals, sweep fractions, sweep normals, sensors, and ground checks remained
    green.
  - Added an explicit non-unit physics ray test proving reported distance and
    point stay in meters.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d aabb_ray_tests physics_collision_queries_tests physics_kinematic_motor_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(aabb_ray_tests|physics_collision_queries_tests|physics_kinematic_motor_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - Core still intentionally does not expose a hit normal. If more runtime
    systems need the same normal policy later, add a deliberate core/API shape
    rather than duplicating physics normal derivation elsewhere.
