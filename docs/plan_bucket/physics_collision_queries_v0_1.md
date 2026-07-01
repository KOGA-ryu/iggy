# Physics Collision Queries v0.1

This document defines the first collision-query layer for Iggy3D physics.
It depends on `physics_math_foundation_v0_1.md` and
`physics_step_pipeline_v0_1.md`.

The goal is to make collision math cheap, named, testable, and reusable before
the full solver exists. Queries should answer questions like "what did this
body hit?", "can this body move there?", "where is the ground?", and "what is
under the cursor?" without forcing callers to know renderer triangles or room
authoring internals.

## Ownership

Collision queries live in runtime physics/collision space.

They may read:

- runtime physics bodies;
- runtime physics shapes;
- static room collision primitives;
- material ids and collision flags;
- broadphase candidate data;
- query request structs.

They must not own:

- AppShell routing;
- product UI;
- renderer/Vulkan mesh data;
- ASCII source;
- save/load orchestration;
- editor cursor state;
- gameplay command admission.

Rendering can draw query/debug results. Rendering must not be the source of
collision truth.

## Query Types

The first query set should be:

```text
AABB overlap
closest point on AABB
ray cast
capsule overlap/sweep
swept AABB/body movement
ground check
```

These are enough to support:

- first-person player movement;
- room wall/floor blocking;
- editor placement preview;
- object pickup/use targeting;
- debug draw;
- future physics contact generation.

## Shared Result Shape

Queries should return facts with stable status/reason codes.

Expected shape:

```cpp
struct PhysicsQueryResult {
  bool hit = false;
  std::string status = "physics_query_no_hit";
  std::string reasonCode = "physics_query_no_hit";
  PhysicsBodyId bodyId;
  PhysicsShapeId shapeId;
  MaterialId materialId;
  Vec3 point;
  Vec3 normal;
  float distanceMeters = 0.0F;
  float timeOfImpact = 1.0F;
  std::uint32_t checkedCandidateCount = 0;
};
```

Source may split this into specialized result structs, but each result should
keep the same idea: hit state, stable reason, candidate count, point, normal,
distance, and time of impact where relevant.

## AABB Overlap

AABB queries are the cheapest broad collision primitive.

Use them for:

- body-body broad rejection;
- editor preview blocking;
- trigger volume checks;
- first-pass room collision;
- expanded swept movement candidate collection.

Overlap equation:

```cpp
const bool overlap =
    a.min.x <= b.max.x && a.max.x >= b.min.x &&
    a.min.y <= b.max.y && a.max.y >= b.min.y &&
    a.min.z <= b.max.z && a.max.z >= b.min.z;
```

Rules:

- invalid or nonfinite AABBs reject before query work;
- min must be less than or equal to max on every axis;
- touching counts as overlap for contact generation;
- caller policy decides whether touching counts as blocking for movement.

Do not allocate during AABB overlap tests.

Do not attach gameplay semantics to the overlap helper.

## Closest Point On AABB

Closest-point queries support cheap distance checks without sphere/mesh logic.

Equation:

```cpp
closest.x = clamp(point.x, box.min.x, box.max.x);
closest.y = clamp(point.y, box.min.y, box.max.y);
closest.z = clamp(point.z, box.min.z, box.max.z);
```

Squared distance:

```cpp
const Vec3 delta = point - closest;
const float distanceSquared = dot(delta, delta);
```

Use squared distance for comparisons. Only compute `sqrt` when returning a
human-facing distance or when the caller genuinely needs the scalar.

## Ray Cast

Ray casts answer "what is along this line?"

First uses:

- look/use targeting;
- editor picking;
- debug probes;
- projectile precheck;
- line-of-sight later.

Request shape:

```cpp
struct PhysicsRayCastRequest {
  Vec3 origin;
  Vec3 direction;
  float maxDistanceMeters = 0.0F;
  PhysicsCollisionMask mask;
};
```

Rules:

- origin must be finite;
- direction must be finite and nonzero;
- normalize direction once at request validation;
- max distance must be finite and positive;
- broadphase should use the ray AABB before per-shape tests;
- return the closest hit by stable tie-break order.

Ray vs AABB should use slab math.

Slab concept:

```text
For each axis:
  find t interval where ray overlaps the box slab.
Intersect all axis intervals.
Hit when final interval overlaps [0, maxDistance].
```

Implementation should avoid division by zero by handling near-zero direction
axes explicitly.

## Capsule Queries

Capsules are the preferred first shape for player-like bodies.

Reason:

- smoother over small edges than a box;
- cheaper than mesh collision;
- good for first-person movement;
- works with ground checks and wall slides.

Capsule data:

```cpp
struct PhysicsCapsule {
  Vec3 bottom;
  Vec3 top;
  float radiusMeters = 0.0F;
};
```

Validation:

- bottom/top finite;
- radius finite and positive;
- segment length nonzero unless the query explicitly allows sphere fallback.

Cheap tests:

- capsule vs point uses closest point on capsule segment;
- capsule vs AABB can start with expanded AABB by capsule radius;
- capsule vs floor span can use bottom sphere/foot point first;
- capsule vs wall slab can use horizontal segment or swept AABB before exact
  capsule math.

Expensive capsule work should be isolated in named helpers. Do not put capsule
special cases into AppShell or product movement branches.

## Swept Movement

Swept movement answers "can this body move from A to B this tick?"

First use:

- first-person player movement;
- dynamic body movement;
- editor placement ghost blocking;
- projectiles later.

Request shape:

```cpp
struct PhysicsSweepRequest {
  PhysicsShapeId shapeId;
  Vec3 start;
  Vec3 delta;
  PhysicsCollisionMask mask;
};
```

Rules:

- start and delta must be finite;
- zero delta returns no-hit or already-overlapping status without broad search;
- build swept AABB first;
- query broadphase/static room candidates with swept AABB;
- run narrowphase only on candidates;
- return earliest time of impact in `[0, 1]`;
- ties use stable body/surface id order.

Swept AABB:

```text
swept.min = min(startAabb.min, startAabb.min + delta)
swept.max = max(startAabb.max, startAabb.max + delta)
```

Movement result:

```text
safe_delta = delta * max(0, time_of_impact - skin)
remaining_delta = delta - safe_delta
```

Skin should be a small config value. It prevents resting bodies from vibrating
against surfaces.

## Ground Checks

Ground checks answer:

- is the body standing?
- what floor height is under it?
- what is the floor normal?
- can this slope be walked on?

Ground checks should not raycast against render triangles.

First ground sources:

- floor spans;
- floor AABBs;
- static room surfaces;
- later moving platforms.

Request shape:

```cpp
struct PhysicsGroundCheckRequest {
  Vec3 position;
  float radiusMeters = 0.0F;
  float snapDistanceMeters = 0.0F;
  float maxWalkableSlopeCos = 0.0F;
  PhysicsCollisionMask mask;
};
```

Rules:

- position finite;
- radius finite and nonnegative;
- snap distance finite and nonnegative;
- slope check uses dot(normal, up) >= maxWalkableSlopeCos;
- no `acos` needed for slope checks;
- return nearest valid walkable floor under the body.

Cheap slope test:

```cpp
const bool walkable = dot(surfaceNormal, worldUp) >= maxWalkableSlopeCos;
```

The profile/config layer can precompute `maxWalkableSlopeCos` from an authored
angle. The hot query should not call trig.

## Query Ordering

Queries must produce stable results.

Stable ordering rules:

- sort candidate body ids ascending before narrowphase if broadphase storage
  order is not already deterministic;
- choose smaller time of impact first;
- tie-break by body id or surface id;
- never rely on unordered container iteration order for final result selection.

This is not about perfect determinism across all hardware. It is about making
tests and editor behavior repeatable.

## Collision Masks

Queries should use compact masks, not strings.

Expected first masks:

```text
actor
projectile
trigger
editor_preview
visibility
```

String tags may exist in authored data, but runtime queries should consume
precomputed bit masks.

Examples:

- player movement uses actor mask;
- projectile ray/sweep uses projectile mask;
- editor ghost placement uses editor_preview mask;
- use/inspect targeting may combine visibility and trigger masks.

## Material Awareness

Collision queries may return material ids. They should not decide full material
behavior.

Allowed in query:

- return hit material id;
- apply collision mask filtering;
- include surface flags like walkable/blocking/trigger.

Not allowed in query:

- decide sound effect;
- decide gameplay damage;
- decide bounce/friction policy beyond returning data needed by solver;
- branch on material names.

Material pair behavior belongs in material/physics tables or the solver layer.

## Expensive Math Approval

Collision queries are the highest-risk area for hidden expensive math.

These operations require a named helper and direct test:

```text
sqrt
normalization
division-heavy slab math
capsule closest-point math
continuous collision time of impact
```

These operations require explicit approval before entering hot query paths:

```text
triangle mesh collision
per-query allocation
string lookup
acos/asin/atan
pow
all-surface scan when broadphase exists
```

## First Source Slice Fit

This document should not force all query types into the first code slice.

Recommended order:

```text
1. AABB validation and overlap helpers
2. closest point on AABB
3. ray vs AABB slab query
4. ground check against floor spans
5. swept AABB movement
6. capsule helpers
```

The first query implementation should avoid product UI and renderer coupling.

Expected first source paths:

```text
src/runtime/physics/PhysicsCollisionQuery.*
tests/unit/physics_collision_query_tests.cpp
```

No AppShell, save/load, Vulkan, or room editor behavior should be changed in the
first query slice.

## Acceptance Gate

This document is accepted when:

- AABB, capsule, ray, swept movement, and ground checks are defined;
- each query names cheap math and expensive-math approval boundaries;
- query ownership stays in runtime physics/collision space;
- renderer triangles are explicitly not collision truth;
- stable query ordering is documented;
- the first source slice can start with AABB helpers without committing to the
  entire physics engine.

