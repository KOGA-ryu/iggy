# Physics Lab Contract v0.1

This document defines the first no-window physics lab for Iggy3D.
It depends on:

- `physics_math_foundation_v0_1.md`;
- `physics_step_pipeline_v0_1.md`;
- `physics_collision_queries_v0_1.md`;
- `physics_material_traits_v0_1.md`.

The lab is not a product UI. It is a deterministic proof surface for physics
behavior before the engine is wired into gameplay, AppShell, renderer/Vulkan,
save/load, or editor tools.

## Purpose

The physics lab should answer one question:

```text
Did this physics rule work under a known scenario?
```

It should prove that answer with no-window runs, stable counters, and readable
receipt fields.

The first lab scenarios are:

```text
drop
slide
wall_hit
stack
moving_platform
room_collision
```

Each scenario should be small, deterministic, and runnable from unit or smoke
tests without creating a window.

## Ownership

The lab may own:

- scenario setup structs;
- expected body/material/shape setup;
- fixed step count;
- physics result summary;
- debug snapshot selection;
- no-window receipt fields.

The lab must not own:

- AppShell routing;
- product menu state;
- Vulkan draw logic;
- renderer upload behavior;
- save/load persistence;
- ASCII parsing;
- editor cursor controls;
- controller mappings;
- gameplay command admission.

Product UI can later show lab results, but lab truth should come from runtime
physics state and physics snapshots.

## Lab Request Shape

The lab should accept a named scenario and fixed-step parameters.

Expected model:

```cpp
enum class PhysicsLabScenario {
  Drop,
  Slide,
  WallHit,
  Stack,
  MovingPlatform,
  RoomCollision,
};

struct PhysicsLabRunRequest {
  PhysicsLabScenario scenario;
  std::uint32_t fixedStepCount = 0;
  float fixedDtSeconds = 1.0F / 60.0F;
  bool collectSnapshots = true;
};
```

Scenario selection is a table problem:

```text
drop -> buildDropScenario
slide -> buildSlideScenario
wall_hit -> buildWallHitScenario
stack -> buildStackScenario
moving_platform -> buildMovingPlatformScenario
room_collision -> buildRoomCollisionScenario
```

Do not implement the lab as a long scenario if-chain.

## Lab Result Shape

The lab should return facts that tests can assert.

Expected model:

```cpp
struct PhysicsLabRunResult {
  bool ok = false;
  std::string status = "physics_lab_not_run";
  std::string reasonCode = "physics_lab_not_run";
  std::string scenarioName;
  std::uint32_t requestedStepCount = 0;
  std::uint32_t executedStepCount = 0;
  std::uint32_t bodyCount = 0;
  std::uint32_t dynamicBodyCount = 0;
  std::uint32_t staticBodyCount = 0;
  std::uint32_t contactCount = 0;
  std::uint32_t solvedContactCount = 0;
  std::uint32_t broadphasePairCount = 0;
  float maxPenetrationDepthMeters = 0.0F;
  float largestVelocityMetersPerSecond = 0.0F;
};
```

Specialized scenario facts can be added around this shared result.

Examples:

```text
drop_final_y
drop_grounded
slide_final_x
slide_velocity_reduced
wall_hit_blocked
stack_top_body_supported
platform_body_carried
room_collision_wall_blocked
```

## Receipt Policy

No-window lab proof should use stable key/value receipt fields.

Common fields:

```text
physics_lab_requested
physics_lab_loaded
physics_lab_status
physics_lab_reason_code
physics_lab_scenario
physics_lab_requested_step_count
physics_lab_executed_step_count
physics_lab_body_count
physics_lab_dynamic_body_count
physics_lab_static_body_count
physics_lab_broadphase_pair_count
physics_lab_contact_count
physics_lab_solved_contact_count
physics_lab_max_penetration_depth_meters
physics_lab_largest_velocity_meters_per_second
```

Scenario-specific fields should use scenario prefixes:

```text
physics_lab_drop_*
physics_lab_slide_*
physics_lab_wall_hit_*
physics_lab_stack_*
physics_lab_moving_platform_*
physics_lab_room_collision_*
```

Do not emit full snapshot dumps as receipts. Receipts should prove the result,
not become a physics trace file.

## Scenario 1: Drop

Purpose:

- prove gravity and fixed-step integration;
- prove ground collision/resting contact once collision exists;
- prove no render or AppShell dependency.

Setup:

```text
static floor at y = 0
dynamic body above floor
gravity enabled
material = debug_floor / stone or generic dynamic material
```

Expected early proof before contacts exist:

```text
body y decreases after N steps
velocity y becomes negative
step count is exact
```

Expected proof after contacts exist:

```text
body rests on floor
final y is stable
grounded = true
contact_count > 0
max_penetration_depth is bounded
```

Expensive math should not be needed beyond validation-safe normalization where
contacts require normals.

## Scenario 2: Slide

Purpose:

- prove friction/material pair behavior;
- prove damping or friction reduces horizontal motion;
- prove material data is not hardcoded in solver branches.

Setup:

```text
static floor
dynamic body with initial horizontal velocity
material pair: body material vs floor material
```

Expected proof:

```text
body moves horizontally
horizontal speed decreases over time
ice-like material loses speed slower than stone-like material
friction pair table was used
```

The lab should compare two table-driven material cases rather than branch on
material names inside the scenario loop.

## Scenario 3: Wall Hit

Purpose:

- prove swept movement or contact blocking against a wall slab;
- prove collision uses room/static primitives, not render triangles;
- prove bounce/rest behavior is material-driven.

Setup:

```text
static wall slab
dynamic body moving toward wall
optional restitution material pair
```

Expected proof:

```text
body does not pass through wall
hit normal points away from wall
time_of_impact is within [0, 1] for swept movement
contact_count > 0 after contact generation exists
```

If restitution is enabled:

```text
post_hit_velocity reflects away from wall
restitution value comes from material pair table
```

## Scenario 4: Stack

Purpose:

- prove multiple contacts can settle;
- prove solver iteration count matters and is visible;
- prove stable ordering for body/contact processing.

Setup:

```text
static floor
two or three dynamic boxes vertically stacked
gravity enabled
```

Expected proof:

```text
bottom body rests on floor
upper body rests on lower body
stack remains bounded after N steps
max_penetration_depth remains below threshold
solved_contact_count >= contact_count or reports bounded unsolved contacts
```

This is the first place solver weakness will show. Keep it small.

Do not begin with tall stacks.

## Scenario 5: Moving Platform

Purpose:

- prove kinematic body interaction;
- prove carried body/platform contact behavior;
- prove physics can support game-feeling traversal later.

Setup:

```text
kinematic platform with scripted velocity
dynamic body resting on platform
gravity enabled
```

Expected proof:

```text
platform moves through kinematic authority
dynamic body remains supported
dynamic body horizontal position follows platform within tolerance
contact remains stable
```

The platform path should be a scenario input table or scripted profile. Do not
hardcode platform behavior in the solver.

## Scenario 6: Room Collision

Purpose:

- prove authored room floor/wall collision conversion;
- prove room-scale collision uses static primitives;
- prove dynamic body or player-like body cannot pass through generated walls.

Setup:

```text
small authored room
floor spans
wall slabs
dynamic capsule or AABB body
movement/sweep toward wall
ground check on floor
```

Expected proof:

```text
ground check succeeds on floor
wall sweep hits before crossing wall
body remains inside walkable room area
checked static surface count is visible
renderer mesh triangle count is irrelevant to collision result
```

This scenario is where the physics lab meets the dungeon authoring pipeline.
It must still avoid AppShell.

## No-Window Execution

The first lab should be runnable from tests only.

Later, a product no-window automation path may be added, but only after runtime
lab models exist.

Possible future command shape:

```text
--physics-lab drop
--physics-lab-steps 120
--print-physics-lab-receipt
```

That command shape is deferred. Do not block source work on CLI polish.

## Debug Snapshot Use

Each lab run may collect snapshots at:

```text
first step
last step
contact begin
contact max penetration
```

Snapshots should be summarized, not dumped wholesale.

Useful snapshot counters:

```text
body_count
active_body_count
contact_count
max_penetration_depth
largest_velocity
broadphase_pair_count
invalid_body_count
```

## Performance Counters

Every lab case should expose enough counters to catch accidental expensive work.

Required counters once implemented:

```text
broadphase_pair_count
narrowphase_candidate_count
contact_count
solver_iteration_count
checked_static_surface_count
checked_dynamic_pair_count
```

These counters make it harder for all-pairs collision or all-surface scans to
slip in quietly.

## Determinism Policy

Lab cases must be deterministic enough for focused tests.

Rules:

- fixed step count;
- fixed dt;
- stable body insertion order;
- stable material table;
- stable candidate/contact ordering;
- no random values unless seeded and reported;
- no wall-clock timing dependency.

The lab does not need cross-platform bit-perfect guarantees at v0.1.

## First Source Slice Fit

The first lab source slice should come after at least the basic fixed-step body
store exists.

Expected first lab source paths:

```text
src/runtime/physics/PhysicsLab.*
tests/unit/physics_lab_tests.cpp
```

Allowed first behavior:

- build lab scenario descriptors;
- run the current physics step for fixed N ticks;
- prove the drop scenario before collision exists;
- return shared lab result facts.

Not allowed in first lab source slice:

- AppShell;
- product UI;
- renderer/Vulkan;
- save/load;
- CLI command plumbing;
- ASCII parser changes;
- full stack/platform/room collision before those physics systems exist.

## Build Order Relationship

The lab should grow with the physics engine:

```text
1. drop without collision: gravity/integration proof
2. drop with floor: contact/resting proof
3. slide: friction/material proof
4. wall_hit: sweep/contact proof
5. stack: solver stability proof
6. moving_platform: kinematic support proof
7. room_collision: authored room static collision proof
```

Each lab scenario should land only when the runtime system it proves exists.

## Acceptance Gate

This document is accepted when:

- the six lab scenarios are named and bounded;
- no-window proof fields are defined;
- lab ownership is separate from AppShell/product UI/rendering;
- the lab grows alongside runtime physics instead of faking future behavior;
- performance counters are required before expensive collision work can hide.

