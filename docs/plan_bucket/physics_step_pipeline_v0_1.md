# Physics Step Pipeline v0.1

This document defines the first fixed-step physics pipeline for Iggy3D.
It depends on `physics_math_foundation_v0_1.md` and uses those math rules as
the cost boundary for every hot loop.

The goal is a deterministic-enough, cheap, inspectable pipeline that can support
first-person movement, editable dungeon rooms, object interaction, and later
material/system behavior without moving physics logic into AppShell, rendering,
or one-off gameplay branches.

## Research Baseline

The pipeline should be modeled after real-time game rigid body engines, not
offline simulation.

Primary references:

- Gaffer on Games, "Fix Your Timestep" for fixed-step accumulator policy.
- Gaffer on Games, "Integration Basics" for explicit versus semi-implicit Euler
  tradeoffs.
- Erin Catto, "Fast and Simple Physics using Sequential Impulses" for the first
  solver baseline.
- Erin Catto, "Solver2D" for PGS, TGS, XPBD comparisons, warm starting,
  accumulated impulses, and the practical solver testbed mindset.
- Erin Catto, "Releasing Box2D 3.0" for modern notes on data-oriented design,
  SIMD, persistent islands, graph coloring, soft step/TGS-style stability, and
  benchmarking.
- Macklin et al., "Small Steps in Physics Simulation" for substep reasoning.
- Macklin, Muller, and Chentanez, "XPBD" for compliant constraints later.

Reference URLs:

- https://gafferongames.com/post/fix_your_timestep/
- https://gafferongames.com/post/integration_basics/
- https://box2d.org/files/ErinCatto_SequentialImpulses_GDC2006.pdf
- https://box2d.org/posts/2024/02/solver2d/
- https://box2d.org/posts/2024/08/releasing-box2d-3.0/
- https://mmacklin.com/smallsteps.pdf
- https://matthias-research.github.io/pages/publications/XPBD.pdf

## Pipeline Summary

Every physics update should run through the same fixed order:

```text
1. collect step input
2. apply forces
3. integrate velocity
4. rebuild broadphase
5. generate narrowphase contacts
6. solve contacts and constraints
7. integrate position
8. publish snapshot/debug facts
```

The first source slice may implement only a subset of this order, but it must
reserve the full shape so later collision and solver work slots in without
rewriting the public API.

Target pseudocode:

```cpp
PhysicsStepResult stepPhysicsWorld(const PhysicsStepRequest& request) {
  validateStepRequest(request);
  beginStepScratch(request.world);

  for (std::uint32_t substep = 0; substep < request.substepCount; ++substep) {
    clearPerSubstepForcesAndContacts();
    applyForces(request.world, request.substepDtSeconds);
    integrateVelocities(request.world, request.substepDtSeconds);
    buildBroadphase(request.world);
    buildContacts(request.world);
    buildIslands(request.world);
    warmStartContacts(request.world);
    solveVelocityConstraints(request.world, request.velocityIterations);
    solvePositionConstraints(request.world, request.positionIterations);
    integratePositions(request.world, request.substepDtSeconds);
    updateSleepingState(request.world);
  }

  publishStepSnapshot(request.world);
  return buildStepResult(request.world);
}
```

The first source slice may stub broadphase/contact/solver phases, but the public
step result should already expose counters for them.

## Fixed Timestep

Physics must not use raw render frame delta as simulation truth.

The runtime should use a fixed step:

```text
fixed_dt_seconds = 1 / 60
max_substeps_per_frame = 4
```

The product/window layer may accumulate frame time and request steps, but the
runtime physics layer should receive a count of fixed steps or a single fixed
step request. It should not know about window frame timing.

Expected accumulator policy outside physics:

```text
accumulator += frame_dt
substeps = min(floor(accumulator / fixed_dt), max_substeps)
accumulator -= substeps * fixed_dt
```

If the frame stalls beyond the cap, the app may drop extra accumulated time or
report a slow-frame/debug status. Physics should not stretch the fixed step to
catch up.

The physics layer has two distinct time concepts:

```text
external fixed step h = 1 / 60
internal solver substep h_sub = h / substep_count
```

For v0.1:

```text
substep_count = 1
```

For later TGS-style testing:

```text
substep_count = 2 or 4
```

Do not confuse this with variable frame delta. TGS-style substeps divide a fixed
physics step. They do not make simulation frame-rate dependent.

## Runtime Ownership

Physics owns:

- body positions used by dynamic physics bodies;
- body velocities;
- accumulated forces/impulses for the current step;
- body AABBs;
- broadphase buckets;
- contact pairs;
- contact manifolds;
- solver scratch data;
- physics debug snapshot facts.

Physics does not own:

- authored room documents;
- ASCII source;
- product UI;
- AppShell routing;
- Vulkan draw commands;
- save/load orchestration;
- gameplay command admission;
- controller mapping;
- editor cursor state.

Those systems may request physics work or consume physics results. They must not
become physics logic owners.

## Step Input

The fixed step should consume a compact request:

```cpp
struct PhysicsStepRequest {
  PhysicsWorld* world = nullptr;
  float fixedDtSeconds = 1.0F / 60.0F;
  std::uint32_t substepCount = 1;
  std::uint32_t velocityIterations = 6;
  std::uint32_t positionIterations = 2;
  bool enableGravity = true;
  bool enableWarmStarting = false;
  bool enableSleeping = false;
  bool rebuildBroadphaseEachSubstep = true;
  bool publishDebugSnapshot = false;
};
```

The first implementation can keep this narrower, but the names and ownership
should stay close to this shape.

Validation guard examples:

- missing world rejects before mutation;
- nonfinite or nonpositive fixed dt rejects before mutation;
- zero substeps rejects before mutation;
- zero solver iterations are allowed only before contacts exist;
- body arrays must have matching lengths;
- nonfinite body position/velocity fails closed for that body or the step,
  depending on the corruption policy chosen in source.

## Step Output

The fixed step should return facts, not strings of hidden behavior.

Expected result shape:

```cpp
struct PhysicsStepResult {
  bool ok = false;
  std::string status = "physics_step_not_run";
  std::string reasonCode = "physics_step_not_run";
  std::uint32_t bodyCount = 0;
  std::uint32_t activeDynamicBodyCount = 0;
  std::uint32_t broadphasePairCount = 0;
  std::uint32_t broadphaseCellCount = 0;
  std::uint32_t maxBroadphaseBucketSize = 0;
  std::uint32_t narrowphaseCandidateCount = 0;
  std::uint32_t contactCount = 0;
  std::uint32_t solvedContactCount = 0;
  std::uint32_t islandCount = 0;
  std::uint32_t sleepingBodyCount = 0;
  std::uint32_t warmStartedContactCount = 0;
  std::uint32_t velocityIterationCount = 0;
  std::uint32_t positionIterationCount = 0;
  float maxPenetrationDepthMeters = 0.0F;
  float largestVelocityMetersPerSecond = 0.0F;
};
```

These counters matter because they show whether the cheap pipeline is still
cheap as maps get larger.

The result should be cheap to compute. It should summarize counters already
known during the step, not scan the whole world again after the fact.

## Phase 1: Apply Forces

Purpose:

- clear previous per-step force accumulators;
- apply gravity to dynamic bodies;
- apply queued impulses or authored forces;
- skip sleeping/static/kinematic bodies.

Cheap form:

```cpp
velocity += gravity * fixedDt;
```

Force equation:

```text
F_total = gravity_force + authored_force + impulse_force
a = F_total * inverse_mass
v_next = v + a * h
```

For gravity:

```text
gravity_force = mass * gravity
a_gravity = gravity
```

So gravity should be applied directly as acceleration:

```cpp
velocity += gravity * h;
```

Do not multiply gravity by mass and then divide by mass in the hot body loop.

Force accumulation should use body indices and arrays. It should not route by
string object names in the hot loop.

Gravity should be table/config data:

```text
gravity = (0, -9.8, 0)
```

Different world zones or materials may alter gravity later, but not through
AppShell branches.

## Phase 2: Integrate Velocity

Purpose:

- apply damping;
- clamp extreme velocity if needed;
- calculate predicted movement for broadphase.

Cheap form:

```cpp
velocity *= dampingPerTick;
predictedDelta = velocity * fixedDt;
```

Semi-implicit Euler order:

```text
v(t + h) = v(t) + h * a(t)
x(t + h) = x(t) + h * v(t + h)
```

This is the baseline because it is cheap and has better energy behavior than
explicit Euler for game-scale dynamics.

Velocity limits should be explicit config:

```text
max_linear_speed
max_angular_speed later
```

If a body exceeds the limit, clamp with length-squared precheck:

```cpp
if (dot(v, v) > maxSpeed * maxSpeed) {
  v = normalizeSafe(v) * maxSpeed;
}
```

This is an approved `sqrt` path because the body has already exceeded a bounded
speed gate and normalization is required.

The first implementation can skip damping if no material/profile exists yet.
If damping exists, use precomputed fixed-step damping scalars from profile data.

Do not use `exp` in the body loop.

## Phase 3: Rebuild Broadphase

Purpose:

- compute each active body's AABB;
- expand AABB by predicted movement when useful;
- insert body ids into spatial hash cells;
- emit candidate body pairs.

First broadphase type:

```text
uniform spatial hash grid
```

Reason:

- good enough for room-scale dungeon play;
- easy to debug;
- works with editable rooms;
- data-oriented;
- does not need dynamic tree maintenance yet.

Input data:

- positions;
- shape ids;
- predicted deltas;
- active flags;
- static room collision primitives.

Output data:

- cell occupancy;
- candidate body pairs;
- candidate static surface pairs.

Rules:

- use integer cell keys;
- use body indices;
- do not allocate per pair in hot loops;
- do not compare strings;
- no all-body O(n^2) pass except tiny unit tests.

First implementation can rebuild broadphase every fixed step. That is simpler
and safer for editable dungeon rooms.

Future optimization:

```text
incremental broadphase update
persistent dynamic AABB tree
static-room BVH or chunk grid
```

Do not add those until counters prove the uniform grid rebuild is the bottleneck.

Required broadphase counters:

```text
body_aabb_count
static_surface_aabb_count
occupied_cell_count
max_bucket_size
candidate_pair_count
duplicate_pair_rejected_count
```

## Phase 4: Generate Narrowphase Contacts

Purpose:

- take broadphase candidates;
- perform shape-specific tests;
- build contact manifolds for actual overlaps or swept hits.

First shape pairs:

```text
dynamic AABB/body vs static wall slab
dynamic AABB/body vs floor plane/span
dynamic AABB vs dynamic AABB
```

Deferred shape pairs:

```text
capsule vs mesh
sphere vs triangle mesh
convex hull
compound shapes
character controller special casing
```

Narrowphase can use dispatch tables by shape pair:

```text
(box, box) -> collideBoxBox
(box, wall_slab) -> collideBoxWallSlab
(box, floor_span) -> collideBoxFloorSpan
```

Do not build this as a growing branch ladder in the step loop.

Contact manifold shape:

```cpp
struct PhysicsContactPoint {
  Vec3 point;
  Vec3 normal;
  float separationMeters = 0.0F;
  float normalImpulse = 0.0F;
  float tangentImpulse0 = 0.0F;
  float tangentImpulse1 = 0.0F;
};

struct PhysicsContactManifold {
  PhysicsBodyId bodyA;
  PhysicsBodyId bodyB;
  PhysicsShapeId shapeA;
  PhysicsShapeId shapeB;
  MaterialPairId materialPair;
  std::uint32_t pointCount = 0;
  PhysicsContactPoint points[4];
};
```

For first proof, one contact point is enough. Keep the struct ready for more
points so box stacking does not force an API rewrite.

Persistent contact identity matters for warm starting. Each contact point should
have a stable feature id when the narrowphase can provide one:

```text
feature_id = face/edge/vertex pair or source surface id + local cell id
```

If the first narrowphase cannot provide a feature id, warm starting should stay
disabled.

## Phase 4B: Build Islands

Purpose:

- group connected awake dynamic bodies and constraints;
- skip independent sleeping islands;
- make later multithreading and graph coloring possible;
- keep solver ordering explicit.

Island graph:

```text
nodes = active dynamic bodies
edges = contacts and joints
```

First implementation can solve one global island, but the public step result
should still expose:

```text
island_count
largest_island_body_count
largest_island_contact_count
```

Island building should use body ids and contact arrays. It should not walk
product/gameplay objects.

Sleeping policy should be conservative:

```text
sleep disabled by default for first source slice
sleep allowed only after lab proves resting contacts
```

Sleep criteria later:

```text
linear_speed_squared < threshold_squared for N ticks
angular_speed_squared < threshold_squared for N ticks
no external impulses
island remains stable
```

## Phase 5: Solve Contacts And Constraints

Purpose:

- prevent bodies from interpenetrating;
- apply normal impulses;
- apply friction impulses;
- apply restitution when configured;
- preserve static and kinematic body authority.

First solver:

```text
sequential impulse solver
```

Fixed iteration counts:

```text
velocity_iterations = 6
position_iterations = 2
```

The exact first values are less important than the rule: iteration counts are
explicit config and visible in step results.

Solver input:

- body velocities;
- inverse masses;
- contact normals;
- penetration depths;
- material friction/restitution;
- warm-start impulse cache when enabled.

Solver output:

- corrected velocities;
- optional position correction;
- solved contact counts.

Do not solve exact global equations for the first engine layer. That is too much
math, too much complexity, and not needed for room-scale gameplay.

Velocity solver order:

```text
1. optional warm start
2. normal impulses
3. tangent/friction impulses
4. restitution only when normal velocity exceeds bounce threshold
```

Warm starting:

```text
previous accumulated impulses are applied before iterations
```

Reason:

- resting stacks converge faster;
- contacts jitter less;
- fewer iterations are needed for stable scenes.

Accumulated impulse clamp:

```cpp
const float incrementalImpulse = -effectiveMass * relativeNormalVelocity;
const float newImpulse = max(0.0F, oldImpulse + incrementalImpulse);
const float appliedImpulse = newImpulse - oldImpulse;
oldImpulse = newImpulse;
```

Clamp accumulated impulse, not just the current iteration's impulse. This is
important because a later iteration must be able to reduce the total impulse.

Friction clamp:

```text
abs(tangent_impulse_accum) <= friction * normal_impulse_accum
```

Position correction options:

```text
Baumgarte bias in velocity solve
nonlinear position solve after velocity solve
TGS substeps later
XPBD compliance later
```

First source solver should use the simplest velocity-level bias. Lab tests must
record max penetration so poor tuning is visible.

TGS note:

When substeps are enabled, the expensive version would rebuild broadphase and
contacts every substep. A more advanced TGS path can keep contact anchors in
local body coordinates and update separation during substeps. That optimization
is deferred until baseline contacts are proven.

## Phase 6: Integrate Position

Purpose:

- move dynamic bodies using solved velocities;
- write previous/current positions;
- update final AABBs;
- keep kinematic/static bodies under their owners' control.

Cheap form:

```cpp
previousPosition = position;
position += velocity * fixedDt;
```

If position correction exists, apply it after velocity solve and before snapshot
publication.

Position integration should happen after the velocity solve so contact impulses
affect the final movement of the same tick.

For static bodies:

```text
position unchanged
velocity ignored or zero
inverse_mass = 0
```

For kinematic bodies:

```text
position follows authored/scripted kinematic target
dynamic bodies may react to kinematic body motion
kinematic bodies do not receive solver impulses
```

Moving platforms belong in the kinematic path, not in one-off player movement
branches.

## Phase 7: Publish Snapshot

Purpose:

- expose debug facts without forcing product UI or renderer coupling;
- make no-window tests prove the physics pipeline;
- support future lab panels and world-space debug draw.

Expected snapshot facts:

```text
source_tick
body_count
dynamic_body_count
sleeping_body_count
broadphase_pair_count
narrowphase_candidate_count
contact_count
solved_contact_count
warm_started_contact_count
island_count
max_penetration_depth
largest_velocity
invalid_body_count
```

Snapshots are read-only product/debug inputs. They must not mutate physics state.

Projection/render/debug may consume snapshots later. Physics must not call
renderer code.

Snapshot rows should be sampled, not dumped unbounded:

```text
first N bodies by id
first N contacts by id/order
worst penetration contact
largest velocity body
largest island
```

This keeps receipts and no-window proof readable.

## Determinism Expectations

The first goal is deterministic-enough repeatability for tests and editor
proofs, not cross-platform bit-perfect physics.

Required:

- fixed dt;
- stable body iteration order;
- stable broadphase candidate sort/order;
- stable contact order before solving;
- stable island order before solving;
- no unordered-map iteration leakage into solver order unless sorted;
- no frame-delta-dependent force scaling.

Deferred:

- cross-CPU bit-exact determinism;
- rollback netcode determinism;
- parallel solver determinism.

## Data-Oriented Boundary

Hot pipeline data should be SoA:

```text
positions[]
previous_positions[]
velocities[]
forces_or_accelerations[]
inverse_masses[]
shape_ids[]
material_ids[]
flags[]
aabbs[]
sleep_counters[]
```

Cold/editor/config data can be AoS:

```text
PhysicsBodyDesc
PhysicsShapeDesc
PhysicsMaterialDesc
PhysicsWorldDesc
```

Conversion from authored/config data into hot arrays should happen at load,
spawn, or editor-confirm boundaries, not inside the fixed step.

## Branch And Table Policy

Good branches inside the pipeline:

- body disabled/static skip;
- nonfinite guard;
- missing shape/material guard;
- no candidates/contact early return;
- invalid request fail before mutation;
- fail-closed corrupt contact data.

Mappings that must be table-driven:

- shape pair to narrowphase function;
- material pair to friction/restitution mix rule;
- body type to integration policy;
- debug counter name to receipt/debug emission later.

Do not add per-object physics behavior branches in the pipeline.

## First Source Slice Fit

The first source slice should implement the pipeline shell without collision:

```text
PhysicsTypes.*
PhysicsBodyStore.*
PhysicsStep.*
physics_step_tests
```

Allowed first behavior:

- validate request;
- store body arrays;
- apply gravity;
- integrate velocity;
- integrate position;
- publish basic step counters;
- prove fixed-step repeatability.

Not allowed in the first source slice:

- broadphase implementation;
- narrowphase implementation;
- contacts;
- solver;
- static room collision extraction;
- AppShell/product UI integration;
- renderer/Vulkan integration;
- save/load integration.

The API should still leave named slots for broadphase, narrowphase, solver, and
snapshot counters so later slices add implementation without replacing the
pipeline shape.

## Implementation Slice Ladder

The pipeline should be implemented in this order:

```text
1. body store + fixed-step integration, no collision
2. AABB shape data + AABB generation
3. broadphase uniform grid counters, no contacts
4. narrowphase AABB/static floor/wall contacts
5. sequential impulse normal solve
6. friction and restitution material-pair solve
7. warm starting with contact feature ids
8. sleeping and island counters
9. TGS substep lab experiment
10. kinematic moving platform proof
```

Each slice must add counters before adding complexity. If a phase cannot be
measured, it is not ready to become product behavior.

## Acceptance Gate

This document is accepted when:

- it defines the fixed tick order;
- it keeps physics independent from AppShell and rendering;
- it names the data each stage consumes and produces;
- it preserves cheap math rules from `physics_math_foundation_v0_1.md`;
- it gives the first source slice a small implementation boundary.
