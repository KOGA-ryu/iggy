# Runtime Packet 8: Tactical Time + Combat Spine

Status: builder-ready planning packet
Repo: `/Users/kogaryu/iggy3d`
Branch: `iggy3d-main`
Expected baseline HEAD: `3e22930 Add Vulkan first room proof packet`

This is a runtime/gameplay packet. It must not implement renderer, Vulkan,
SDL, windowing, shader, projection-output, editor, multiplayer transport, or
AI behavior. It adds the first deterministic tactical combat action to the
existing shippable demo loop.

## 1. Current Capability Audit

### Verified repo state at packet authoring

- `git status --short --branch` reported `## iggy3d-main...origin/iggy3d-main`.
- `git rev-parse --short HEAD` reported `3e22930`.
- No existing `/Users/kogaryu/iggy3d/docs/build_packets` convention existed, so
  this packet created the folder and this file.

### Existing clock and tactical mode

- Source:
  - `/Users/kogaryu/iggy3d/src/runtime/clock/ClockState.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/clock/Clock.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/clock/Clock.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/session/Session.cpp`
  - `/Users/kogaryu/iggy3d/tests/unit/clock_tests.cpp`
  - `/Users/kogaryu/iggy3d/tests/unit/session_runner_tests.cpp`
- Current behavior:
  - `ClockMode::{Normal, Slow, Paused}` exists.
  - `enterSlow`, `exitSlow`, `pause`, `resume`, `requestStep`,
    `consumeStep`, and `advanceTick` are pure deterministic helpers.
  - `Session::submitCommand` executes `ToggleTacticalMode`, `Pause`,
    `Resume`, and `StepTacticalTick` immediately.
  - `StepTacticalTick` calls `Session::stepOneTick()` and advances exactly one
    tick while paused.
- Missing for Packet 8:
  - combat action cost is not connected to clock/tick semantics.
  - no combat command exists to prove tactical mode affects gameplay execution.

### Existing command, target, reach, session tick

- Source:
  - `/Users/kogaryu/iggy3d/src/runtime/command/Command.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/targeting/TargetQuery.*`
  - `/Users/kogaryu/iggy3d/src/runtime/targeting/ReachQuery.*`
  - `/Users/kogaryu/iggy3d/tests/unit/command_admission_tests.cpp`
  - `/Users/kogaryu/iggy3d/tests/unit/session_tick_tests.cpp`
- Current behavior:
  - `CommandKind` covers movement, interaction, inspect, wait, tactical
    controls, retry, reset/save/load placeholders.
  - `CommandAdmission` is read-only and gates actor/slot, clock, target,
    reach, movement distance, retry source, and session-control legality.
  - `SessionTick` dispatches accepted queued `Move`, `Interact`, `Inspect`,
    `Wait`, and `Retry`.
  - Retry resolves into an `EffectiveCommandIntent`.
- Missing for Packet 8:
  - no `CommandKind::Attack`;
  - no `TargetAction::Attack`;
  - no combat command admission;
  - no combat dispatch in `SessionTick`;
  - no combat runtime event or metric.

### Existing combat state

- Source:
  - `/Users/kogaryu/iggy3d/src/runtime/combat/CombatState.hpp`
- File plans:
  - `/Users/kogaryu/iggy3d/docs/file_plans/src_runtime_combat_CombatState_hpp.md`
  - `/Users/kogaryu/iggy3d/docs/file_plans/src_runtime_combat_CombatSystem_hpp.md`
  - `/Users/kogaryu/iggy3d/docs/file_plans/src_runtime_combat_CombatSystem_cpp.md`
  - `/Users/kogaryu/iggy3d/docs/file_plans/tests_unit_combat_system_tests_cpp.md`
- Current behavior:
  - `CombatState` and `CombatantState` exist.
  - No `CombatSystem.hpp`, `CombatSystem.cpp`, or combat unit test exists in
    source.
  - File plans define deterministic `applyAttack` semantics, friendly-fire
    policy, no random rolls, and combat state as save/hash truth.
- Source/docs divergence:
  - The docs already describe `CombatSystem` as a complete API.
  - Source is currently state-only.
  - Packet 8 must implement the planned combat API and integrate it with
    commands/session tick.

### Existing save/load/replay/hash

- Source:
  - `/Users/kogaryu/iggy3d/src/runtime/save/SaveEnvelope.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/save/SaveLoad.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.hpp`
  - `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.cpp`
  - `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.cpp`
- Current behavior:
  - `CombatState::combatants` already serialize through save/load.
  - `StateHash` already includes combatants in vector order.
  - `CommandLogCounts` exists in `CommandLog.hpp` and is filled by
    `CommandLog::counts()`.
  - `CommandReplay` replays accepted queued and immediate command kinds.
  - `CommandPayload::userData0/userData1` are not saved by `SaveCommandRecord`;
    do not use them for combat damage semantics.
- Missing for Packet 8:
  - `CommandLogCounts` has no combat count.
  - `SaveCommandRecord` has no `attackDamage`.
  - `SaveLoad::referencesValid` does not yet validate combatant references,
    duplicate combatants, or HP/defeated invariants.
  - command save/load/hash support for `Attack` and explicit attack damage.
  - replay queue/immediate classification support for `Attack`.
  - expected summary proof fields for combat.

### Existing acceptance demo

- Source:
  - `/Users/kogaryu/iggy3d/tests/acceptance/complete_runtime_demo_tests.cpp`
  - `/Users/kogaryu/iggy3d/fixtures/demos/first_room/scenario.iggy3d.toml`
  - `/Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt`
  - `/Users/kogaryu/iggy3d/docs/acceptance_demo.md`
- Current behavior:
  - Ten submitted commands prove target/reach, move, retry pickup,
    tactical/pause/step/resume, save/load, reset, replay, summary, and final
    hash.
  - Current expected summary has `state_hash=46c911b0c2c0419a`.
  - No combat target/action is present.
- Packet 8 decision:
  - Intentionally extend the first-room fixture with a tactical combat target.
  - Replace the current `Wait` command in the ten-command script with one
    `Attack` command. The submitted command count remains ten.
  - Because the fixture and command script change intentionally, the
    `state_hash=46c911b0c2c0419a` expectation must be replaced by the new
    first-green locked value with rationale in the final Builder report.

## 2. Builder Scope

### Must add

#### `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.hpp`

Purpose: declare deterministic combat mutation API.

Owns:
- `CombatStatus`;
- `CombatAttackRequest`;
- `CombatAttackResult`;
- read-only `previewAttack(const CombatState&, const CombatAttackRequest&)`;
- `applyAttack(CombatState&, const CombatAttackRequest&)`.

Must not own:
- world active/inactive mutation;
- inventory/objective mutation;
- command admission;
- rendering/projection/input/save files/package parsing;
- random rolls or wall-clock timing.

Public API:

```cpp
enum class CombatStatus : std::uint8_t {
  Succeeded,
  InvalidCombatState,
  InvalidAttacker,
  InvalidTarget,
  AttackerDefeated,
  TargetDefeated,
  FriendlyFireBlocked,
  InvalidDamage,
};

struct CombatAttackRequest {
  EntityId attacker;
  EntityId target;
  std::int32_t damage = 0;
  CommandId sourceCommandId = kInvalidCommandId;
};

struct CombatAttackResult {
  CombatStatus status = CombatStatus::InvalidCombatState;
  EntityId attacker;
  EntityId target;
  std::int32_t damageApplied = 0;
  std::int32_t targetHitPoints = 0;
  bool targetDefeated = false;
  bool combatMutated = false;
};

CombatAttackResult previewAttack(
    const CombatState& combat,
    const CombatAttackRequest& request);

CombatAttackResult applyAttack(
    CombatState& combat,
    const CombatAttackRequest& request);
```

Ordering and error semantics:
- `previewAttack` and `applyAttack` must use the same internal validation path
  and first-failure ordering.
- `previewAttack` validates full combat state, projects `damageApplied`,
  `targetHitPoints`, and `targetDefeated`, never mutates, and always returns
  `combatMutated=false`.
- `applyAttack` uses the same validation path, mutates only on `Succeeded`, and
  returns `combatMutated=true` only on success.
- invalid failures mutate nothing.
- successful attack mutates only target combatant HP/defeated in `CombatState`.
- `CommandAdmission` must call `previewAttack`; it must not copy combat state
  for admission and must not duplicate combat validation logic.

Expected tests:
- covered by `/Users/kogaryu/iggy3d/tests/unit/combat_system_tests.cpp`.

#### `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.cpp`

Purpose: implement `previewAttack` and `applyAttack`.

Algorithm:
1. validate no duplicate combatant entity ids;
2. validate every combatant has `maxHitPoints > 0`, `hitPoints` in
   `[0, maxHitPoints]`, and `defeated == (hitPoints == 0)`;
3. find attacker in vector order;
4. find target in vector order;
5. reject defeated attacker as `AttackerDefeated`;
6. reject defeated target as `TargetDefeated`;
7. reject same nonzero faction as `FriendlyFireBlocked`;
8. reject damage `<= 0` as `InvalidDamage`;
9. subtract deterministic integer damage;
10. clamp target HP to zero;
11. set `defeated=true` if target HP reaches zero;
12. return exact result fields.

`previewAttack` uses this algorithm without writing to the vector.
`applyAttack` uses this algorithm and writes only after all validation passes.

No hidden static mutable state. No RNG. No wall-clock.

#### `/Users/kogaryu/iggy3d/tests/unit/combat_system_tests.cpp`

Purpose: prove deterministic combat system behavior.

Required assertions:
- empty combat state is structurally valid but attack with missing attacker
  returns `InvalidAttacker`;
- valid attack subtracts exact integer HP;
- lethal damage clamps HP to zero and sets defeated;
- lethal damage does not mutate world/entity active flags;
- invalid attacker or target fails without mutation;
- invalid combat state fails without mutation;
- invalid combat state coverage is exact: `maxHitPoints <= 0`,
  negative `hitPoints`, `hitPoints > maxHitPoints`, `defeated=true`
  with `hitPoints > 0`, and `defeated=false` with `hitPoints == 0`
  each return `InvalidCombatState`;
- same nonzero faction returns `FriendlyFireBlocked` without mutation;
- neutral faction `0` does not block by itself;
- different factions apply damage;
- defeated attacker returns `AttackerDefeated`;
- defeated target returns `TargetDefeated`;
- zero/negative damage returns `InvalidDamage`;
- `previewAttack` returns the same status/projected damage/projected target HP
  as `applyAttack` would, but leaves combat state unchanged and returns
  `combatMutated=false`;
- `applyAttack` returns `combatMutated=true` only on `Succeeded`;
- results are deterministic across repeated identical inputs.

#### `/Users/kogaryu/iggy3d/tests/unit/combat_command_tests.cpp`

Purpose: prove command admission and session tick integration for combat.

Required assertions:
- `CommandKind::Attack` requires actor and entity target.
- attack target must exist, be active, targetable, support `TargetAction::Attack`,
  and be in reach.
- out-of-range attack rejects with `OutOfRange`.
- invalid/missing combatant attacker maps to `InvalidActor`.
- invalid/missing combatant target maps to `InvalidTarget`.
- defeated attacker maps to `AttackerDefeated`.
- defeated target maps to `TargetDefeated`.
- same nonzero faction maps to `FriendlyFireBlocked`.
- invalid damage maps to `InvalidDamage`.
- accepted attack queues exactly once and executes through `SessionTick`.
- attack while `Paused` rejects as `SessionPaused`; attack accepted in `Slow`
  executes on normal tick; already queued attack executes on
  `StepTacticalTick` if it was accepted before pausing.
- rejected `InvalidDamage` attack can still be appended to command log and
  replayed as a rejected command.
- an accepted attack with `attackDamage <= 0` cannot exist after admission.
- `CommandLogCounts::combat` increments for submitted `Attack` records,
  including rejected attacks, matching submitted-kind count semantics.

### Must change

#### `/Users/kogaryu/iggy3d/src/runtime/command/Command.hpp`

Add:
- `CommandKind::Attack`;
- `CommandRejectionReason::{InvalidDamage, AttackerDefeated, TargetDefeated, FriendlyFireBlocked}`;
- explicit attack payload field:

```cpp
std::int32_t attackDamage = 0;
```

Do not use `userData0` or `userData1` for attack damage.

Helper updates:
- `requiresActor(Attack) == true`;
- `requiresEntityTarget(Attack) == true`;
- `requiresPointTarget(Attack) == false`;
- session-control helpers unchanged.

Save/replay impact:
- update every command-kind enum parse/format/count site.
- persist `attackDamage` in command save records and state hash.
- existing `userData0` and `userData1` may remain for compatibility but must
  not be read by admission, combat, save/load, replay, or summary for Attack
  semantics.

#### `/Users/kogaryu/iggy3d/src/runtime/world/EntityState.hpp`

Add `TargetAction::Attack`.

Do not add combat health to `EntityState`. Combat HP belongs to
`CombatState`.

#### `/Users/kogaryu/iggy3d/src/runtime/targeting/TargetQuery.cpp`

Map `CommandKind::Attack` to `TargetAction::Attack`.

Target query remains read-only and deterministic.

#### `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.hpp`

Add `const CombatState* combat = nullptr;` to `CommandAdmissionContext`.

Read-only only. Admission must not mutate combat.

#### `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.cpp`

Add attack legality in existing rule order:
- context validation requires `combat` for `Attack`;
- clock validation reuses existing paused behavior;
- target existence/activity and targetability run before combat-specific checks;
- reach validation applies to `Interact` and `Attack`;
- kind-specific attack rules validate combatants, defeated state, friendly fire,
  and positive damage.
- call `previewAttack(*context.combat, request)` for combat-specific legality;
  do not copy `CombatState` and do not duplicate `CombatSystem` validation
  logic in admission.

First-build attack range:
- use `RuntimeConfig::interactionRangeMeters`;
- actor transform position to target entity transform position;
- no bounds/closest-point policy.

Failure mapping:
- missing attacker combatant: `InvalidActor`;
- missing target combatant: `InvalidTarget`;
- attacker defeated: `AttackerDefeated`;
- target defeated: `TargetDefeated`;
- same nonzero faction: `FriendlyFireBlocked`;
- `attackDamage <= 0`: `InvalidDamage`;
- invalid combat state: `InternalError`;
- reach failure: existing reach rejection mapping, including `OutOfRange`.

`previewAttack` status mapping:
- `Succeeded`: command may be accepted if earlier rules passed;
- `InvalidCombatState`: `InternalError`;
- `InvalidAttacker`: `InvalidActor`;
- `InvalidTarget`: `InvalidTarget`;
- `AttackerDefeated`: `AttackerDefeated`;
- `TargetDefeated`: `TargetDefeated`;
- `FriendlyFireBlocked`: `FriendlyFireBlocked`;
- `InvalidDamage`: `InvalidDamage`.

#### `/Users/kogaryu/iggy3d/src/runtime/session/Session.cpp`

Update `CommandAdmissionContext` construction to pass `&state_.combat`.

Update queue policy:
- accepted `Attack` queues for tick execution;
- attack is never immediate.

Do not change command-id or append cursor policy.

#### `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.hpp`

`combatExecuted` already exists in `SessionTickResult`; keep it and use it.

#### `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.cpp`

Include `runtime/combat/CombatSystem.hpp`.

Dispatch order for each resolved effective command remains command-sequence
order. For an `Attack` intent:
1. build `CombatAttackRequest` from effective command actor, entity target,
   `payload.attackDamage`, and `intent.sourceCommandId`;
2. call `applyAttack(state.combat, request)`;
3. if status is not `Succeeded`, return `InvalidState` before processing later
   commands;
4. increment `result.combatExecuted`;
5. increment runtime combat metrics;
6. emit `RuntimeEventKind::CombatAttacked`;
7. if `targetDefeated`, emit `RuntimeEventKind::CombatantDefeated`;
8. push executed sequence exactly once.

Do not mutate world active flags on combat defeat.

#### `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeEvent.hpp`

Add:
- `CombatAttacked`;
- `CombatantDefeated`.

Events are transient diagnostics and excluded from save/hash.

#### `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeMetrics.hpp`

Add:
- `combatExecutions`;
- `combatDefeats`.

Metrics are transient and excluded from save/hash.

#### `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.cpp`

Add `Attack` to queued accepted command kinds.

Replay must resubmit attack proposals from the logged command, preserve
`attackDamage`, run the tick, and compare final hash/summary.

#### `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.cpp`

Add `CommandPayload::attackDamage` to command-record hash input.

Combat state hash already includes combatants in vector order; preserve that
order and add no transient result values.

#### `/Users/kogaryu/iggy3d/src/runtime/save/SaveEnvelope.hpp`

Add explicit command payload field:

```cpp
std::int32_t attackDamage = 0;
```

to `SaveCommandRecord`.

Combat state fields already exist; do not duplicate them elsewhere.

#### `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.cpp`

Add:
- command kind `Attack`;
- rejection reasons added in this packet;
- command record field `commandLog.record.N.attackDamage`;
- target action `Attack` if target actions are encoded anywhere in save text.

Compatibility:
- current save schema is v1; do not silently make existing v1 saves unreadable;
- encode `commandLog.record.N.attackDamage` for all newly written command
  records;
- decode a missing `commandLog.record.N.attackDamage` as `0`;
- require positive damage only through admission/command-log invariants for
  accepted `Attack` records, not as a generic decode requirement.

#### `/Users/kogaryu/iggy3d/src/runtime/save/SaveLoad.cpp`

Round-trip `CommandPayload::attackDamage`.

Combat state is already copied to/from save envelope; preserve vector order.

`SaveLoad::referencesValid` must also:
- verify every saved combatant references an existing world entity;
- reject duplicate combatant entity ids;
- enforce `maxHitPoints > 0`;
- enforce `0 <= hitPoints <= maxHitPoints`;
- enforce `defeated == (hitPoints == 0)`.

Bad saves must be rejected before a restored runtime session becomes usable.

#### `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.hpp`

Add to `CommandLogCounts`:

```cpp
std::uint64_t combat = 0;
```

Count semantics:
- `CommandLog::counts()` increments `combat` for every
  `CommandKind::Attack` record;
- the count is by submitted kind, matching current movement/interaction/control
  count style, not by successful execution.

Command-log payload invariants:
- `CommandLog::append` must preserve rejected `Attack` records, including
  rejected `InvalidDamage` records.
- Append validation must require actor/entity target shape for `Attack`.
- Append validation must require `attackDamage > 0` only for accepted `Attack`
  records.
- Rejected `Attack` records with `attackDamage <= 0` are valid log records when
  their admission/rejection fields reflect `InvalidDamage`.

#### `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.cpp`

Update counts/formatting to classify `Attack` as combat where command-log
counts are produced.

Implement the `CommandLogCounts::combat` behavior declared in
`CommandLog.hpp`. `RuntimeSummary` must prefer `counts.combat` for
`commands.combat`; scanning records is fallback-only if a local helper needs to
cross-check the count in tests.

#### `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.hpp`
#### `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.cpp`

Add summary fields:

```text
commands.combat=<integer>
combat.training_dummy.hp=<integer>
combat.training_dummy.defeated=true|false
combat.last_attack.command_id=<integer>
combat.last_attack.sequence=<integer>
combat.last_attack.damage=<integer>
combat.last_attack.target=training_dummy
```

If the target stable name changes, update all fields consistently.

`commands.combat` must come from `CommandLogCounts::combat`, not a separate
summary-owned counting rule.

#### `/Users/kogaryu/iggy3d/src/content/FixtureScenarioLoader.hpp`
#### `/Users/kogaryu/iggy3d/src/content/FixtureScenarioLoader.cpp`

Add optional combat authoring fields on `[[entities]]`:

```text
combatant = true|false
faction_id = <u32>
hit_points = <i32>
max_hit_points = <i32>
```

Add exact source-level seed shape:

```cpp
struct ScenarioCombatantSeed {
  bool enabled = false;
  bool hasFactionId = false;
  bool hasHitPoints = false;
  bool hasMaxHitPoints = false;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
};

struct ScenarioEntitySeed {
  // existing fields...
  ScenarioCombatantSeed combatant;
};
```

Parser additions:
- add signed `parseI32` for `hit_points` and `max_hit_points`;
- parse `TargetAction::Attack`;
- `combatant=true` enables combatant seed;
- if `combatant=true`, require `faction_id`, `hit_points`, and
  `max_hit_points`;
- if `faction_id`, `hit_points`, or `max_hit_points` appear while `combatant`
  is absent or false, reject deterministically as
  `ScenarioLoadStatus::MissingRequiredKey` with diagnostic code
  `scenario.missing_required_key`, matching existing required-field parser
  style for related grouped keys.

Compatibility impact:
- old fixtures remain valid because these fields are optional;
- if `combatant=true`, all three numeric fields are required;
- if combat fields are present while `combatant` is missing or false, reject as
  `MissingRequiredKey` / `scenario.missing_required_key`;
- reject negative HP, `max_hit_points <= 0`, `hit_points > max_hit_points`, and
  `hit_points == 0` because Packet 8 does not author defeated starting
  combatants;
- invalid numeric syntax rejects as `InvalidNumber`;
- combatants are stored in entity/world seed order and mapped to assigned
  `EntityId` during `Session::create`.

Also parse `TargetAction::Attack`.

#### `/Users/kogaryu/iggy3d/src/runtime/session/Session.cpp`

During `Session::create`, build `state.combat.combatants` from scenario entity
seeds after world IDs are assigned. Combatant order must follow seed/world
order.

Mapping:
- map each combatant seed by scenario entity stable name and entity seed order
  to the assigned `EntityId`;
- reject duplicate combatant entity ids;
- reject invalid HP invariants;
- return deterministic creation error `session.combat_seed_failed` if combat
  construction fails.

Baseline reset already copies combat; preserve it.

#### `/Users/kogaryu/iggy3d/fixtures/demos/first_room/scenario.iggy3d.toml`

Intentionally extend first-room fixture:
- player entity is combatant, `faction_id=1`, `hit_points=10`,
  `max_hit_points=10`;
- add `training_dummy` entity:
  - kind `Npc`;
  - position `(2.000,0.000,2.000)`;
  - active and persistent;
  - targetable;
  - target actions include `Attack` and `Inspect`;
  - combatant `true`;
  - `faction_id=2`;
  - `hit_points=3`;
  - `max_hit_points=3`.

Do not remove existing `gold_key` or `tactical_marker_alpha` facts.

#### `/Users/kogaryu/iggy3d/tests/acceptance/complete_runtime_demo_tests.cpp`

Replace the current `cmd_wait` step with `cmd_attack_dummy`.

Expected ten-command script:
1. `cmd_interact_oob`: rejected `Interact(player,gold_key)`, `OutOfRange`.
2. `cmd_move_to_key`: accepted move to `(2,0,0)`, tick executes.
3. `cmd_retry_key`: accepted retry, tick picks up key.
4. `cmd_enter_tactical`: accepted immediate `ToggleTacticalMode`, Slow.
5. `cmd_tactical_move`: accepted move to `(2,0,1)`, tick executes.
6. `cmd_pause`: accepted immediate `Pause`.
7. `cmd_step`: accepted immediate `StepTacticalTick`, advances one paused tick.
8. `cmd_resume`: accepted immediate `Resume`, returns Slow.
9. `cmd_attack_dummy`: accepted `Attack(player,training_dummy,damage=3)`,
   tick executes and defeats dummy.
10. `cmd_exit_tactical`: accepted immediate `ToggleTacticalMode`, returns
    Normal.

Expected final facts:
- submitted `10`;
- accepted `9`;
- rejected `1`;
- retry `1`;
- control `5`;
- movement `2`;
- combat `1`;
- final tick remains `5`;
- player final position remains `(2.000,0.000,1.000)`;
- `training_dummy.hp=0`;
- `training_dummy.defeated=true`;
- `gold_key.active=false`;
- `tactical_marker_alpha.active=true`;
- objective remains complete;
- lifecycle complete after finalization;
- save/load/replay hash pass.

#### `/Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt`

Update after first green implementation with the new combat fields and new
locked `state_hash`.

The old hash `46c911b0c2c0419a` must not be preserved after the intentional
fixture/script change.

#### `/Users/kogaryu/iggy3d/docs/acceptance_demo.md`

Update command table and summary to replace `cmd_wait` with
`cmd_attack_dummy`.

State the hash changed because Packet 8 intentionally extended first-room
fixture and command payload truth.

#### `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`

Register:
- `combat_system_tests`;
- `combat_command_tests`.

Labels:
- `unit;runtime;combat;iggy3d`;
- `unit;runtime;command;combat;iggy3d`.

#### `/Users/kogaryu/iggy3d/CMakeLists.txt`

Add `src/runtime/combat/CombatSystem.cpp` to `iggy3d` target sources.

### May edit only if needed

- `/Users/kogaryu/iggy3d/tests/unit/command_admission_tests.cpp`: strengthen
  existing command admission coverage if `combat_command_tests.cpp` cannot
  isolate all attack admission assertions cleanly.
- `/Users/kogaryu/iggy3d/tests/unit/save_load_tests.cpp`: add focused attack
  command payload round-trip. Required focused cases:
  - old-style non-`Attack` command record with no `attackDamage` decodes with
    default `0` if optional-key parsing supports that fixture shape;
  - new `Attack` command record round-trips exact `attackDamage`;
  - accepted `Attack` with missing or non-positive damage is rejected by
    invariants/admission before it can become a usable loaded session;
  - load rejects combatants referencing missing world entities;
  - load rejects duplicate combatant entity ids;
  - load rejects invalid HP/defeated invariants.
- `/Users/kogaryu/iggy3d/tests/unit/session_tick_tests.cpp`: add focused attack
  tick execution if `combat_command_tests.cpp` does not cover queue/step
  behavior.
- `/Users/kogaryu/iggy3d/tests/unit/package_loader_tests.cpp`: add parser tests
  for optional combatant fields and `TargetAction::Attack`. Required focused
  cases:
  - old first-room fixture without combat fields still loads if used in a
    compatibility fixture;
  - `TargetAction::Attack` parses;
  - stray `faction_id`, `hit_points`, or `max_hit_points` without
    `combatant=true` rejects as `MissingRequiredKey` /
    `scenario.missing_required_key`;
  - negative HP, `max_hit_points <= 0`, `hit_points > max_hit_points`, and
    starting `hit_points == 0` reject deterministically.

## 3. Runtime Semantics

### Runtime mode

- Normal realtime: `ClockMode::Normal`, `timeScale=1.0`.
- Tactical slow time: `ClockMode::Slow`, `timeScale=config.slowTimeScale`.
- Paused: `ClockMode::Paused`, `timeScale=0.0`.
- `ToggleTacticalMode` changes Normal to Slow and Slow to Normal.
- `Pause` stores previous unpaused mode and scale.
- `Resume` restores previous unpaused mode and scale.
- `StepTacticalTick` is valid only while paused and advances exactly one tick.

### Tactical command execution

- `Attack` is a queued gameplay command like `Move` and `Interact`.
- `Attack` is allowed in Normal and Slow.
- `Attack` is rejected while Paused unless it was already accepted before
  pausing and is executed by `StepTacticalTick`.
- One attack costs one deterministic session tick.
- Packet 8 adds no cooldown state. Repeated attacks are ordered by command
  sequence and each accepted attack consumes one tick when executed.

### Attack command path

1. caller submits pending `CommandRecord{kind=Attack}`;
2. `Session` assigns `commandId`;
3. `CommandAdmission` validates slot, actor, clock, target, reach, combatant
   state, friendly fire, and damage;
4. `CommandLog::append` assigns sequence;
5. accepted attack queues once in `pendingExecutionSequences`;
6. `SessionTick` resolves effective intent;
7. `CombatSystem::applyAttack` mutates `CombatState` only;
8. session emits combat transient events/metrics;
9. tick advances exactly once;
10. `StateHash` changes because combat state changed.

### Target discovery and target identity

- `TargetQuery` remains read-only.
- `TargetAction::Attack` is a targetability action on an entity.
- Target identity is `EntityId`; stable names are authoring/test references
  only.
- The Packet 8 target is `training_dummy`; its assigned id follows fixture
  seed order.

### Reach checks

- Attack reach uses actor transform position to target entity transform
  position.
- First-build attack range uses `RuntimeConfig::interactionRangeMeters`.
- No bounds, closest point, or local bounds reach policy.
- Out of range maps to existing `OutOfRange`.

### Conflict ordering

- Session executes queued accepted commands in command log sequence order.
- If multiple attacks target the same combatant in one tick, the lower sequence
  applies first.
- Later commands observe earlier mutations.
- No unordered container iteration may affect outcome.

### Save/load/replay/hash

- Save/load persists:
  - combatants vector;
  - command kind `Attack`;
  - command payload `attackDamage`;
  - command log sequence/ids;
  - clock/tactical state.
- State hash includes:
  - combatants in vector order;
  - attack command payload damage in command records;
  - command log state already included.
- State hash excludes:
  - combat result structs;
  - runtime events;
  - runtime metrics;
  - renderer output;
  - wall-clock data.
- Replay must reproduce attack admission, execution, HP, defeat flag, final
  hash, and summary.

## 4. Data Ownership

- `Session` owns aggregate runtime truth, command id allocation, queue
  handoff, reset baseline, save/load replacement, and state hash recompute.
- `CommandAdmission` owns read-only command legality and exact rejection reason
  selection. It never mutates world, combat, clock, command log, inventory, or
  objectives.
- `CombatSystem` owns combat mutation semantics inside `CombatState` only.
- `Clock` owns pure state transition helpers for simulation mode, time scale,
  and step requests.
- `SessionTick` owns deterministic dispatch order and calls the owning
  subsystem for mutation.
- `SaveLoad` serializes durable state and command records; it does not invent
  gameplay semantics.
- Fixture content describes starting state and expected proof. It does not
  execute commands.

## 5. No-Go Surfaces

- No renderer, Vulkan, SDL, window, swapchain, shader, GPU, screenshot, or frame
  hash dependency in runtime/content/projection/save/combat code.
- No multiplayer implementation in Packet 8.
- No AI behavior beyond preserving existing dormant state.
- No broad package schema churn. Only optional entity combatant fields and
  target action `Attack` are approved.
- No new machine-contract format. Use existing TOML-style fixtures and
  deterministic key-value summaries.
- No unrelated refactors.
- No hidden global state.
- No wall-clock randomness.
- No pointer-address ordering.
- No unordered-map iteration affecting results.
- No old `/Users/kogaryu/iggy` dependency.

## 6. Acceptance Demo Contract

Packet 8 intentionally changes the first-room fixture and summary.

The demo must prove:
- target discovery can identify an attackable target;
- target/reach validation rejects invalid or out-of-range attack in unit tests;
- runtime enters tactical slow time;
- one accepted tactical attack executes through session tick;
- combat result is deterministic;
- target HP and defeated flag mutate only in `CombatState`;
- save/load preserves combat state and attack command payload;
- replay reproduces the same final hash and summary;
- reset still restores baseline combat state;
- renderer proof remains unaffected by runtime combat output.

Acceptance summary additions:

```text
commands.combat=1
combat.training_dummy.hp=0
combat.training_dummy.defeated=true
combat.last_attack.command_id=9
combat.last_attack.sequence=9
combat.last_attack.damage=3
combat.last_attack.target=training_dummy
```

The old summary hash `46c911b0c2c0419a` is no longer valid after this packet.
Builder must update the summary hash only after default, Werror, headless,
save/load, replay, and targeted combat tests are green.

## 7. Verification Commands

Run from `/Users/kogaryu/iggy3d`.

Default configure/build/test:

```sh
cmake -S /Users/kogaryu/iggy3d -B /tmp/iggy3d-build
cmake --build /tmp/iggy3d-build
ctest --test-dir /tmp/iggy3d-build --output-on-failure
```

Targeted runtime tests:

```sh
ctest --test-dir /tmp/iggy3d-build --output-on-failure \
  -R 'combat_system|combat_command|clock|command_admission|session_tick|session_runner|save_load|complete_runtime_demo'
```

Headless demo and replay proof:

```sh
/tmp/iggy3d-build/iggy3d_headless_demo \
  --fixture /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml \
  --summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt \
  --save /tmp/iggy3d_packet8_runtime.save

/tmp/iggy3d-build/iggy3d_replay_tool \
  --fixture /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml \
  --save /tmp/iggy3d_packet8_runtime.save \
  --summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt
```

Warnings-as-errors:

```sh
cmake -S /Users/kogaryu/iggy3d -B /tmp/iggy3d-build-werror \
  -DIGGY3D_WARNINGS_AS_ERRORS=ON
cmake --build /tmp/iggy3d-build-werror
ctest --test-dir /tmp/iggy3d-build-werror --output-on-failure
```

Runtime graphics firewall:

```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' \
  /Users/kogaryu/iggy3d/src/runtime \
  /Users/kogaryu/iggy3d/src/content \
  /Users/kogaryu/iggy3d/src/projection \
  /Users/kogaryu/iggy3d/src/runtime/save
```

Expected: no matches.

No new format drift in runtime/demo formats:

```sh
rg -n 'json|JSON|Json|StatusCode|namespace runtime3d|iggy::three_d|/Users/kogaryu/iggy' \
  /Users/kogaryu/iggy3d/src/runtime \
  /Users/kogaryu/iggy3d/src/content \
  /Users/kogaryu/iggy3d/tests \
  /Users/kogaryu/iggy3d/fixtures \
  /Users/kogaryu/iggy3d/docs/build_packets/runtime_packet_8_tactical_combat.md
```

Expected: no new matches except the scan expression itself if echoed by a tool.

Patch hygiene:

```sh
git -C /Users/kogaryu/iggy3d diff --check
git -C /Users/kogaryu/iggy3d status --short
```

## 8. Builder Handoff

### Packet title

Runtime Packet 8: Tactical Time + Combat Spine

### Files approved

Approved must-edit list:
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/combat/CombatSystem.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/combat_system_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/combat_command_tests.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/command/Command.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/world/EntityState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/targeting/TargetQuery.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeEvent.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeMetrics.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveEnvelope.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveLoad.cpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.cpp`
- `/Users/kogaryu/iggy3d/src/content/FixtureScenarioLoader.hpp`
- `/Users/kogaryu/iggy3d/src/content/FixtureScenarioLoader.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/package_loader_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/save_load_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/acceptance/complete_runtime_demo_tests.cpp`
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/scenario.iggy3d.toml`
- `/Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt`
- `/Users/kogaryu/iggy3d/docs/acceptance_demo.md`
- `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`
- `/Users/kogaryu/iggy3d/CMakeLists.txt`

May-edit list:
- files named in "May edit only if needed" above.

Forbidden:
- `/Users/kogaryu/iggy3d/src/render/**`
- `/Users/kogaryu/iggy3d/src/app/platform/**`
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/**`
- Vulkan/SDL/shader/CMake renderer files unless a build-system source list
  mechanically requires no-op preservation.
- `/Users/kogaryu/iggy`

### Exact behavior to implement

Implement deterministic `Attack` command support from command creation through
admission, command log, session tick, combat mutation, save/load, replay,
state hash, runtime summary, and complete runtime demo.

### Proof required before reporting green

- default configure/build/CTest green;
- focused combat/clock/command/session/save/replay/acceptance tests green;
- headless demo summary match green;
- replay tool summary/hash match green;
- Werror green;
- firewall/no-format-drift scans clean or explained;
- `git diff --check` clean.

### Expected Builder final report

Builder Dex should report:
1. files changed;
2. exact command/script changes;
3. new summary fields and new locked state hash;
4. test and command outputs summarized;
5. firewall/no-format scan results;
6. remaining risks or `No remaining blocker in Packet 8`.

Builder must stop after Packet 8 and report. Do not commit or push unless
explicitly asked by lead.

## 9. Next Packet Scout

Likely next packet: Multiplayer Authority + Replication Packet 1.

Prerequisites before starting:
- `Attack` command exists as deterministic command payload;
- admission exposes exact combat rejection reasons;
- combat state is saved, loaded, replayed, and hashed;
- command log and replay can reproduce the combat action;
- tactical clock state is already authoritative and command-driven.

Suggested next-packet scope:
- authority policy for local/remote command submit;
- packet-safe command serialization for Move/Interact/Retry/Attack/control;
- deterministic merge/order of local and remote command proposals;
- replication proof over command log/state hash, not renderer output.

Do not start multiplayer until Packet 8 combat semantics are green.

## Open Decisions

None blocking.

Conservative defaults chosen by this packet:
- first-room fixture is intentionally extended;
- command count remains ten by replacing `Wait` with `Attack`;
- first-build attack range reuses `interactionRangeMeters`;
- first-build attack damage is explicit command payload `attackDamage`;
- no cooldown state in Packet 8;
- combat defeat does not deactivate world entities.
