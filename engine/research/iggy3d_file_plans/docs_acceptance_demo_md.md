# `docs/acceptance_demo.md`

Updated: 2026-06-20

Exact purpose: define the first shippable `iggy3d` demo in runtime terms,
including fixture data, command script, expected state transitions, save/load,
retry/reset, replay, diagnostics, and deterministic summary output.

## Authority Of This Document

This document is the acceptance contract for the first complete runtime build.

The builder is done with the runtime loop only when the headless demo app and
acceptance test prove every requirement here without renderer code and without
old `/Users/kogaryu/iggy` dependencies.

If implementation details require changing this script, update this document,
the fixture plans, the expected summary plan, and the acceptance test plan in
the same review.

## Demo Identity

Fixture directory:

```text
fixtures/demos/first_room
```

Package manifest:

```text
fixtures/demos/first_room/package.iggy3d.toml
```

Scenario file:

```text
fixtures/demos/first_room/scenario.iggy3d.toml
```

Expected summary:

```text
fixtures/demos/first_room/expected_summary.txt
```

Package id:

```text
iggy3d.first_room
```

Scenario id:

```text
first_room.runtime_loop
```

Required proof executables:

```text
iggy3d_headless_demo
iggy3d_validate_package
iggy3d_replay_tool
```

No renderer, window, GPU API, old `iggy` code, or app-specific raw input is
required.

## Demo Purpose

The demo proves the runtime product loop:

1. find a target;
2. reject an out-of-range interaction;
3. move into range;
4. retry the rejected interaction through normal command admission;
5. execute interaction effects;
6. mutate inventory and objective state;
7. switch from realtime camera to tactical slow-time camera;
8. pause, step, and resume;
9. return to realtime camera;
10. save and load;
11. reset to baseline;
12. replay command history;
13. match deterministic summary and state hash.

This is the minimum shippable demo for runtime. It is not a renderer demo.

## Coordinate And Unit Contract

Coordinate system:

- X: right;
- Y: up;
- Z: forward.

Units:

- positions are meters;
- distances are meters;
- summary vector formatting is fixed to three decimals;
- state hash uses canonical quantized vector values as defined by `StateHash`.

## Runtime Defaults

The acceptance demo uses these runtime defaults unless `RuntimeConfig` names a
stricter value:

- fixed tick rate: `20` Hz;
- interaction range: `1.500` meters;
- movement per command: `3.000` meters;
- default realtime camera: `ThirdPerson`;
- alternate realtime camera supported by config: `FirstPerson`;
- default tactical camera: `TacticalOverhead`;
- initial clock mode: `Normal`;
- initial camera mode: `ThirdPerson`;
- initial lifecycle: `Playing`.

The acceptance test may assert config defaults directly if they are exposed by
`RuntimeConfig`.

## Fixture Entity Contract

Entities are allocated in deterministic scenario order. The expected ids below
assume allocation starts at `1`.

| EntityId | Stable Name | Kind | Initial Position | Active | Runtime Meaning |
| --- | --- | --- | --- | --- | --- |
| `1` | `player` | `Player` | `(0.000,0.000,0.000)` | `true` | actor bound to player slot 0 |
| `2` | `gold_key` | `Pickup` | `(3.000,0.000,0.000)` | `true` | interactable pickup and objective item |
| `3` | `tactical_marker_alpha` | `Marker` | `(2.000,0.000,1.000)` | `true` | tactical movement destination |

Required entity metadata:

- `player` has targetability disabled for self-interact in this demo;
- `gold_key` is targetable for `Interact` and `Inspect`;
- `gold_key` has item id `gold_key`, count `1`, interaction kind `Pickup`;
- `gold_key` bounds must make the closest reach point equivalent to its
  position for this demo unless bounds are explicitly defined in the fixture;
- `tactical_marker_alpha` is targetable as a tactical point/marker but is not
  collectible;
- all entities use finite transforms and valid bounds.

## Player And Objective Contract

Player roster:

- player slot `0`;
- slot kind `Local`;
- actor binding: `EntityId(1)`;
- authority mode: local authoritative.

Objective:

- objective id: `collect_gold_key`;
- initial status: `Active`;
- completion condition: player slot 0 inventory contains `gold_key:1`;
- final status: `Complete`;
- completion emits a runtime event.

Initial inventory:

```text
player0=<empty>
```

Final inventory:

```text
player0=gold_key:1
```

## Preflight Validation

Before gameplay commands run, the demo must prove:

1. package manifest loads;
2. package validator accepts package id `iggy3d.first_room`;
3. scenario loader reads scenario id `first_room.runtime_loop`;
4. seed entity count is exactly `3`;
5. objective count is exactly `1`;
6. player slot 0 binds to `player`;
7. baseline state hash can be computed;
8. runtime summary can be generated for the initial state if requested.

Any preflight failure aborts the demo with nonzero process exit.

## Command Identity Contract

Command ids and sequence numbers must be deterministic for the scripted demo.

The exact numeric ids may be assigned by `CommandLog`, but the acceptance test
must be able to identify commands by stable labels:

| Label | Kind | Expected Admission | Notes |
| --- | --- | --- | --- |
| `cmd_interact_oob` | `Interact` | rejected | first interaction with `gold_key`, reason `OutOfRange` |
| `cmd_move_to_key` | `Move` | accepted | moves player to `(2.000,0.000,0.000)` |
| `cmd_retry_key` | `Retry` | accepted | references `cmd_interact_oob` |
| `cmd_enter_tactical` | `ToggleTacticalMode` | accepted | enters slow-time tactical camera |
| `cmd_tactical_move` | `Move` | accepted | moves player to marker `(2.000,0.000,1.000)` |
| `cmd_pause` | `Pause` | accepted | enters paused mode |
| `cmd_step` | `StepTacticalTick` | accepted | advances exactly one tick while paused |
| `cmd_resume` | `Resume` | accepted | returns to slow-time mode |
| `cmd_wait` | `Wait` | accepted | accepted no-op for command/log proof |
| `cmd_exit_tactical` | `ToggleTacticalMode` | accepted | returns to normal realtime camera |

Expected counts:

- submitted gameplay commands: `10`;
- accepted gameplay commands: `9`;
- rejected gameplay commands: `1`;
- retry commands: `1`;
- movement commands accepted: `2`;
- interaction commands executed: `1`;
- control commands accepted: `5` (`ToggleTacticalMode`, `Pause`,
  `StepTacticalTick`, `Resume`, `ToggleTacticalMode`);
- no-op wait commands accepted: `1`.

Save, load, reset proof, and replay proof are control/tool phases. They may
produce runtime events and diagnostics, but they are not counted in the ten
gameplay command labels above unless the implementation explicitly models them
as command records. If modeled as command records, summary fields must separate
`commands.gameplay.*` from `commands.control_proof.*`.

## Scripted Runtime Flow

### Step 0: Initial State

Expected:

- lifecycle `Playing`;
- clock `Normal`;
- camera `ThirdPerson`;
- previous realtime camera `ThirdPerson`;
- player position `(0.000,0.000,0.000)`;
- `gold_key.active=true`;
- inventory empty;
- objective `collect_gold_key=Active`;
- command log empty;
- baseline hash captured for reset proof.

### Step 1: Target Discovery

Operation:

```text
TargetQuery(actor=player, kind=Interact)
```

Expected:

- result status `Found`;
- target entity `gold_key`;
- target id `2`;
- target distance `3.000` meters before reach adjustment;
- no command log entry;
- no state mutation.

### Step 2: Out-Of-Range Interaction Rejection

Command:

```text
cmd_interact_oob = Interact(actor=player, target=gold_key)
```

Expected:

- authority accepted;
- admission rejected;
- rejection reason `OutOfRange`;
- command log contains rejected command;
- interaction system does not execute;
- player position unchanged;
- inventory unchanged;
- objective unchanged;
- `gold_key.active=true`;
- runtime event `CommandRejected`.

### Step 3: Move Into Reach

Command:

```text
cmd_move_to_key = Move(actor=player, targetPoint=(2.000,0.000,0.000))
```

Expected:

- command accepted;
- movement system executes;
- player position becomes `(2.000,0.000,0.000)`;
- player is within `1.500` meters of `gold_key`;
- inventory unchanged;
- objective unchanged;
- runtime event `Moved`.

### Step 4: Retry Rejected Interaction

Command:

```text
cmd_retry_key = Retry(source=cmd_interact_oob)
```

Expected:

- retry command accepted;
- original interact intent is re-admitted under current state;
- reach query now succeeds;
- interaction system executes exactly once;
- inventory gains `gold_key:1`;
- `gold_key.active=false`;
- objective `collect_gold_key=Complete`;
- runtime events include `CommandAccepted`, `Interacted`, `ItemAcquired`, and
  `ObjectiveCompleted`;
- command log preserves both the rejected original and accepted retry.

Retry must not bypass authority or command admission.

### Step 5: Enter Tactical Slow Time

Command:

```text
cmd_enter_tactical = ToggleTacticalMode()
```

Expected:

- command accepted;
- clock mode becomes `Slow`;
- camera mode becomes `TacticalOverhead`;
- previous realtime camera remains `ThirdPerson`;
- camera input-clear request is set;
- runtime event `ClockChanged`;
- runtime event `CameraChanged`.

### Step 6: Tactical Movement

Command:

```text
cmd_tactical_move = Move(actor=player, targetPoint=(2.000,0.000,1.000))
```

Expected:

- command accepted;
- movement system executes through the same mutation path as realtime movement;
- player position becomes `(2.000,0.000,1.000)`;
- clock remains `Slow`;
- camera remains `TacticalOverhead`.

### Step 7: Pause

Command:

```text
cmd_pause = Pause()
```

Expected:

- command accepted;
- clock mode becomes `Paused`;
- camera remains `TacticalOverhead`;
- automatic session advancement stops;
- runtime event `ClockChanged`.

### Step 8: Step While Paused

Command:

```text
cmd_step = StepTacticalTick()
```

Expected:

- command accepted;
- exactly one deterministic tick executes;
- clock remains `Paused` after the step;
- no extra automatic ticks execute;
- player position remains `(2.000,0.000,1.000)`;
- objective remains `Complete`;
- tick counter increases by exactly `1` relative to paused state.

### Step 9: Resume Slow Time

Command:

```text
cmd_resume = Resume()
```

Expected:

- command accepted;
- clock mode returns to `Slow`;
- camera remains `TacticalOverhead`;
- previous realtime camera remains `ThirdPerson`.

### Step 10: Wait No-Op

Command:

```text
cmd_wait = Wait()
```

Expected:

- command accepted;
- command log count increases;
- no gameplay state changes except deterministic tick/event/metric accounting;
- state hash changes only if command log/tick counters are included in the hash.

The implementation must document whether command-log-only changes affect
`StateHash`. Once chosen, replay and summary must match it.

### Step 11: Exit Tactical Slow Time

Command:

```text
cmd_exit_tactical = ToggleTacticalMode()
```

Expected:

- command accepted;
- clock mode becomes `Normal`;
- camera restores `ThirdPerson`;
- previous realtime camera remains `ThirdPerson`;
- camera input-clear request is set.

## Save Load Proof

After Step 11, create a save envelope from the completed gameplay session.

Required saved fields:

- package id `iggy3d.first_room`;
- scenario id `first_room.runtime_loop`;
- world state including player, inactive `gold_key`, marker;
- player roster;
- clock `Normal`;
- camera `ThirdPerson`;
- inventory `gold_key:1`;
- objective `Complete`;
- command log with ten gameplay command records;
- state hash metadata.

Load proof:

1. decode save envelope;
2. validate compatibility;
3. load into a fresh session;
4. recompute state hash;
5. compare loaded hash to saved hash;
6. compare loaded summary to source summary.

Failure behavior:

- incompatible save rejects before mutating destination session;
- malformed save reports structured diagnostic;
- loaded projection is regenerated, not loaded as save truth.

## Reset Proof

Reset must be proven on a separate branch session so it does not destroy the
completed source session needed for save/replay comparison.

Reset operation:

```text
Session::resetToBaseline()
```

Expected reset state:

- lifecycle `Playing`;
- clock `Normal`;
- camera `ThirdPerson`;
- player position `(0.000,0.000,0.000)`;
- `gold_key.active=true`;
- inventory empty;
- objective `collect_gold_key=Active`;
- command log empty or reset according to documented reset semantics;
- reset hash equals initial baseline hash.

The implementation must document whether reset clears command history or starts
a new epoch. Acceptance requires one choice and tests it explicitly.

## Replay Proof

Replay starts from the validated baseline fixture and the recorded command log.

Replay requirements:

1. create fresh baseline session;
2. replay `cmd_interact_oob`;
3. require rejection reason `OutOfRange`;
4. replay all accepted commands through normal authority/admission/session path;
5. compare final state hash to saved/completed source hash;
6. compare final summary to expected summary;
7. report first mismatch if replay diverges.

Replay must not:

- copy the final world state directly;
- skip rejected command records;
- bypass reach checks for retry;
- bypass command admission for accepted commands.

## Expected Final Runtime State

Final gameplay state before reset branch:

- lifecycle: `Complete` or `Playing` with objective `Complete`; the implementation
  must choose one and make `RuntimeSummary` explicit;
- outcome: `DemoComplete`;
- player slot 0 actor: `player`;
- player position: `(2.000,0.000,1.000)`;
- inventory for player slot 0: `gold_key:1`;
- `gold_key.active=false`;
- `tactical_marker_alpha.active=true`;
- objective `collect_gold_key=Complete`;
- clock mode: `Normal`;
- camera mode: `ThirdPerson`;
- previous realtime camera: `ThirdPerson`;
- first rejection reason: `OutOfRange`;
- command log contains all ten labeled gameplay commands;
- final state hash is stable across save/load/replay.

## Expected Runtime Events

The demo must emit enough structured runtime events to explain the run.

Required event kinds:

- `CommandRejected` for `cmd_interact_oob`;
- `CommandAccepted` for accepted gameplay commands;
- `Moved` for both move commands;
- `Interacted` for successful pickup;
- `ItemAcquired` for `gold_key`;
- `ObjectiveCompleted` for `collect_gold_key`;
- `ClockChanged` for tactical/pause/resume/normal transitions;
- `CameraChanged` for tactical/normal camera transitions;
- `SaveCreated`;
- `LoadCompleted`;
- `ResetCompleted`;
- `ReplayCompleted`.

Event text is not acceptance-sensitive. Event kind, command label/id, entity id,
and tick are acceptance-sensitive where present.

## Expected Summary Fields

`fixtures/demos/first_room/expected_summary.txt` must contain these fields in
this order:

```text
scenario=iggy3d.first_room:first_room.runtime_loop
outcome=DemoComplete
final_tick=<locked integer after implementation>
player.position=(2.000,0.000,1.000)
inventory.player0=gold_key:1
gold_key.active=false
objective.collect_gold_key=Complete
clock.mode=Normal
camera.mode=ThirdPerson
commands.submitted=10
commands.accepted=9
commands.rejected=1
commands.retry=1
first_rejection=OutOfRange
save.roundtrip=pass
reset.baseline=pass
replay.hash=pass
state_hash=<locked lowercase 16-hex value after first green implementation>
```

The final tick and hash placeholders are intentionally not guessed in planning.
The first correct implementation of `SessionRunner` and `StateHash` locks them.
After that, tests compare the expected summary byte-for-byte.

## Tool Responsibilities

### `iggy3d_validate_package`

Must prove:

- package loads;
- scenario loads;
- fixture ids match this document;
- entity count and objective count match;
- invalid package exits nonzero with structured diagnostics.

### `iggy3d_headless_demo`

Must:

- run the full scripted runtime flow;
- perform save/load/reset/replay proof;
- emit deterministic summary;
- exit `0` only when all pass;
- exit nonzero and print diagnostics on any mismatch.

### `iggy3d_replay_tool`

Must:

- rebuild baseline session from fixture;
- replay command log through normal runtime path;
- verify final hash;
- optionally compare expected summary;
- report first divergence.

## Acceptance Test Responsibilities

`tests/acceptance/complete_runtime_demo_tests.cpp` must assert:

- preflight package/scenario validation;
- initial state;
- target discovery result;
- first rejection reason and no-mutation behavior;
- move into reach;
- retry behavior and interaction effects;
- inventory and objective mutation;
- tactical camera and slow clock transition;
- pause/step/resume tick behavior;
- normal camera restoration;
- save/load hash equality;
- reset baseline equality;
- replay hash equality;
- expected summary match.

The acceptance test may call library APIs directly and may also execute the
headless demo binary if the test harness supports it. It must not require a
renderer.

## Pass Fail Rules

Pass requires every item below:

- package validator accepts the fixture;
- target query finds `gold_key` at start;
- first interact rejection is exactly `OutOfRange`;
- rejected interact does not change player, inventory, objective, or key active
  state;
- move puts player in reach;
- retry succeeds without bypassing command admission;
- pickup mutates inventory and objective exactly once;
- slow time changes camera to tactical;
- pause blocks automatic ticks;
- step advances exactly one tick;
- resume returns to slow time;
- tactical toggle restores normal realtime camera;
- save/load round-trip preserves state hash;
- reset branch restores baseline hash;
- replay reaches final hash;
- runtime summary matches expected file;
- no old `/Users/kogaryu/iggy` code is linked or included.

Any mismatch is a failing acceptance demo, not a warning.

## Open Implementation Choices To Lock During Build

These are allowed to be decided during implementation, but once chosen they must
be reflected in code, tests, and expected summary:

- whether objective completion changes lifecycle to `Complete` immediately or
  keeps lifecycle `Playing` with outcome `DemoComplete`;
- whether command-log-only changes affect `StateHash`;
- whether reset clears command history or starts a new command epoch;
- exact final tick count;
- exact final state hash.

No other acceptance semantics are optional.

## Completion Gate

This document is complete when:

- fixture files encode the state described here;
- command types support every scripted command;
- acceptance test asserts every pass/fail rule;
- headless demo emits the expected summary;
- package validator and replay tool pass on the fixture;
- save/load/reset/replay proof is deterministic;
- the runtime can prove the whole loop with no renderer and no old `iggy`
  dependency.
