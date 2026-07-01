# Physics Material Traits v0.1

This document defines the first material and interaction trait layer for Iggy3D
physics. It depends on `physics_math_foundation_v0_1.md`,
`physics_step_pipeline_v0_1.md`, and `physics_collision_queries_v0_1.md`.

The goal is to keep material behavior data-driven. Floors, walls, props, and
dynamic bodies may carry material ids, but physics behavior should come from
material/profile tables, not from if-chains that branch on material names.

## Ownership

Material traits live between authored content and runtime physics.

They may define:

- friction;
- restitution/bounce;
- damping;
- weight class;
- collision sound hooks;
- impact thresholds;
- interaction flags;
- solver combine rules.

They must not own:

- AppShell routing;
- renderer/Vulkan draw behavior;
- ASCII glyph semantics;
- save/load orchestration;
- controller mapping;
- gameplay command admission;
- UI panel behavior.

ASCII may produce room layout and default material ids such as debug floor or
debug wall. ASCII must not define friction law, sound routing, bounce behavior,
or object interaction logic.

## Material Ids

Material ids are authored/config identifiers at boundaries. Runtime hot loops
should consume compact material indices or ids, not strings.

Boundary shape:

```cpp
struct PhysicsMaterialDesc {
  std::string id;
  float staticFriction = 0.6F;
  float dynamicFriction = 0.5F;
  float restitution = 0.0F;
  float linearDampingPerTick = 1.0F;
  PhysicsWeightClass weightClass = PhysicsWeightClass::Medium;
  PhysicsMaterialFlags flags;
  PhysicsSoundMaterialId soundMaterialId;
};
```

Hot-loop shape:

```cpp
struct PhysicsMaterialTable {
  std::vector<float> staticFriction;
  std::vector<float> dynamicFriction;
  std::vector<float> restitution;
  std::vector<float> linearDampingPerTick;
  std::vector<PhysicsWeightClass> weightClass;
  std::vector<std::uint32_t> flags;
  std::vector<PhysicsSoundMaterialId> soundMaterialId;
};
```

Strings belong at load/authoring/debug edges. Step, query, and solver code
should use integer material ids or indices.

## Built-In First Materials

The first built-ins should be boring and readable:

```text
debug_floor
debug_wall
stone
wood
metal
ice
rubber
cloth
trigger
```

These names are content-facing. Runtime physics should resolve them before the
fixed step.

Suggested first values:

| Material | Static friction | Dynamic friction | Restitution | Weight class |
|---|---:|---:|---:|---|
| debug_floor | 0.7 | 0.6 | 0.0 | static |
| debug_wall | 0.8 | 0.7 | 0.0 | static |
| stone | 0.8 | 0.7 | 0.05 | heavy |
| wood | 0.6 | 0.5 | 0.15 | medium |
| metal | 0.5 | 0.4 | 0.10 | heavy |
| ice | 0.05 | 0.03 | 0.0 | medium |
| rubber | 0.9 | 0.8 | 0.75 | light |
| cloth | 0.7 | 0.6 | 0.05 | light |
| trigger | 0.0 | 0.0 | 0.0 | static |

These are initial tuning values, not realism claims.

## Friction

Friction must be a material-pair rule, not a material-name branch.

First combine policy:

```cpp
combinedStaticFriction = sqrt(a.staticFriction * b.staticFriction);
combinedDynamicFriction = sqrt(a.dynamicFriction * b.dynamicFriction);
```

Because `sqrt` is controlled math, this should be precomputed into a material
pair table whenever possible:

```cpp
struct PhysicsMaterialPairTable {
  std::vector<float> staticFriction;
  std::vector<float> dynamicFriction;
  std::vector<float> restitution;
  std::vector<PhysicsInteractionFlags> interactionFlags;
};
```

Hot solver code should read:

```text
pair = material_pair_table[material_a][material_b]
```

It should not compute string rules like:

```text
if material == "ice" ...
```

## Bounce / Restitution

Restitution controls bounce.

First combine policy:

```cpp
combinedRestitution = max(a.restitution, b.restitution);
```

Reason:

- cheap;
- predictable;
- common enough for first-pass game physics;
- lets bouncy materials work when hitting mostly non-bouncy surfaces.

Restitution should be clamped:

```text
0.0 <= restitution <= 1.0
```

Bounce should also have an impact-speed threshold so tiny resting contacts do
not jitter.

```text
minimum_bounce_speed = 0.5 m/s
```

The threshold belongs in material/solver config, not in object-specific code.

## Weight Class

Weight class is not a replacement for mass. It is a gameplay-facing grouping
for interaction rules.

First enum:

```text
Static
Light
Medium
Heavy
Massive
```

Use cases:

- whether the player can push an object;
- whether a carried object slows movement;
- what sound intensity bucket an impact uses;
- whether a pressure plate is activated;
- whether a tool can move or break an object.

Runtime physics should still use mass and inverse mass for solving.

Weight class should be used for game interaction and tool policy, not contact
math that already has mass.

## Sound Material Hooks

Physics should not play sounds directly.

Physics may emit impact facts:

```cpp
struct PhysicsImpactEvent {
  PhysicsBodyId bodyA;
  PhysicsBodyId bodyB;
  PhysicsSoundMaterialId soundA;
  PhysicsSoundMaterialId soundB;
  Vec3 point;
  Vec3 normal;
  float normalSpeedMetersPerSecond = 0.0F;
  float impulseMagnitude = 0.0F;
};
```

Audio/product layers can map these facts to sound effects later.

First sound material ids:

```text
stone
wood
metal
cloth
rubber
ice
generic
silent
```

No audio branching should live in the solver.

## Interaction Traits

Interaction traits describe what gameplay systems may do with an object or
surface. They should be flags or table rows.

First flags:

```text
walkable
blocks_actor
blocks_projectile
trigger
pushable
carryable
breakable
flammable
conductive
magnetic
slippery
bouncy
no_sound
```

These traits do not all need implementation now. The important part is that the
shape is table-driven and owned outside AppShell.

Examples:

- `slippery` may select low-friction material values;
- `bouncy` may select high restitution;
- `no_sound` suppresses impact event emission;
- `trigger` participates in queries but not solid solving;
- `walkable` allows ground checks to treat a surface as valid floor.

## Material Pair Overrides

Some interactions need pair-specific overrides.

Examples:

```text
rubber + metal -> louder impact, high bounce
ice + stone -> very low friction
cloth + metal -> low sound, medium friction
trigger + any -> no solid contact
```

These overrides belong in a material-pair table:

```cpp
struct PhysicsMaterialPairDesc {
  std::string materialA;
  std::string materialB;
  float staticFriction;
  float dynamicFriction;
  float restitution;
  PhysicsInteractionFlags interactionFlags;
  PhysicsImpactSoundRule soundRule;
};
```

Compile the authoring pair table into dense runtime arrays before simulation.

Do not put pair-specific behavior in narrowphase or solver if-chains.

## Solver Use

The solver may read material-pair values:

- combined static friction;
- combined dynamic friction;
- restitution;
- bounce threshold;
- solid/trigger policy;
- sound event threshold.

The solver should not:

- compare material strings;
- choose sound effect names;
- decide gameplay damage;
- mutate inventory/objective state;
- call product UI or renderer code.

## Query Use

Collision queries may return:

- hit material id;
- hit interaction flags;
- sound material id if needed for debug;
- walkable/blocking/trigger status.

Queries should not decide:

- friction response;
- bounce impulse;
- sound playback;
- gameplay reaction.

That keeps queries cheap and reusable.

## Editor And Authoring Use

Editor tools should show material selection as authored data.

Allowed editor behavior:

- choose floor/wall material id;
- preview friction/bounce/weight summary;
- warn when unknown material id would resolve to default;
- show material-pair result in a physics lab.

Not allowed:

- material behavior hardcoded in editor placement logic;
- ASCII glyphs assigning hidden physics traits;
- AppShell owning material lookup.

## Missing And Invalid Materials

Missing material policy should fail soft at authoring edges and fail closed in
runtime hot paths.

Recommended behavior:

- unknown authored material id reports validation/debug warning;
- product/editor can substitute `debug_floor` or `debug_wall` only at explicit
  content import boundaries;
- runtime physics body with unresolved material id uses `invalid_material`
  status and is skipped or fails the step before unsafe solving.

No silent fallback to a bouncy or slippery material.

## First Source Slice Fit

The first material source slice should be model-only:

```text
src/runtime/physics/PhysicsMaterialTraits.*
tests/unit/physics_material_traits_tests.cpp
```

Allowed first behavior:

- define material ids/descriptors;
- validate friction/restitution/damping ranges;
- build a built-in material table;
- resolve string ids to runtime material indices;
- build pair tables;
- prove pair combine rules.

Not allowed in the first material source slice:

- solver integration;
- collision query integration;
- AppShell/product UI;
- renderer/Vulkan;
- save/load schema;
- audio playback;
- ASCII parser changes.

## Acceptance Gate

This document is accepted when:

- friction, bounce, weight class, sound hooks, and interaction traits are
  defined as data;
- material-pair behavior is table-driven;
- string material ids are kept out of hot physics loops;
- queries and solver have clear ownership boundaries;
- first source implementation can start with model/tests only.

