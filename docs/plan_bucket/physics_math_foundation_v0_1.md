# Physics Math Foundation v0.1

This document defines the first math rules for a bespoke Iggy3D physics engine.
It is a technical foundation, not a slogan sheet. The point is to lock the
equations, approximation choices, data layout expectations, and approval
boundaries before runtime physics code exists.

The target is a game-feeling physics layer: stable, inspectable, editable, and
cheap enough to run while the player is building and testing rooms.

## Design Direction

Physics must be built as a runtime system, not as AppShell behavior and not as
render behavior.

The first implementation path should be:

```text
authored room/object data
  -> runtime body/shape/material data
  -> fixed physics step
  -> collision/contact results
  -> gameplay/session state
  -> projection/debug/render
```

Rendering can visualize physics. Rendering must not own physics truth.

ASCII can create map/layout source. ASCII must not define physics behavior,
material law, object interaction, or controller behavior. Those belong to
authored object/material/physics metadata layered on top of the room.

## Reference Baseline

The useful industry baseline is not exact simulation. The useful baseline is
stable approximation with strong tools.

- Fixed timestep simulation keeps physics from changing with render framerate.
  Reference: Gaffer on Games, "Fix Your Timestep".
- Sequential impulses are a practical way to solve contacts and constraints in
  real time.
  Reference: Erin Catto, "Fast and Simple Physics using Sequential Impulses".
- Collision detection should reject most work with broadphase and cheap
  primitive tests before narrowphase.
  Reference: Christer Ericson, "Real-Time Collision Detection".
- Nintendo's recent systemic game design direction is useful as a goal shape:
  object behavior should come from composable physics/material/system rules, not
  one-off scripts per object.
  Reference: "Tunes of the Kingdom: Evolving Physics and Sounds".
- David Baraff's rigid body notes are the correct academic baseline for
  understanding equations of motion, contact forces, and constraint solving.
- Brian Mirtich's impulse-based rigid body work is a useful deeper reference for
  contact impulses and collision response.
- Macklin, Muller, and Chentanez's XPBD and "Small Steps" work is the relevant
  modern real-time direction for compliant constraints, stability, and solver
  substepping.

Reference URLs:

- https://gafferongames.com/post/fix_your_timestep/
- https://box2d.org/files/ErinCatto_SequentialImpulses_GDC2006.pdf
- https://box2d.org/files/ErinCatto_UnderstandingConstraints_GDC2014.pdf
- https://realtimecollisiondetection.net/
- https://gdcvault.com/play/1034667/Tunes-of-the-Kingdom-Evolving
- https://www.cs.cmu.edu/~baraff/sigcourse/
- https://people.eecs.berkeley.edu/~jfc/mirtich/impulse.html
- https://matthias-research.github.io/pages/publications/XPBD.pdf
- https://mmacklin.com/smallsteps.pdf

## Engine Math Target

Iggy3D should not begin with a full research simulator. It should begin with a
modern real-time game physics stack that can later absorb better solvers.

The target stack is:

```text
rigid bodies
  -> simple convex/static shapes
  -> broadphase
  -> narrowphase contacts
  -> sequential impulse / PGS baseline
  -> TGS-style substep option later
  -> XPBD/compliance option later for joints/tools
```

Terms:

- PGS: Projected Gauss-Seidel. Iteratively solves constraints one at a time and
  clamps impulses into valid ranges.
- Sequential impulse: game-friendly PGS phrased as impulses applied to body
  velocities.
- TGS: Temporal Gauss-Seidel. Uses smaller substeps to improve stability
  without simply adding more solver iterations.
- XPBD: Extended Position-Based Dynamics. Useful later for compliant joints,
  soft constraints, ropes, tools, and editor-friendly stable constraints.

First implementation choice:

```text
semi-implicit Euler + sequential impulses + fixed timestep
```

Deferred implementation choices:

```text
TGS substepping for stacks and moving platforms
XPBD compliance for joints, ropes, editor constraints, and toolable simulation
GJK/EPA for arbitrary convex shapes
dynamic AABB tree for large moving-object worlds
```

The reason is practical: the first dungeon/editor loop needs stable bodies,
walls, floors, slides, and stacks before it needs general convex collision or
deformable simulation.

## Rigid Body State

The physics state for a dynamic rigid body is:

```text
x      position
q      orientation quaternion
v      linear velocity
w      angular velocity
m      mass
I      inertia tensor
M^-1   inverse mass matrix
```

For the first Iggy3D source slice, orientation and angular velocity may be
reserved but not implemented. Dynamic falling/sliding AABBs can start with
translation-only bodies. The API should leave room for rotation so boxes and
props do not require a rewrite later.

Continuous equations:

```text
dx/dt = v
dv/dt = F / m
dq/dt = 0.5 * omega_quat * q
dw/dt = I^-1 * (tau - w x Iw)
```

First implementation translation-only form:

```text
v_next = v + h * inverse_mass * force
x_next = x + h * v_next
```

Where:

```text
h = fixed timestep seconds
```

This is semi-implicit Euler. It is cheap, simple, and better behaved for games
than explicit Euler because velocity is advanced before position.

Do not use variable `dt` from render frames as `h`.

## Constraint Equation Baseline

Most collision response can be framed as constraints.

General constraint:

```text
C(x) >= 0
```

Velocity-level constraint:

```text
Jv + b >= 0
```

Where:

```text
J = constraint Jacobian
v = velocity vector for involved bodies
b = bias term for penetration correction/restitution
```

Effective mass:

```text
K = J * M^-1 * J^T
```

Impulse solve:

```text
lambda = -(Jv + b) / K
```

Then clamp and apply:

```text
lambda_accum = clamp(lambda_accum + lambda, lower, upper)
delta_lambda = lambda_accum_new - lambda_accum_old
v += M^-1 * J^T * delta_lambda
```

This is the core sequential impulse loop. It gives us a data-oriented solver:
arrays of bodies, contacts, Jacobian-derived terms, accumulated impulses, and
material-pair limits.

## Contact Normal Impulse

For a simple contact between bodies A and B:

```text
ra = contact_point - center_of_mass_a
rb = contact_point - center_of_mass_b
relative_velocity = (vB + wB x rb) - (vA + wA x ra)
vn = dot(relative_velocity, normal)
```

For translation-only first proof, rotational terms are zero:

```text
relative_velocity = vB - vA
vn = dot(relative_velocity, normal)
```

Normal impulse magnitude:

```text
j = -(1 + e) * vn / effective_mass
```

Where:

```text
e = restitution
effective_mass = invMassA + invMassB + rotational_terms
```

Clamp:

```text
j >= 0
```

Reason: contact can push bodies apart, but it cannot pull them together.

For resting contacts, restitution should be disabled below a velocity threshold:

```text
if abs(vn) < restitution_velocity_threshold:
  e = 0
```

This prevents jitter on resting stacks.

## Penetration Correction

First solver can use Baumgarte-style velocity bias:

```text
b = beta / h * max(penetration_depth - slop, 0)
```

Typical first values:

```text
beta = 0.1 to 0.2
slop = 0.005m to 0.02m
```

Do not overcorrect penetration. Large beta values make stacks jitter and moving
platforms unstable.

Later TGS or XPBD can reduce reliance on aggressive Baumgarte correction.

## Friction Constraint

Tangential velocity:

```text
vt = relative_velocity - normal * dot(relative_velocity, normal)
```

Tangent direction:

```text
t = normalize_safe(vt)
```

Friction impulse:

```text
jt = -dot(relative_velocity, t) / effective_mass_tangent
```

Coulomb clamp:

```text
abs(jt_accum) <= mu * normal_impulse_accum
```

Where:

```text
mu = combined friction coefficient from material pair table
```

The material system should precompute friction pairs. The solver should not
branch on material names.

For the first proof, one tangent direction is acceptable. A full 3D solver can
use two orthogonal tangent directions:

```text
t1 = normalized tangent
t2 = cross(normal, t1)
```

## Solver Family Choice

### Baseline: Sequential Impulse / PGS

Use first.

Strengths:

- simple;
- known game baseline;
- works with fixed iteration count;
- easy to instrument;
- fits SoA and contact arrays;
- supports warm starting later.

Weaknesses:

- stacks can compress or jitter;
- convergence depends on iteration count;
- large mass ratios are hard;
- moving platforms need careful contact handling.

### Next: TGS-style substeps

Use after baseline contacts exist.

Idea:

```text
Instead of one large step with many solver iterations,
run smaller substeps with fewer iterations.
```

Why it matters:

- better contact stability;
- improved stacks;
- less explosive penetration correction;
- useful for first-person body/platform contact.

Initial policy:

```text
substeps = 1 by default
substeps = 2 or 4 for lab experiments
```

No product gameplay should depend on TGS until lab tests prove it.

### Later: XPBD Compliance

Use for constraints that designers/tools need to tune:

- ropes;
- springs;
- joints;
- grab tools;
- editor constraints;
- breakable constraints;
- soft constraints.

XPBD compliance equation shape:

```text
delta_lambda = (-C(x) - alpha_tilde * lambda) /
               (sum(w_i * |grad C_i|^2) + alpha_tilde)
```

Where:

```text
alpha_tilde = compliance / h^2
```

This lets stiffness remain more stable across timestep changes than old PBD.

Do not use XPBD for first floor/wall collision. It is a later constraint tool,
not the first contact foundation.

## Collision Algorithm Ladder

The collision ladder should be explicit:

```text
1. AABB validation
2. broadphase bucket candidate collection
3. AABB overlap / swept AABB
4. primitive narrowphase
5. convex narrowphase later
6. triangle mesh queries only for explicit special cases
```

First primitive narrowphase:

```text
AABB vs AABB
AABB/capsule vs floor span
AABB/capsule vs wall slab
ray vs AABB slab
swept AABB vs static AABB/slab
```

Later convex narrowphase:

```text
GJK for convex distance/intersection
EPA or contact clipping for penetration/contact normal
SAT for boxes/OBBs where simpler
```

Do not use GJK/EPA before the room engine needs arbitrary convex props.
They are real tools, but adding them early would hide the simpler room/floor
problems under advanced code.

## Broadphase Algorithm Tradeoff

First broadphase:

```text
uniform spatial hash grid
```

Why:

- dungeon rooms are grid/tile-ish;
- editor placement is local;
- static walls/floors can be bucketed;
- implementation is transparent;
- counters are easy.

Known failure mode:

- many bodies in one cell degrades to high candidate counts.

Required counters:

```text
cell_count
occupied_cell_count
max_bucket_size
candidate_pair_count
rejected_pair_count
```

Future broadphase options:

```text
sweep and prune: good when movement is mostly coherent on one axis
dynamic AABB tree: good for many moving objects and broad world scenes
BVH over static room chunks: good for large static authored geometry
```

Do not start with a tree unless profiling proves the uniform grid is the wrong
tool.

## Hot-Loop Cost Policy

Hot-loop physics code is code that runs per tick across bodies, contacts,
candidate pairs, or room collision surfaces.

Allowed hot-loop operations:

```text
add
subtract
multiply
dot product
length squared
min/max/clamp
absolute value
integer cell hashing
array indexing
bit flags
precomputed table lookup by id
```

Allowed with named helper and test:

```text
sqrt
division
normalization
matrix/vector transform
```

Not allowed in hot loops without explicit approval:

```text
pow
acos
asin
atan
sin
cos
exp
log
string comparison
allocation
virtual dispatch per body/contact
triangle-mesh collision for basic room floors/walls
O(n^2) all-body collision pass outside tiny tests
```

The rule is not "never use expensive math." The rule is "name it, isolate it,
test it, and prove the cheap path was not sufficient."

## Cheap Equation Replacements

### Distance checks

Do not compute real distance just to compare against a radius.

```cpp
// Avoid.
const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
const bool inside = distance <= radius;

// Use.
const float distanceSquared = dx * dx + dy * dy + dz * dz;
const bool inside = distanceSquared <= radius * radius;
```

Use actual distance only when a caller needs the scalar value for display,
normalized direction, time of impact, or report fields.

### Horizontal room checks

Most room/player checks should start in the horizontal X/Z plane.

```cpp
const float dx = point.x - origin.x;
const float dz = point.z - origin.z;
const float horizontalDistanceSquared = dx * dx + dz * dz;
```

This is the cheap first test for:

- leash radius;
- pickup radius;
- editor cursor proximity;
- room portal trigger checks;
- broadphase candidate rejection.

### Angle checks

Do not use inverse trig to answer "is this inside the cone?"

```cpp
// Avoid.
const float angle = std::acos(dot(forward, direction));
const bool visible = angle <= maxAngleRadians;

// Use.
const bool visible = dot(normalizedForward, normalizedDirection) >= cosMaxAngle;
```

The cosine threshold should be precomputed in the profile/config layer when
possible.

### Damping

Do not evaluate exponential damping per body per tick.

```cpp
// Avoid in hot body loops.
velocity *= std::exp(-drag * fixedDt);

// Use a precomputed fixed-step scalar.
velocity *= dampingPerTick;
```

Different materials or movement profiles can store different damping scalars.

### Gravity and integration

The first integrator should be semi-implicit Euler.

```cpp
velocity += gravity * fixedDt;
position += velocity * fixedDt;
```

This is cheap and stable enough for the first runtime engine layer. Higher-order
integrators are deferred until a measured problem appears.

### Normalization

Normalization must guard against tiny or nonfinite vectors.

```cpp
const float len2 = dot(v, v);
if (len2 <= epsilonSquared) {
  return fallback;
}
const float invLen = 1.0F / std::sqrt(len2);
return v * invLen;
```

This is an example of a good branch: it protects the runtime from invalid math.
The problem is not branches. The problem is branch ladders pretending to be data.

## Collision Math Stack

Physics should reject work in layers.

```text
1. body active/enabled flags
2. broadphase cell overlap
3. AABB overlap
4. shape pair narrowphase
5. contact manifold
6. solver
```

Do not start with narrowphase. Do not start with triangles.

### AABB overlap

```cpp
const bool overlap =
    a.min.x <= b.max.x && a.max.x >= b.min.x &&
    a.min.y <= b.max.y && a.max.y >= b.min.y &&
    a.min.z <= b.max.z && a.max.z >= b.min.z;
```

This belongs in a small named helper. The direct comparisons are cheap and
clear.

### Expanded AABB for swept movement

For fast-moving bodies, expand the start AABB by the motion vector before
broadphase.

```text
swept.min = min(start.min, start.min + delta)
swept.max = max(start.max, start.max + delta)
```

That gives broadphase a conservative candidate set without expensive continuous
collision for every object.

### Room floor/wall collision

The first room physics should not collide against render triangles.

Floors and walls should be converted into simple runtime collision primitives:

- floor plane or floor AABB span;
- wall slab/AABB;
- optional portal/trigger volume;
- optional one-way or special surface later through material flags.

The renderer may draw optimized merged floor/wall meshes. Physics should use
authoritative collision primitives derived from room data, not draw triangles.

## Broadphase Policy

The first broadphase should be a uniform spatial hash grid.

Reason:

- simple to implement;
- easy to debug;
- good enough for room-scale dungeons;
- works with editable maps;
- does not require tree rebalancing.

Expected data:

```cpp
struct PhysicsBroadphaseCell {
  std::uint32_t firstBodyIndex;
  std::uint32_t bodyCount;
};
```

The hot path should operate on integer cell keys and body indices, not strings
or object pointers.

Cell size should start as a config value, likely based on the largest common
dynamic body diameter or room tile size.

## Solver Policy

The first contact solver should use sequential impulses with a fixed iteration
count.

The cheap runtime loop is:

```text
for each fixed tick:
  apply forces
  integrate velocities
  build broadphase
  build contacts
  solve contacts for N iterations
  integrate positions
  publish debug snapshot
```

Initial values should be boring:

```text
fixed_dt = 1 / 60
velocity_iterations = 6
position_iterations = 2
gravity = (0, -9.8, 0)
```

These are not final tuning values. They are stable first proof values.

## Data Layout Policy

Editor/authored records can be array-of-structs because they are cold and need
to be readable.

```cpp
struct PhysicsBodyDesc {
  Vec3 position;
  Vec3 velocity;
  float massKg;
  PhysicsShapeId shapeId;
  MaterialId materialId;
};
```

Runtime physics body data should be struct-of-arrays.

```cpp
struct PhysicsBodyStore {
  std::vector<Vec3> positions;
  std::vector<Vec3> previousPositions;
  std::vector<Vec3> velocities;
  std::vector<float> inverseMasses;
  std::vector<PhysicsShapeId> shapeIds;
  std::vector<MaterialId> materialIds;
  std::vector<std::uint32_t> flags;
};
```

Reason:

- physics loops usually stream the same fields across many bodies;
- broadphase wants AABBs and body ids;
- solver wants velocities, inverse masses, contacts, and flags;
- less pointer chasing makes performance easier to reason about.

## Branch Policy

Good branches:

- invalid input guard;
- nonfinite math guard;
- missing body/shape/material guard;
- disabled/static body skip;
- backend/runtime failure return;
- fail-closed corrupt data path.

Bad branches:

- string-to-enum ladders;
- material pair behavior ladders;
- object kind behavior ladders;
- per-body virtual dispatch;
- render mode policy inside physics;
- gameplay special cases inside broadphase/solver loops.

Mapping belongs in tables.

Routing belongs in dispatchers.

Validation belongs in guard clauses or validators.

Hot loops belong in data buckets.

## Approval Gate For Expensive Math

Any new hot-loop use of these operations requires an explicit note in the build
order or code review:

```text
sqrt
pow
acos
asin
atan
sin
cos
exp
log
allocation
string lookup
triangle collision
all-pairs collision
```

The note must answer:

```text
What is the operation used for?
Why is the cheap equivalent insufficient?
What test proves it is bounded?
What debug/receipt field proves it is not accidentally used everywhere?
```

## First Build Boundary

The first physics source slice should not touch product UI, AppShell, renderer,
save/load, or gameplay behavior.

First source slice should add only:

```text
src/runtime/physics/PhysicsTypes.*
src/runtime/physics/PhysicsBodyStore.*
src/runtime/physics/PhysicsStep.*
tests/unit/physics_*_tests.cpp
```

Allowed behavior:

- create body descriptors;
- store hot body fields in SoA;
- validate finite positions/velocities/mass;
- run fixed-step gravity/integration;
- report deterministic body positions after N ticks.

Not allowed yet:

- collision;
- contacts;
- solver;
- room integration;
- product UI;
- AppShell;
- save/load;
- renderer/Vulkan;
- material interaction.

## Acceptance Gate

This foundation is accepted when:

- the doc exists in `docs/plan_bucket`;
- no source behavior changed;
- the next physics source slice can be written from these math rules without
  inventing its own hidden policy;
- expensive math usage has a named approval rule before code exists.
