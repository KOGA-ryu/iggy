# NPC Behavior Contract v0.1

## Objective

Define the durable first NPC behavior system for Iggy3D.

The goal is not to make a clever AI layer first. The goal is to add a
deterministic runtime-owned behavior loop that can be tested from generated
room/session data without launching a window. ASCII may author the map used by a
test fixture, but ASCII is map making only and is not behavior truth.

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

## Current Implemented Baseline

NPC Behavior v0.1 is now implemented as a runtime-owned baseline.

Current implemented truth:

- typed AI behavior and intent vocabulary exists in `AiState.hpp`;
- durable AI state includes target, behavior, last intent, decision tick,
  deterministic policy, cooldown, and enabled flag;
- save/load/hash covers the durable AI target, behavior, last intent, and
  cooldown fields;
- pure `NpcBehaviorSystem` owns perception, decision, and command building;
- `CommandAdmission` accepts `CommandSource::Ai` commands without a player
  slot while preserving normal actor, target, combat, movement, reach, and
  lifecycle checks;
- `CommandLog` accepts and restores AI `Move`, `Attack`, and `Wait` records
  with invalid/default player slots through the same narrow AI policy;
- `Session::tick()` enqueues NPC behavior commands before normal pending
  command execution and then applies accepted commands through existing
  movement/combat tick systems;
- product gameplay tape receipts prove current-run AI command logging, AI
  attack logging, wait logging, player damage, AI actor/target ids, behavior,
  and intent;
- synthesized product-package players are attack/inspect targetable for NPC
  proof while remaining separate from NPC command ownership;
- runtime edge tests cover chase movement through normal `Move`, rejected AI
  command-log visibility, and defeated-player no-attack behavior.

Latest focused proof targets:

```text
save_load_tests
session_state_tests
npc_behavior_system_tests
command_admission_tests
session_tick_tests
product_gameplay_tape_runner_tests
product_gameplay_tape_smoke
product_ascii_gameplay_loop_smoke
```

## NPC Behavior v0.2 Profile Baseline

NPC Behavior v0.2 now has a runtime-owned behavior profile seam. Profiles are
runtime AI configuration, not map source, ASCII source, product automation, or
renderer state.

Current profile baseline:

- `NpcEngagementPolicy::Hostile` and `NpcEngagementPolicy::Passive` exist;
- `NpcBehaviorProfile`, `NpcBehaviorProfileCatalog`, and profile resolution
  result/request types exist;
- built-in profiles are `default`, `melee_training`, and `passive`;
- `default` and `melee_training` resolve to hostile v0.1 numeric defaults;
- `passive` resolves to valid numeric defaults plus `Passive` engagement
  policy;
- `AiActorState::behaviorProfileId` defaults to `default` and is durable;
- save text writes the actor profile key as
  `ai.actor.<index>.behavior_profile_id`;
- old saves missing the actor profile key decode and load as `default`;
- state hash includes `behaviorProfileId`;
- `Session::tick()` resolves each AI actor profile through the built-in catalog
  before perception, decision, and command building;
- missing, invalid, or empty profile ids fail closed during live ticks: no
  command is generated and AI state is not mutated for that actor;
- `passive` actors may perceive the player and enter `Alert` / `Wait`, but do
  not chase or attack.

Profile resolution is intentionally not authored from map source in this
baseline. Package/scenario profile assignment is deferred.

## Historical Source Baseline

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

This was the pre-v0.1 baseline. It proved that AI already belonged to runtime
session state, but did not yet express actual NPC behavior.

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

`SaveLoad.cpp` already wrote and read `AiActorState` records before v0.1. The
v0.1 durable fields have since been added to the save envelope, codec, load
validation, load replacement path, and state hash.

Existing state hash support:

```text
src/runtime/replay/StateHash.cpp
```

AI state already participated in state hashing before v0.1. The v0.1 durable
fields now participate in state hash immediately.

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

Existing map-authored/package NPC creation:

```text
src/app/iggy3d/AsciiRoomGrid.hpp
src/app/iggy3d/AsciiRoomGrid.cpp
src/app/iggy3d/ProductPackageSessionSeed.hpp
src/app/iggy3d/ProductPackageSessionSeed.cpp
```

Useful existing facts:

- map-authoring glyph `N` compiles into an NPC spawn;
- map-authoring glyph `M` compiles into a monster spawn;
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
- run an attack against a generated NPC entity;
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

Implemented v0.1 durable shape:

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

Current runtime defaults:

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

Implemented files:

```text
src/runtime/ai/NpcBehaviorSystem.hpp
src/runtime/ai/NpcBehaviorSystem.cpp
tests/unit/npc_behavior_system_tests.cpp
```

Extended AI state file:

```text
src/runtime/ai/AiState.hpp
```

Implemented save/hash coverage files:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.cpp
src/runtime/replay/StateHash.cpp
tests/unit/save_load_tests.cpp
```

These files were updated in the same implementation ladder as the durable AI
fields.

Implemented public functions:

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

Implemented v0.1 integration:

```text
Session::tick
-> before pending command execution, ask AI for commands
-> submit each AI command through Session::submitCommand or an equivalent
   internal admission path that appends to CommandLog
-> runSessionTick executes the accepted pending commands
```

The landed implementation uses runtime-owned session helpers to enqueue
`CommandSource::Ai` records before `pendingAcceptedCommands(state_)` is
collected. AI commands are admitted and appended to `CommandLog`; accepted
commands are then executed by the existing tick path.

No AppShell integration is allowed for behavior truth.

## Save, Load, Reset, And Hash

Current `AiActorState` is saved, loaded, baselined, reset, and hashed.

The v0.1 durable fields are covered in:

- `SaveEnvelope.hpp`;
- `SaveCodec.cpp` encode/decode;
- `SaveLoad.cpp` save/load;
- load validation for enum text;
- `StateHash.cpp`;
- baseline/reset tests;
- save/load roundtrip tests.

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

Implemented product gameplay tape proof fields:

```text
gameplay_tape_ai_command_logged=true|false
gameplay_tape_ai_attack_logged=true|false
gameplay_tape_ai_wait_logged=true|false
gameplay_tape_ai_player_damaged=true|false
gameplay_tape_ai_player_hp_before=<int>
gameplay_tape_ai_player_hp_after=<int>
gameplay_tape_ai_actor_id=<id-or-none>
gameplay_tape_ai_target_id=<id-or-none>
gameplay_tape_ai_behavior=<idle|alert|chasing|attacking|defeated|none>
gameplay_tape_ai_intent=<wait|move_toward_target|attack_target|none>
```

Receipt rules:

- receipts must not drive behavior;
- receipts must be deterministic key-value lines;
- receipts must not use JSON;
- product app summarizes runtime AI facts after a tape run;
- no-window smoke proves receipt values;
- current-run receipt semantics must ignore AI command-log records that existed
  before the tape run.

## Implemented Proof Plan

Unit tests for `NpcBehaviorSystem` cover:

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

Runtime/session tests cover:

- AI attack command uses `CommandSource::Ai`;
- AI command is admitted through normal command admission;
- AI attack reduces player HP through `CombatSystem`;
- defeated player is not attacked again;
- AI movement uses normal movement/collision behavior;
- AI rejected command is visible in command log;
- state hash changes when durable AI state changes.

Save/load tests cover:

- AI behavior fields roundtrip;
- invalid AI enum fails decode/load;
- cooldown and target fields roundtrip;
- reset returns AI to baseline.

Product no-window proof uses gameplay tape waits to drive autonomous NPC ticks
against a generated room. The example source below is map-authoring input only;
the runtime test operates on the compiled room/session data.

```text
ASCII:
#####
#PN#
#####

Tape:
wait
```

Implemented proof:

- app stays no-window;
- world enters gameplay;
- NPC behavior is enabled by runtime state;
- NPC perceives player by distance;
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

## Completed Implementation Slices

Slice 1 completed: contract and current-source alignment.

```text
docs/plan_bucket/npc_behavior_contract_v0_1.md
docs/plan_bucket/README.md
```

Slice 2 completed: durable AI state vocabulary, persistence, and hash
coverage.

```text
src/runtime/ai/AiState.hpp
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.cpp
src/runtime/replay/StateHash.cpp
tests/unit/save_load_tests.cpp
tests/unit/session_state_tests.cpp
```

No session behavior changed in that slice.

Slice 3 completed: pure NPC behavior system.

```text
src/runtime/ai/NpcBehaviorSystem.hpp
src/runtime/ai/NpcBehaviorSystem.cpp
tests/unit/npc_behavior_system_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

No session integration changed in that slice.

Slice 4 completed: AI command admission policy.

```text
src/runtime/command/CommandAdmission.cpp
tests/unit/command_admission_tests.cpp
```

Slice 5 completed: session enqueue/integration and command-log AI slot
invariant.

```text
src/runtime/session/Session.cpp
src/runtime/replay/CommandLog.cpp
tests/unit/session_tick_tests.cpp
tests/unit/session_state_tests.cpp
```

Slice 6 completed: product proof receipts and current-run honesty repair.

```text
src/app/iggy3d/ReceiptBuilder.hpp
src/app/iggy3d/ReceiptBuilder.cpp
src/app/iggy3d/ProductGameplayTapeRunner.hpp
src/app/iggy3d/ProductGameplayTapeRunner.cpp
tests/smoke/product_gameplay_tape_smoke.cpp
```

Slice 7 completed: runtime edge proofs.

```text
tests/unit/session_tick_tests.cpp
```

v0.2 Slice 1 completed: engagement policy and profile model.

```text
src/runtime/ai/NpcBehaviorSystem.hpp
src/runtime/ai/NpcBehaviorSystem.cpp
src/runtime/ai/NpcBehaviorProfile.hpp
src/runtime/ai/NpcBehaviorProfile.cpp
tests/unit/npc_behavior_profile_tests.cpp
tests/unit/npc_behavior_system_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

v0.2 Slice 2 completed: durable actor profile binding.

```text
src/runtime/ai/AiState.hpp
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveLoad.cpp
src/runtime/replay/StateHash.cpp
tests/unit/save_load_tests.cpp
tests/unit/session_state_tests.cpp
```

v0.2 Slice 3 completed: live session profile resolution.

```text
src/runtime/session/Session.cpp
tests/unit/session_tick_tests.cpp
```

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

The complete v0.1 batch is satisfied.

Accepted baseline:

- NPC behavior state is typed and durable;
- pure perception and decision tests pass;
- behavior commands use `CommandSource::Ai`;
- AI commands enter normal session command admission/log/tick execution;
- NPC can attack the player in a no-window generated-room smoke;
- defeated NPC emits no further commands;
- save/load/hash cover new durable AI fields;
- product gameplay tape receipts prove behavior without launching a window;
- no AI logic exists in AppShell;
- no renderer files are touched.

Latest focused verification style:

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

## Next NPC Behavior Work

Later contracts should cover:

- authored/scenario profile source model;
- profile assignment from package or authoring data;
- profile diagnostics, receipts, and debug overlay;
- simple patrol or guard anchors from map-authored/package markers;
- line-of-sight and perception through spatial surfaces;
- pathfinding and navigation surfaces;
- replay/multiplayer authority policy once networking begins;
- patrol route authoring;
- guard posts;
- alert propagation;
- sound/noise perception;
- cover seeking;
- ranged attacks and spell casting;
- faction reputation;
- tactical slow-time decision cadence;
