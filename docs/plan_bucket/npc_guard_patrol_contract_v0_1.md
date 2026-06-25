# NPC Guard and Patrol Contract v0.1

## Objective

Define the NPC Behavior v0.4 guard/home-anchor layer before source work
begins.

The first v0.4 implementation should give an NPC a durable home position that
comes from authored scenario/package metadata. That lets an NPC guard an area,
chase only inside approved leash policy, and return home when the target leaves
that policy or the NPC drifts too far away.

This is deliberately not full patrol routing yet. It is also not pathfinding,
line-of-sight, group tactics, editor UI, AppShell behavior, renderer behavior,
or ASCII behavior semantics.

First source slice selection:

- add durable AI guard/home anchor state first;
- keep full patrol routes deferred;
- keep direct coordinate anchor metadata deferred;
- use stable-name scenario marker anchors as the first authored source.

## Source Truth And Ownership

Runtime ownership:

- `AiActorState` owns active AI behavior state, target, intent, cooldown,
  current profile id, and future guard/home runtime fields;
- future guard/home fields must become runtime durable state before behavior
  decisions consume them;
- `NpcBehaviorSystem` owns guard-aware perception, chase, attack, return, and
  wait decisions;
- `Session::tick()` owns enqueueing the resulting AI commands through the
  normal command pipeline.

Scenario/package ownership:

- scenario/package metadata owns authored profile bindings and future
  guard/patrol assignment by stable entity names;
- guard assignment keys must be stable scenario entity names, not map glyphs;
- guard anchor references should resolve to authored marker entities for the
  v0.4 first implementation;
- authored metadata is loaded and validated before runtime session creation.

World and command ownership:

- `WorldState` owns entity transforms and stable names;
- movement owns physical movement execution;
- combat owns attack mutation;
- command admission and command log own accepted and rejected `Move`, `Wait`,
  and `Attack` records;
- guard return movement must use normal AI `Move` command admission and normal
  movement tick execution;
- no guard behavior may directly write entity transforms.

Product/debug ownership:

- product receipts and HUDs are diagnostic proof only;
- debug snapshots and projection expose guard state and decisions after runtime
  state exists;
- display text, receipt fields, and render overlays are not behavior truth.

ASCII boundary:

- ASCII may author map geometry or room fixtures used by tests;
- ASCII glyphs and layout symbols do not assign behavior, profile ids, guard
  anchors, patrol routes, or leash policy;
- scenario/package metadata owns AI behavior assignment.

## Durable Runtime Model

The first source slice should add a small guard/home state to `AiActorState`.
Suggested field shape:

```cpp
bool hasHomePosition = false;
Vec3 homePosition;
std::string homeStableName;
float leashRadiusMeters = 0.0F;
float returnRadiusMeters = 0.0F;
float homeToleranceMeters = 0.0F;
```

Field semantics:

- `hasHomePosition=false` means no guard behavior is configured and the actor
  keeps existing profile-driven v0.3 behavior;
- `homePosition` is the resolved world-space home/guard anchor position;
- `homeStableName` is the authored marker stable name used to resolve the home;
- `leashRadiusMeters` defines the home-centered engagement boundary;
- `returnRadiusMeters` is the distance at which the actor should stop returning
  and resume guarding/waiting;
- `homeToleranceMeters` allows small floating-point movement drift around home;
- all enabled guard distances must be finite and positive, and
  `returnRadiusMeters` should be less than or equal to `leashRadiusMeters`.

Deferred runtime fields:

- `lastKnownTargetPosition` is deferred until line-of-sight, sound/noise, or
  search behavior needs it;
- patrol route index and waypoint progress are deferred until patrol routes are
  explicitly implemented;
- authored direct-position anchors are deferred until the stable-name marker
  flow is proven.

Behavior and intent vocabulary additions for the first source slice:

- add `AiBehaviorKind::Returning`;
- add `AiIntentKind::ReturnToAnchor`.

Deferred vocabulary:

- `Guarding` may be added later if `Idle`/`Alert` stops carrying enough debug
  meaning;
- `Patrolling` and `FollowPatrolRoute` belong to the later patrol route slice.

Durability requirements:

- every new guard/home field must save, load, and hash in the same slice where
  it is added;
- old saves missing guard/home fields must decode as no guard configured;
- reset and baseline restore must preserve authored guard/home state if it was
  seeded into the baseline;
- invalid saved enabled guard data should be rejected during decode/load rather
  than silently coerced.

## Scenario Metadata Shape

The v0.4 first authoring source is a stable-name marker anchor:

```toml
[[ai_guard_anchors]]
actor = "training_dummy"
anchor = "guard_post_alpha"
leash_radius_meters = 6.0
return_radius_meters = 1.0
home_tolerance_meters = 0.25
```

Required keys:

- `actor`: stable scenario entity name of the NPC actor;
- `anchor`: stable scenario entity name of a marker entity used as home;
- `leash_radius_meters`: finite positive home-centered engagement radius;
- `return_radius_meters`: finite positive stop distance for return behavior;
- `home_tolerance_meters`: finite non-negative tolerance around the home point.

Validation policy:

- missing or empty `actor` rejects;
- `actor` must resolve to an existing scenario entity;
- `actor` must resolve to an NPC entity;
- duplicate guard assignment for the same actor rejects;
- missing or empty `anchor` rejects;
- `anchor` must resolve to an existing scenario entity;
- `anchor` must resolve to `EntityKind::Marker` for the first source slice;
- invalid, non-finite, zero, or negative leash/return distances reject;
- negative or non-finite home tolerance rejects;
- `return_radius_meters` greater than `leash_radius_meters` rejects;
- unknown profile ids still follow the existing v0.2 profile policy and are
  separate from guard anchor validation.

Stable diagnostic code vocabulary should be lower snake. Suggested codes:

```text
scenario.ai_guard_anchor_ok
scenario.missing_guard_actor
scenario.guard_actor_not_found
scenario.non_npc_guard_actor
scenario.duplicate_guard_actor
scenario.missing_guard_anchor
scenario.guard_anchor_not_found
scenario.non_marker_guard_anchor
scenario.invalid_guard_leash_radius
scenario.invalid_guard_return_radius
scenario.invalid_guard_home_tolerance
scenario.guard_return_exceeds_leash
```

Deferred direct-position syntax:

```toml
[[ai_guard_anchors]]
actor = "training_dummy"
position_m = [2.0, 0.0, 4.0]
leash_radius_meters = 6.0
return_radius_meters = 1.0
home_tolerance_meters = 0.25
```

Direct-position anchors are not the first v0.4 source slice. If later allowed,
`anchor` and `position_m` must be mutually exclusive. Stable-name `anchor` should
have priority in migration guidance because it keeps behavior authoring tied to
named content instead of anonymous coordinates.

## Runtime Semantics

Guard-aware behavior keeps using the existing profile-resolved
`NpcBehaviorConfig` and the existing `Move`, `Wait`, and `Attack` command kinds.

Baseline rules:

- no configured guard state keeps current v0.3 profile behavior;
- configured guard state never bypasses command admission;
- no direct transform writes are allowed;
- attack remains a normal AI `Attack` command;
- chase and return remain normal AI `Move` commands;
- `Wait` remains the normal non-movement/non-attack command.

Home and leash rules:

- home-centered leash policy decides whether the NPC may pursue or attack;
- if the actor is farther than `leashRadiusMeters + homeToleranceMeters` from
  home, choose return behavior before chase or attack;
- if the perceived target is outside `leashRadiusMeters` from home, choose
  return behavior when the actor is not already home, otherwise wait/guard;
- if a chase destination would exceed the leash boundary, choose return instead
  of chasing past the allowed area;
- when returning, move toward `homePosition` until distance to home is less than
  or equal to `returnRadiusMeters` or `homeToleranceMeters`, whichever is
  larger;
- once home is reached, choose `Idle` or `Alert` with `Wait` until a valid
  target is again allowed by profile and leash policy.

Target and combat rules:

- hostile/default profile can chase and attack only when profile policy and
  guard leash both allow engagement;
- passive profile still waits/alerts and does not chase or attack;
- unknown or invalid profile ids keep the v0.2 fail-closed behavior and emit no
  AI command;
- defeated actor and defeated target policies remain unchanged.

Blocked movement policy:

- there is no pathfinding in v0.4 first guard behavior;
- return/chase movement is straight-line move intent through existing movement
  admission and execution;
- blocked movement should appear as a normal rejected command log entry;
- future pathfinding or navigation surfaces can improve blocked return/chase
  without changing the guard metadata contract.

Runtime invalid guard data policy:

- missing guard data means no guard semantics and current profile behavior;
- enabled but invalid guard data fails closed for that actor during AI decision:
  no chase, no attack, no return command, and debug should expose the invalid
  guard state once diagnostics are extended;
- runtime should not silently fall back from invalid guard data to hostile
  default behavior.

## Save, Replay, And Determinism

Save/load/hash requirements:

- all durable guard/home fields must be present in `SaveEnvelope`, `SaveCodec`,
  `SaveLoad`, and `StateHash` in the same slice where they are added;
- old saves without guard/home keys load as `hasHomePosition=false`;
- reset/baseline restore preserves authored guard state when the scenario seed
  populated it;
- guard fields are durable runtime state, not a transient package lookup.

Replay requirements:

- deterministic actor iteration remains by actor entity id;
- guard metadata application must resolve stable names deterministically;
- duplicate or invalid authored metadata rejects before session creation;
- no new command-log player-slot policy is needed while guard uses existing AI
  `Move`, `Wait`, and `Attack` commands;
- stable tie-breaks are required before patrol routes add multiple candidate
  waypoints.

## Debug And Receipt Requirements

Later v0.4 diagnostic slices should extend the existing NPC debug ladder:

- `NpcBehaviorDebugSnapshot` should expose whether guard/home is configured;
- snapshot rows should include `homeStableName`, home position, leash radius,
  return radius, home distance, target-home distance, and whether the actor is
  returning;
- projection should keep NPC guard HUD lines compact and ASCII-only;
- product receipt fields should expose summary booleans/counts, not full
  free-form guard HUD lines;
- product HUD and receipt proof remain diagnostics and do not become runtime
  truth.

Suggested receipt proof fields for a later product slice:

```text
npc_behavior_guard_configured_count
npc_behavior_guard_returning_count
npc_behavior_guard_leash_blocked_count
npc_behavior_guard_debug_available
```

Exact field names can change in the source slice, but the ownership boundary
must not: runtime state and command logs are truth; product fields are proof.

## Build Slices

Slice 1: durable AI guard anchor state model.

- add `AiActorState` guard/home fields;
- add `Returning` / `ReturnToAnchor` vocabulary if source-fit confirms the enum
  names;
- update save envelope, codec, load, state hash, reset/baseline tests;
- no behavior changes yet.

Slice 2: scenario/package guard anchor metadata parse and validation.

- parse `[[ai_guard_anchors]]` with stable-name marker anchors;
- validate actor, marker anchor, duplicates, and distances;
- keep direct coordinates and patrol routes deferred;
- no session behavior changes yet.

Slice 3: product/session seed mapping.

- resolve scenario guard metadata into `FixtureScenarioSeed` or the existing
  session seed seam;
- map actor/anchor stable names to baseline `AiActorState` guard fields;
- preserve reset/baseline behavior;
- prove ASCII glyph names have no special behavior meaning.

Slice 4: pure `NpcBehaviorSystem` guard/return decisions.

- add guard-aware decision inputs and outputs;
- prove return-to-home, leash-blocked chase, allowed chase, allowed attack, and
  passive wait policies;
- use existing command builders only.

Slice 5: session tick integration.

- make `Session::tick()` use guard-aware runtime state/config;
- enqueue only existing AI `Move`, `Wait`, and `Attack` commands;
- prove command-log admission, rejected blocked movement, cooldown, and reset
  behavior.

Slice 6: debug snapshot/projection/product receipt proof.

- extend NPC debug snapshot rows with guard/leash/return facts;
- extend projection and product HUD summaries;
- add no-window product proof for return/leash behavior.

Slice 7: documentation closeout.

- update this contract and the main NPC behavior contract with implemented
  truth and focused proof targets;
- remove completed guard/return items from future work.

## Non-Goals

- no pathfinding or navmesh;
- no line-of-sight;
- no group tactics;
- no patrol splines or patrol routes in the first source slice;
- no editor UI;
- no renderer or Vulkan work;
- no AppShell-owned behavior;
- no product receipt truth beyond diagnostic proof;
- no ASCII behavior semantics.

## Acceptance Gate

The v0.4 guard/home anchor baseline is accepted only when:

- authored scenario/package metadata can assign a marker anchor to an NPC by
  stable entity name;
- invalid metadata is rejected before session creation;
- seeded baseline AI state preserves home/leash fields across reset;
- save/load/hash cover all new durable fields;
- pure behavior tests prove allowed chase/attack, leash-blocked chase, return,
  passive wait, and invalid guard fail-closed policy;
- session tests prove return and chase use normal AI command admission/log/tick;
- debug/product proof exposes guard/leash/return summaries without making HUD or
  receipts behavior truth;
- ASCII remains map-making only.

## Implementation Stop Rules

Stop and return to planning if a source slice needs:

- direct coordinate anchors before stable-name marker anchors are proven;
- pathfinding or navmesh work;
- line-of-sight or sound/noise perception;
- patrol routes before home/guard return behavior lands;
- new command kinds instead of existing `Move`, `Wait`, and `Attack`;
- AppShell-owned AI decisions;
- renderer/Vulkan changes;
- ASCII glyph behavior semantics;
- broad save schema churn outside the durable guard fields.
