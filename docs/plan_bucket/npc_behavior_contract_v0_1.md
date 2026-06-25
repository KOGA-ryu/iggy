# NPC Behavior Contract v0.1

## Objective

Define the durable first NPC behavior system for Iggy3D.

The goal is not to make a clever AI layer first. The goal is to add a
deterministic runtime-owned behavior loop that can be tested from ASCII rooms
without launching a window.

Target v0.1 behavior:

```text
NPC exists
-> NPC is enabled for behavior
-> NPC perceives the player by distance
-> NPC chooses wait, chase, or attack
-> NPC command enters the normal session command pipeline
-> session tick applies movement/combat
-> receipts prove what happened
```

This contract exists to prevent a temporary NPC hack from becoming permanent
engine structure. NPC behavior must be a runtime system, not an AppShell branch,
renderer trick, or product automation special case.

## Current Baseline

Existing runtime AI state:

```text
src/runtime/ai/AiState.hpp
```

Current types:

```cpp
struct AiActorState {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
};

struct AiState {
  std::vector<AiActorState> actors;
};
```

This is enough to prove that AI already belongs to runtime session state. It is
not enough to express actual NPC behavior yet.

Existing session ownership:

```text
src/runtime/session/SessionState.hpp
```

`SessionState` already owns:

```text
WorldState
PlayerRoster
ClockState
CombatState
AiState
ObjectiveState
CommandLog
```

Existing save/load support:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.hpp
src/runtime/save/SaveLoad.cpp
```

`SaveLoad.cpp` already writes and reads `AiActorState` records. Any added
durable AI fields must be added to the save envelope, codec, load validation,
and load replacement path in the same slice.

Existing state hash support:

```text
src/runtime/replay/StateHash.cpp
```

AI state already participates in state hashing. Any new durable AI field must
participate in state hash immediately.

Existing command and tick pipeline:

```text
src/runtime/command/Command.hpp
src/runtime/command/CommandAdmission.hpp
src/runtime/command/CommandAdmission.cpp
src/runtime/session/Session.hpp
src/runtime/session/Session.cpp
src/runtime/session/SessionTick.hpp
src/runtime/session/SessionTick.cpp
```

Useful existing command facts:

- `CommandKind::Move` exists;
- `CommandKind::Attack` exists;
- `CommandKind::Wait` exists;
- `CommandSource::Ai` exists;
- `Session::submitCommand` admits commands and appends them to the command log;
- accepted queued commands are executed by `Session::tick`;
- `SessionTick.cpp` already applies move, interact, attack, ability, inspect,
  and wait commands.

Existing target and combat systems:

```text
src/runtime/targeting/TargetQuery.hpp
src/runtime/targeting/TargetQuery.cpp
src/runtime/targeting/ReachQuery.hpp
src/runtime/targeting/ReachQuery.cpp
src/runtime/combat/CombatState.hpp
src/runtime/combat/CombatSystem.hpp
src/runtime/combat/CombatSystem.cpp
```

Useful existing facts:

- target queries can filter by command kind;
- attack targetability uses `TargetAction::Attack`;
- combat preview/apply already validates defeated state, damage, factions, and
  friendly fire;
- attack mutation belongs to `CombatState`;
- target HP and defeated flags must not be mutated directly by AI.

Existing movement system:

```text
src/runtime/movement/MovementCommand.hpp
src/runtime/movement/MovementPolicy.hpp
src/runtime/movement/MovementSystem.hpp
src/runtime/movement/MovementSystem.cpp
```

Useful existing facts:

- movement commands are already validated by distance and collision surfaces;
- movement execution belongs to the movement system;
- NPC chase must use `Move` command semantics or a later approved movement
  request seam, not raw position writes.

Existing ASCII/package NPC creation:

```text
src/app/iggy3d/AsciiRoomGrid.hpp
src/app/iggy3d/AsciiRoomGrid.cpp
src/app/iggy3d/ProductPackageSessionSeed.hpp
src/app/iggy3d/ProductPackageSessionSeed.cpp
```

Useful existing facts:

- ASCII `N` creates an NPC spawn;
- ASCII `M` creates a monster spawn;
- NPCs become `EntityKind::Npc`;
- NPCs are targetable for `TargetAction::Attack` and `TargetAction::Inspect`;
- NPC combatants currently use faction `2`, HP `3`, and max HP `3`;
- player combatants currently use faction `1`, HP `10`, and max HP `10`.

Existing no-window proof:

```text
src/app/iggy3d/ProductGameplayTape.hpp
src/app/iggy3d/ProductGameplayTape.cpp
src/app/iggy3d/ProductGameplayTapeRunner.hpp
src/app/iggy3d/ProductGameplayTapeRunner.cpp
tests/unit/product_gameplay_tape_tests.cpp
tests/unit/product_gameplay_tape_runner_tests.cpp
tests/smoke/product_gameplay_tape_smoke.cpp
```

Current tape proof can:

- parse `attack <stable_name>`;
- run an attack against an ASCII NPC;
- prove NPC targetable and defeated through product receipts;
- run with `window_launch_count=0`.

## Non-Goals

V0.1 must not implement:

- behavior trees;
- utility AI;
- pathfinding;
- navmesh;
- line of sight through geometry;
- patrol routes;
- ranged spells;
- group tactics;
- stealth/noise;
- editor-side AI authoring;
- renderer/debug draw;
- multiplayer replication;
- JSON behavior files.

Those are later layers. V0.1 only establishes the runtime behavior spine.

## Data Ownership

Runtime truth:

```text
SessionState::ai
SessionState::world
SessionState::combat
SessionState::players
SessionState::clock
SessionState::commandLog
```

Owned by runtime AI:

- behavior-enabled actor records;
- current behavior state;
- target entity;
- next decision tick;
- cooldown ticks;
- deterministic policy id;
- last chosen intent proof if it is durable or hash-significant.

Owned by world:

- entity id;
- stable name;
- entity kind;
- transform;
- active flag;
- targetability.

Owned by combat:

- hit points;
- max hit points;
- faction;
- defeated flag.

Owned by session command pipeline:

- command ids;
- command sequence;
- admission;
- rejection;
- command log;
- pending execution.

Owned by product/app:

- receipts;
- no-window smoke setup;
- gameplay tape proof;
- display or debug summaries.

Product/app must not own AI state transitions.

Renderer ownership:

- none for v0.1.

Renderer may eventually draw NPC state, perception radii, paths, or debug labels,
but renderer output is never behavior truth.

## Durable AI State Model

Extend `AiActorState` rather than creating a disconnected NPC store.

Proposed v0.1 durable shape:

```cpp
enum class AiBehaviorKind : std::uint8_t {
  None,
  Idle,
  Alert,
  Chasing,
  Attacking,
  Defeated,
};

enum class AiIntentKind : std::uint8_t {
  None,
  Wait,
  MoveTowardTarget,
  AttackTarget,
};

struct AiActorState {
  EntityId actor;
  EntityId target;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind lastIntent = AiIntentKind::None;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  std::uint32_t cooldownTicksRemaining = 0;
  bool enabled = true;
};
```

Field semantics:

- `actor`: entity controlled by this AI record.
- `target`: current target entity, usually the player actor.
- `behavior`: durable behavior mode after the latest decision.
- `lastIntent`: latest selected intent for proof and replay debugging.
- `nextDecisionTick`: earliest session tick where this actor may decide again.
- `deterministicPolicy`: stable policy id, not a random seed.
- `cooldownTicksRemaining`: durable attack/action cooldown.
- `enabled`: false means the AI record is dormant and emits no commands.

Rules:

- `actor` must be a valid active `EntityKind::Npc` for v0.1 behavior.
- `target` may be invalid when there is no target.
- defeated combatants must enter `Defeated` and emit no commands;
- inactive entities emit no commands;
- disabled AI records emit no commands;
- AI records must be sorted or iterated deterministically by actor id.

## Behavior Semantics

`None`

- invalid or uninitialized state;
- should not be produced by a valid behavior tick except as a default before
  any decision.

`Idle`

- no valid player target is perceived;
- no command is emitted;
- `lastIntent=Wait` is acceptable if a decision was made.

`Alert`

- player is perceived but action is deferred because of decision cadence,
  cooldown, or policy;
- no attack or movement command is emitted in this tick.

`Chasing`

- player is perceived;
- player is outside attack range;
- NPC chooses a movement destination toward the player;
- command source must be `CommandSource::Ai`;
- movement must go through normal movement admission and tick execution.

`Attacking`

- player is perceived;
- player is inside attack range;
- cooldown allows attack;
- NPC emits an `Attack` command against the player actor;
- damage is set by AI config/policy, not hardcoded in the session tick.

`Defeated`

- NPC combatant is defeated;
- no command is emitted;
- behavior remains defeated until the combat state changes through an approved
  runtime path.

## Configuration Policy

V0.1 should keep config local and explicit. Do not hide values in AppShell.

Proposed runtime defaults:

```cpp
struct NpcBehaviorConfig {
  float perceptionRadiusMeters = 6.0F;
  float chaseStopDistanceMeters = 1.25F;
  float attackRangeMeters = 1.5F;
  float chaseStepMeters = 1.0F;
  std::int32_t attackDamage = 1;
  std::uint32_t decisionIntervalTicks = 1;
  std::uint32_t attackCooldownTicks = 2;
};
```

Rules:

- config must validate finite positive distances;
- attack damage must be positive;
- cooldowns are tick counts, not wall-clock seconds;
- default config must be deterministic and testable;
- scenario/package-authored AI config is deferred.

## Runtime Function Layout

New files:

```text
src/runtime/ai/NpcBehaviorSystem.hpp
src/runtime/ai/NpcBehaviorSystem.cpp
tests/unit/npc_behavior_system_tests.cpp
```

Existing file to extend:

```text
src/runtime/ai/AiState.hpp
```

Later save/hash files:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.cpp
src/runtime/replay/StateHash.cpp
tests/unit/save_load_tests.cpp
```

Only edit those later files in the same slice that adds durable AI fields.

Preferred public functions:

```cpp
NpcPerceptionResult queryNpcPerception(const NpcPerceptionRequest& request);

NpcBehaviorDecision chooseNpcBehaviorIntent(
    const NpcBehaviorDecisionRequest& request);

NpcBehaviorCommandResult buildNpcBehaviorCommand(
    const NpcBehaviorCommandRequest& request);

NpcBehaviorTickResult tickNpcBehaviorSystem(
    NpcBehaviorTickRequest& request);
```

Why this shape:

- perception is testable without command admission;
- decision is testable without mutating the session;
- command building is testable before integration;
- tick orchestration can be thin and deterministic;
- future line-of-sight/pathfinding can replace perception/chase math without
  rewriting command execution.

## Perception Semantics

V0.1 perception is distance-only.

Inputs:

- `WorldState`;
- `CombatState`;
- NPC actor id;
- player actor id;
- `NpcBehaviorConfig`.

Output:

```cpp
struct NpcPerceptionResult {
  NpcPerceptionStatus status;
  EntityId actor;
  EntityId target;
  Vec3 actorPosition;
  Vec3 targetPosition;
  float distanceMeters;
  bool actorActive;
  bool targetActive;
  bool actorDefeated;
  bool targetDefeated;
  bool targetInPerceptionRadius;
  bool targetInAttackRange;
};
```

Status vocabulary:

```text
npc_perception_ready
npc_perception_invalid_world
npc_perception_invalid_combat
npc_perception_invalid_actor
npc_perception_invalid_target
npc_perception_actor_inactive
npc_perception_target_inactive
npc_perception_actor_defeated
npc_perception_target_defeated
npc_perception_target_out_of_range
```

Rules:

- perception never mutates;
- perception never submits commands;
- perception never changes AI state;
- line-of-sight is not part of v0.1;
- if the player is defeated, NPC should not attack.

## Decision Semantics

Decision input:

- AI actor state;
- perception result;
- config;
- current tick.

Decision output:

```cpp
struct NpcBehaviorDecision {
  NpcBehaviorDecisionStatus status;
  AiBehaviorKind behavior;
  AiIntentKind intent;
  EntityId target;
  std::uint64_t nextDecisionTick;
  std::uint32_t cooldownTicksRemaining;
};
```

Status vocabulary:

```text
npc_behavior_decided
npc_behavior_disabled
npc_behavior_waiting_for_decision_tick
npc_behavior_actor_defeated
npc_behavior_no_target
npc_behavior_on_cooldown
npc_behavior_invalid_config
```

Decision rules:

1. disabled actor -> no command, preserve disabled state;
2. defeated actor -> `Defeated`, no command;
3. no perceived target -> `Idle`, no command;
4. target perceived but outside attack range -> `Chasing`, move intent;
5. target inside attack range and cooldown is zero -> `Attacking`, attack
   intent;
6. target inside attack range but cooldown active -> `Alert`, wait intent.

No random choice in v0.1.

## Command Building Semantics

AI commands must be normal `CommandRecord` values:

```cpp
CommandRecord command;
command.actor = npcActor;
command.kind = CommandKind::Attack | CommandKind::Move | CommandKind::Wait;
command.source = CommandSource::Ai;
command.payload...
```

Rules:

- AI must not directly mutate `WorldState` position;
- AI must not directly mutate `CombatState`;
- AI must not bypass command admission;
- AI must not write to `CommandLog` directly;
- AI must not assume command id or sequence values;
- command ids remain owned by `Session::submitCommand`;
- rejected AI commands must remain visible in the command log.

Attack command:

```cpp
kind = CommandKind::Attack
payload.target.hasEntity = true
payload.target.entity = playerActor
payload.attackDamage = config.attackDamage
```

Move command:

```cpp
kind = CommandKind::Move
payload.target.hasPoint = true
payload.target.point = computed chase destination
```

Wait command:

```cpp
kind = CommandKind::Wait
```

## Chase Math

V0.1 chase should use simple vector math.

Given:

```text
from = npc position
to = player position
step = config.chaseStepMeters
stop = config.chaseStopDistanceMeters
```

Compute:

```text
delta = to - from
horizontal = Vec3{delta.x, 0, delta.z}
distance = length(horizontal)
moveDistance = min(step, max(0, distance - stop))
destination = from + normalize(horizontal) * moveDistance
```

Rules:

- if distance is zero, no movement;
- if already within stop distance, do not chase;
- destination must be finite;
- destination enters normal movement command validation;
- slopes/elevation/pathfinding are deferred;
- collision response comes from existing movement/collision systems.

## Session Integration

Preferred v0.1 integration:

```text
Session::tick
-> before pending command execution, ask AI for commands
-> submit each AI command through Session::submitCommand or an equivalent
   internal admission path that appends to CommandLog
-> runSessionTick executes the accepted pending commands
```

Important: direct recursive use of `Session::submitCommand` inside
`Session::tick` may require a small internal helper to avoid re-entry problems.
The builder must inspect `Session.cpp` before choosing the exact implementation.

Allowed implementation options:

Option A, preferred if clean:

- add a private/internal helper in `Session.cpp` that admits and appends a
  command without applying control commands;
- AI tick uses that helper to enqueue `CommandSource::Ai` commands before
  `pendingAcceptedCommands(state_)` is collected.

Option B, acceptable if A becomes too broad:

- keep `NpcBehaviorSystem` pure in the first source slice;
- add a later `Session::enqueueAiCommandsForTick` slice with focused tests.

No AppShell integration is allowed for behavior truth.

## Save, Load, Reset, And Hash

Current `AiActorState` is already saved, loaded, baselined, reset, and hashed.

When adding durable fields:

- update `SaveEnvelope.hpp`;
- update `SaveCodec.cpp` encode/decode;
- update `SaveLoad.cpp` save/load;
- update load validation if invalid enum/cooldown/target combinations exist;
- update `StateHash.cpp`;
- update baseline/reset tests;
- update save/load roundtrip tests.

Do not save derived perception facts:

- can see player;
- distance to target;
- target in range;
- line-of-sight result.

Those are computed each tick.

Save durable state only:

- behavior;
- target;
- next decision tick;
- policy id;
- cooldown;
- enabled flag.

## Receipts And Proof Fields

Product receipts are proof only.

Potential product fields:

```text
npc_behavior_enabled=true|false
npc_behavior_actor_count=<u64>
npc_behavior_tick_count=<u64>
npc_behavior_command_count=<u64>
npc_behavior_accepted_command_count=<u64>
npc_behavior_rejected_command_count=<u64>
npc_behavior_last_actor=<entity-id-or-none>
npc_behavior_last_actor_stable_name=<stable-name-or-none>
npc_behavior_last_state=<idle|alert|chasing|attacking|defeated|none>
npc_behavior_last_intent=<wait|move_toward_target|attack_target|none>
npc_behavior_last_target=<entity-id-or-none>
npc_behavior_last_target_stable_name=<stable-name-or-none>
npc_behavior_last_status=<status>
npc_behavior_attack_executed=true|false
npc_behavior_move_executed=true|false
npc_behavior_player_damaged=true|false
npc_behavior_npc_defeated=true|false
```

Receipt rules:

- receipts must not drive behavior;
- receipts must be deterministic key-value lines;
- receipts must not use JSON;
- product app may summarize runtime AI facts after a tick;
- no-window smoke should prove receipt values.

## Test Plan

Unit tests for `NpcBehaviorSystem`:

- disabled AI emits no command;
- inactive NPC emits no command;
- defeated NPC becomes `Defeated` and emits no command;
- no player target produces `Idle`;
- target outside perception radius produces `Idle`;
- target inside perception but outside attack range chooses `Chasing`;
- target inside attack range chooses `Attacking`;
- attack cooldown chooses `Alert` or wait;
- invalid config rejects deterministically;
- chase destination is finite and respects step/stop distances;
- decision order is deterministic by actor id.

Runtime/session tests:

- AI attack command uses `CommandSource::Ai`;
- AI command is admitted through normal command admission;
- AI attack reduces player HP through `CombatSystem`;
- defeated player is not attacked again;
- AI movement uses normal movement/collision behavior;
- AI rejected command is visible in command log;
- state hash changes when durable AI state changes.

Save/load tests:

- AI behavior fields roundtrip;
- invalid AI enum fails decode/load;
- cooldown and target fields roundtrip;
- reset returns AI to baseline.

Product no-window smoke:

```text
ASCII:
#####
#PN#
#####

Tape:
wait
```

Expected proof:

- app stays no-window;
- world enters gameplay;
- NPC behavior enabled;
- NPC perceives player;
- NPC attacks or chases according to configured distance;
- command source is AI;
- player HP changes if attack is in range;
- receipt fields prove the result.

## Compute Costs

V0.1 expected cost:

```text
O(N) NPC scan per tick
O(1) player target lookup for player slot 0
O(N) combatant lookup if implemented by linear scan
```

This is acceptable for first playable proof.

Cost rules:

- do not add spatial indexes in v0.1;
- do not add pathfinding in v0.1;
- do not allocate heavily per NPC decision;
- reserve vectors when emitting command intents;
- if combatant lookup becomes repeated, add a local per-tick lookup helper, not
  a global cache.

## Owned Files By Slice

Slice 1: contract and current-source alignment.

```text
docs/plan_bucket/npc_behavior_contract_v0_1.md
docs/plan_bucket/README.md
```

Slice 2: AI state vocabulary only.

```text
src/runtime/ai/AiState.hpp
tests/unit/session_state_tests.cpp
```

No session behavior change yet.

Slice 3: pure NPC behavior system.

```text
src/runtime/ai/NpcBehaviorSystem.hpp
src/runtime/ai/NpcBehaviorSystem.cpp
tests/unit/npc_behavior_system_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

No session integration yet.

Slice 4: save/load/hash for durable AI fields.

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.cpp
src/runtime/replay/StateHash.cpp
tests/unit/save_load_tests.cpp
tests/unit/session_state_tests.cpp
```

Slice 5: session enqueue/integration.

```text
src/runtime/session/Session.hpp
src/runtime/session/Session.cpp
src/runtime/session/SessionTick.hpp
src/runtime/session/SessionTick.cpp
tests/unit/session_tick_tests.cpp
tests/unit/session_runner_tests.cpp
tests/unit/combat_command_tests.cpp
```

Slice 6: product proof receipts.

```text
src/app/iggy3d/ReceiptBuilder.hpp
src/app/iggy3d/ReceiptBuilder.cpp
src/app/iggy3d/ProductGameplayTapeRunner.hpp
src/app/iggy3d/ProductGameplayTapeRunner.cpp
tests/smoke/product_gameplay_tape_smoke.cpp
```

Only add product receipts after runtime behavior exists.

## No-Go Files

Do not implement NPC behavior in:

```text
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/OpeningMenuView.cpp
src/render/**
src/render/vulkan/**
apps/**
```

Do not change:

```text
fixtures/**
docs/vulkan/**
```

unless a later slice explicitly requires fixture proof or Vulkan/debug draw
documentation.

## Acceptance Gate

The complete v0.1 batch is accepted when:

- NPC behavior state is typed and durable;
- pure perception and decision tests pass;
- behavior commands use `CommandSource::Ai`;
- AI commands enter normal session command admission/log/tick execution;
- NPC can attack the player in a no-window ASCII smoke;
- defeated NPC emits no further commands;
- save/load/hash cover new durable AI fields;
- receipts prove behavior without launching a window;
- no AI logic exists in AppShell;
- no renderer files are touched.

Required verification style:

```text
cmake --build build --target iggy3d_app
cmake --build build --target npc_behavior_system_tests
ctest --test-dir build --output-on-failure -R '^npc_behavior_system_tests$'
ctest --test-dir build --output-on-failure -R '^(session_tick|save_load|product_gameplay_tape)_'
git diff --check
```

No `--window` proof is required for v0.1.

## Implementation Stop Rules

Stop and return to planning if the builder needs:

- pathfinding;
- line-of-sight;
- patrol routes;
- multiple player target policy;
- editor-authored AI profiles;
- scenario-authored config schema;
- multiplayer authority rules;
- renderer debug draw;
- a broad `Session.cpp` rewrite;
- AppShell-owned AI behavior;
- direct HP/position mutation outside existing systems.

## Deferred Work

Later contracts should cover:

- patrol route authoring;
- guard posts;
- alert propagation;
- sound/noise perception;
- line-of-sight through collision surfaces;
- cover seeking;
- ranged attacks and spell casting;
- faction reputation;
- tactical slow-time decision cadence;
- multiplayer server authority;
- AI debug overlay and world-space draw.

## First Builder Order

First implementation slice after this contract:

```text
NPC Behavior v0.1 / Slice 2 - AI State Vocabulary
```

Scope:

- extend `AiState.hpp` with typed behavior and intent enums;
- add durable fields with conservative defaults;
- update only tests needed to prove default construction remains stable;
- do not wire behavior into session tick;
- do not change save/load/hash in this slice unless the added fields break
  existing serialization tests, in which case stop and split the save/hash slice.

Rationale:

The state vocabulary is the smallest durable seam. Once it is stable, the pure
behavior system can be built against typed data instead of temporary strings or
product receipt fields.
