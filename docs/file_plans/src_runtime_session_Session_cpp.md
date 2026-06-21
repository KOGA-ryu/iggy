# `src/runtime/session/Session.cpp`

Updated: 2026-06-20

Exact purpose: implement the high-level runtime session facade declared by
`Session.hpp`, coordinating session creation, command admission/logging,
session-control effects, tick/step hooks, reset, load replacement, and
read-only state access without owning local subsystem rules.

## Build Position

- priority rank: 65
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/session`
- file kind: `source`

This file is the runtime coordinator. It must be explicit about delegation:
`Session` owns orchestration and aggregate state replacement, while subsystem
files own local behavior.

## Ownership

This file owns implementation of:

- `Session` constructors;
- `Session::create`;
- state accessors;
- lifecycle/outcome/hash accessors;
- `submitCommand`;
- session-control command handling;
- tick/step/run-until-idle entry points or delegation;
- `resetToBaseline`;
- `replaceStateFromLoad`;
- private state invariant helpers.

It must not own:

- package/scenario file parsing;
- command admission rule implementation;
- command log storage internals;
- movement execution details;
- interaction effects;
- inventory operations;
- combat resolution;
- AI proposal rules;
- objective completion rules;
- save codec encoding/decoding;
- replay loop;
- projection generation;
- renderer/window/input code;
- network sockets;
- old `/Users/kogaryu/iggy` adapters.

## Required Include Order

Implementation must include paired header first:

```cpp
#include "runtime/session/Session.hpp"
```

Then include only required implementation dependencies:

```cpp
#include <utility>

#include "config/RuntimeConfig.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "runtime/camera/CameraModePolicy.hpp"
#include "runtime/clock/Clock.hpp"
#include "runtime/command/CommandAdmission.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/SessionTick.hpp"
```

The include set above is the first complete build include set. This source must
not include app, projection, renderer, test, socket, or old `iggy` headers.

## Internal Helper Layout

Prefer unnamed-namespace helpers in this order:

1. create identity from seed;
2. create world from seed;
3. create player roster from seed;
4. create subsystem states from seed;
5. build baseline snapshot;
6. validate created state invariants;
7. assign `commandId` for submitted commands;
8. build admission context;
9. lifecycle prefilter for command submission;
10. append admission result to command log;
11. handle immediate session-control effects;
12. recompute or dirty state hash;
13. clear transient state after reset/load.

Helpers must be deterministic and side-effect limited to the `SessionState`
object explicitly passed to them.

## Constructor Implementation

### Default Constructor

Required behavior:

- create `state_` in default `Loading` state;
- do not allocate fixture data;
- do not parse files;
- do not generate identifiers from global state;
- do not read wall-clock time.

### `Session(SessionState state)`

Required behavior:

- move or copy provided state into `state_`;
- trust only states produced by `Session::create` or load path in production;
- tests use this constructor for focused scenarios;
- debug builds validate invariants for states not produced by `Session::create`
  or `replaceStateFromLoad`.

## `Session::create` Implementation

Required algorithm:

1. validate request config and seed are structurally usable;
2. initialize empty `SessionState`;
3. set `identity.packageId`;
4. set `identity.scenarioId`;
5. set deterministic session seed/schema version;
6. create `WorldState` from seed entity order;
7. create `PlayerRoster` and bind player slot 0;
8. initialize `ClockState` to `Normal`;
9. initialize `CameraState` to default realtime camera;
10. initialize empty `CommandLog`;
11. initialize inventory/combat/AI/objective states from seed;
    - inventory initialization creates `PlayerInventory{playerSlot=0,
      stacks={}}` for the bound first-room player before commands execute;
12. set lifecycle `Playing`;
13. set outcome `None`;
14. create baseline snapshot from initialized authoritative state;
15. compute baseline hash if `StateHash` is available; otherwise set dirty flag;
16. compute and set current state hash;
17. clear transient events and metrics;
18. validate required invariants;
19. return `Result<Session>`.

First-room expected result:

- package id `iggy3d.first_room`;
- scenario id `first_room.runtime_loop`;
- three entities;
- player slot 0 bound to `player`;
- player position `(0.000,0.000,0.000)`;
- `gold_key.active=true`;
- objective `collect_gold_key=Active`;
- inventory contains player slot 0 with empty stacks;
- clock `Normal`;
- camera `ThirdPerson`;
- command log empty.

Failure behavior:

- invalid seed returns failed `Result`;
- missing player entity returns failed `Result`;
- missing slot binding returns failed `Result`;
- no partial session escapes as valid.

## State Accessors

### `state()`

Return const reference.

Rules:

- O(1);
- normal app/tool/test read path;
- caller cannot mutate runtime truth.

### Mutable Access

If `mutableStateForOwnedSystems()` exists:

- keep it private or narrowly exposed to runtime coordinator code;
- app/tools must not use it;
- document every call site in `Session.cpp` or `SessionTick.cpp`.

First complete build policy:

- no public mutable state access for apps/tools.

### `lifecycle()`, `outcome()`, `stateHash()`

Return current cached values from `state_`.

Rules:

- O(1);
- no recomputation unless explicitly documented;
- if hash is dirty, return cached hash and expose dirty state elsewhere or
  recompute through a deterministic helper.

## Admission Context Construction

`submitCommand` must build `CommandAdmissionContext` from `state_`.

Required context:

- `world = &state_.world`;
- `players = &state_.players`;
- `clock = &state_.clock`;
- `commandLog = &state_.commandLog`;
- `config = &runtime config` if session stores config or request-level config is
  available.

If `RuntimeConfig` is not stored in `SessionState`, `Session` must own config or
derive needed constants deterministically. Do not pull config from app globals.

Lifecycle prefilter:

- Session owns lifecycle gating before command admission.
- `Session` owns lifecycle gating before calling `CommandAdmission`.
- If `state_.lifecycle` is not playable, `submitCommand` rejects or routes the
  command through session-owned lifecycle/control handling before admission.
- `CommandAdmissionContext` carries `ClockState`, not `SessionLifecycle`.
- `CommandAdmission` is called only after the lifecycle prefilter permits the
  command to proceed to clock, roster, actor, target, reach, and retry checks.
- A lifecycle rejection uses the stable session-level reason/status
  `SessionNotPlaying` and does not call `admitCommand`.
- Lifecycle-prefiltered rejections still receive a stable nonzero
  `CommandRecord::commandId` before they are recorded.

Command identity ownership:

- `Session` command construction owns assigning a stable nonzero
  `CommandRecord::commandId` before lifecycle prefiltering and before
  `CommandAdmission`.
- `SessionState::nextCommandId` is authoritative allocation state.
- Fresh scenario creation and reset baseline initialize `nextCommandId = 1`.
- `submitCommand` prepares `candidateCommandId = state_.nextCommandId`, assigns it
  to the command record, and commits `state_.nextCommandId =
  candidateCommandId + 1` only after `CommandLog::append` succeeds.
- Lifecycle/admission rejected records advance `nextCommandId` after successful
  rejected append because they are recorded commands.
- If append fails because the command record is malformed, duplicate, or otherwise
  unrecordable, `nextCommandId` is not advanced.
- `CommandLog::append` owns assigning deterministic `CommandRecord::sequence`
  when sequence is invalid/unset.
- `CommandAdmission` never assigns `commandId` and never assigns sequence.
- Accepted and rejected commands are both appended with stable `commandId`
  values.
- Replay order is sequence-driven; retry source lookup is `commandId`-driven.
- Accepted gameplay commands are queued for execution by sequence exactly once;
  command log history is never scanned as an execution queue.

## `submitCommand` Implementation

Required algorithm:

1. copy input command into local record;
2. prepare `candidateCommandId = state_.nextCommandId` and assign it as stable
   nonzero `commandId`;
3. run lifecycle prefilter;
4. if lifecycle prefilter rejects, append the session-level rejected command
   record, mark summary/hash dirty, and return without calling admission;
5. build admission context;
6. call `admitCommand`;
7. append returned command record to `state_.commandLog`;
8. if append returns non-`Ok`, return `SessionCommandResult` with
   `appendStatus` set to the exact `CommandLogAppendStatus` and leave
   `nextCommandId`, command log, execution queue, and gameplay state unchanged;
9. commit `state_.nextCommandId = candidateCommandId + 1`;
10. emit command accepted/rejected event;
11. if rejected, mark summary/hash dirty and return result without execution;
12. if accepted and kind is immediate session-control command, handle control
   effect;
13. if accepted and kind is gameplay command, append the stored sequence returned
   by `CommandLog::append` to
   `state_.transient.pendingExecutionSequences` exactly once and leave execution
   for `SessionTick`;
14. mark summary/hash dirty;
15. return `SessionCommandResult`.

Rules:

- rejected commands are always logged;
- accepted commands are logged before execution;
- accepted gameplay commands are queued by stored command-log sequence exactly
  once after append succeeds;
- accepted `CommandKind::Retry` is queued by its retry command sequence; the
  effective original intent is reconstructed later by `SessionTick`;
- immediate/control commands are not queued for gameplay execution;
- no command executes if not accepted;
- lifecycle-gated commands are rejected before admission;
- admission remains read-only;
- admission may read `commandId` for diagnostics/source linkage but must not
  mutate identity;
- app code never appends directly to command log;
- command log append is deterministic.

## Replay/Internal Submission

The first complete public `Session` API does not expose `submitAdmittedCommand`.
Replay/load/internal tests restore command history through `CommandLog::restoreForLoad`
or resubmit commands through `submitCommand` so lifecycle, admission, logging,
command-id allocation, and execution queue behavior remain observable in one
path.

## Immediate Session-Control Handling

Session-control commands may take effect during submission if accepted.

Immediate control kinds:

- `ToggleTacticalMode`;
- `Pause`;
- `Resume`;
- `StepTacticalTick`.

First complete build behavior:

- `Pause` and `Resume` mutate clock immediately;
- `ToggleTacticalMode` mutates clock/camera immediately through `Clock` and
  `CameraModePolicy`;
- `StepTacticalTick` calls `stepOneTick` and returns after exactly one tick;
- reset proof calls `resetToBaseline` through the proof/API path, not a gameplay
  command record;
- save/load proof calls `SaveLoad` and `replaceStateFromLoad` through the
  proof/API path, not gameplay command records.

Session must delegate:

- clock transitions to `Clock`;
- camera transitions to `CameraModePolicy`;
- reset to `resetToBaseline`;
- tick execution to `SessionTick` or `SessionRunner`.

## Gameplay Command Execution

This file must not implement movement, interaction, combat, AI, or objective
rules.

Accepted gameplay commands are executed by:

- command submission logs/admission;
- `SessionTick` owns execution order.

Acceptance implications:

- `cmd_move_to_key` must mutate player position only when tick/execution path
  runs;
- `cmd_interact_oob` never executes;
- `cmd_retry_key` executes interaction only after admission succeeds.

## `tick()` Implementation

Return `StatusResult`. `ResultStatus::Ok` means the tick completed and any
resulting state mutations/events are visible in `state_`. Non-`Ok` means the
tick failed before partial tick mutation.

Required behavior:

- verify lifecycle/clock allows tick;
- call `SessionTick` for exactly one deterministic tick;
- provide only `state_.transient.pendingExecutionSequences` records as execution
  input and never scan command log history as a fallback queue;
- let `SessionTick` dispatch systems in fixed order;
- apply outcome/lifecycle updates returned by tick;
- update transient metrics/events;
- recompute or mark dirty state hash/summary;
- return structured status.

Must not:

- read wall-clock time;
- parse files;
- call app code;
- render frames.

If clock is paused, `tick()` returns a paused/no-work status without advancing.
Only `stepOneTick` can force one paused tick.

## `stepOneTick()` Implementation

Required behavior:

1. verify clock/session is paused;
2. execute exactly one deterministic tick;
3. leave clock paused afterward;
4. update metrics/hash/summary;
5. return structured status.

Failure:

- if not paused, return failed status or route through admission rejection
  `StepRequiresPaused` before this function is called.

No extra automatic ticks may run.

## `runUntilIdle(maxTicks)` Implementation

Required behavior:

- loop while work remains and lifecycle allows;
- call `tick()` each iteration;
- stop when idle, complete, failed, or max ticks reached;
- return failed status if max ticks is exceeded before expected stop.

This is the shared headless demo/test loop. `SessionRunner` delegates to this
function and preserves the same stop statuses.

## `resetToBaseline()` Implementation

Required algorithm:

1. copy baseline world into active world;
2. copy baseline players into active roster;
3. copy baseline clock into active clock;
4. copy baseline camera into active camera;
5. copy baseline inventory/combat/AI/objective states;
6. set lifecycle `Playing`;
7. set outcome `None`;
8. reset command log with `CommandLogResetPolicy::Clear`;
9. set `state_.nextCommandId = 1`;
10. clear `state_.transient.pendingExecutionSequences`;
11. clear transient events;
12. reset metrics according to metrics policy;
13. recompute current hash;
14. return `SessionResetResult`.

Rules:

- reset does not parse fixture files;
- reset does not call app code;
- reset does not mutate any other session branch;
- reset does not preserve projection as truth.

Acceptance reset branch:

- player returns to `(0.000,0.000,0.000)`;
- `gold_key.active=true`;
- inventory empty;
- objective active/incomplete;
- clock `Normal`;
- camera `ThirdPerson`;
- command log empty under clear policy;
- `nextCommandId == 1`;
- `pendingExecutionSequences` empty;
- current hash equals baseline hash.

## `replaceStateFromLoad` Implementation

Required all-or-nothing algorithm:

1. keep a copy or move-safe backup of current state;
2. validate loaded state invariants;
3. recompute loaded hash;
4. clear/regenerate transient data on candidate;
5. if validation/hash fails, leave `state_` unchanged and return failure;
6. if valid, replace `state_` with candidate;
7. return `SessionLoadResult`.

Required status mapping:

- valid replacement returns `SessionLoadStatus::Ok`;
- structurally invalid loaded state returns `InvalidCandidateState`;
- zero or stale `nextCommandId` returns `InvalidCommandIdCursor`;
- invalid command-log records, `nextSequence`, or `epoch` returns
  `InvalidCommandLogState`;
- invalid lifecycle/outcome pair returns `InvalidLifecycleState`;
- failure in the final replacement boundary after validation returns
  `ReplacementRejected`.

Every non-`Ok` result returns `previousHash`, leaves active `state_` unchanged,
sets `loadedHash` to `0`, and fills `diagnostic` with the failed invariant.

Must not:

- decode save bytes;
- check file paths;
- decide schema compatibility;
- partially apply loaded world before discovering invalid subsystem state.

Those responsibilities belong to `SaveCodec`, `SaveCompatibility`, and
`SaveLoad`.

## Hash And Summary Dirtying

Session must ensure state hash/summary are current or explicitly dirty after:

- session creation;
- accepted command execution;
- rejected command log append if command log participates in hash/summary;
- reset;
- load replacement;
- tick;
- objective outcome change.

First complete build behavior:

- recompute hash at deterministic acceptance checkpoints;
- mark dirty after intermediate changes if full hash is not yet available.

Never hash cached `currentStateHash` into itself.

## Runtime Events And Metrics

`Session.cpp` appends runtime events/metrics for:

- command accepted;
- command rejected;
- clock changed;
- camera changed;
- reset completed;
- load completed;
- step completed.

Event emission must not decide gameplay outcome. Events explain state changes
after the owning state change has happened.

## Save Handoff Implementation

Session does not write files.

For `Save` command:

- first complete build does not model save proof as a gameplay command record;
- `SaveLoad` creates the envelope through proof/tool APIs;
- command counts are unchanged by save proof APIs.

For `Load` command:

- first complete build does not model load proof as a gameplay command record;
- actual loaded state arrives through `replaceStateFromLoad`;
- source path and codec work stay outside session.

## Replay Handoff Implementation

Replay interacts with `Session` through normal APIs.

This source must not add shortcuts that:

- force accepted commands into execution without admission;
- mutate world directly from replay;
- skip rejected command records;
- fake command log sequence.

Replay-specific hooks are allowed only if they preserve the same admission,
logging, and execution semantics.

## Multiplayer Handoff Implementation

Local multiplayer coordinator calls `submitCommand` in merged deterministic
order.

Session must:

- preserve player slot values;
- pass roster into admission;
- log command source/slot;
- not assume only player slot 0 except during fixture creation tests.

Session must not own sockets, packet decode, or transport.

## Diagnostics And Errors

Use structured result/status values for:

- invalid create request;
- seed missing required player;
- seed missing objective;
- command admission rejected;
- command log append invariant failure if recoverable;
- tick while not allowed;
- step while not paused;
- reset unavailable;
- invalid loaded state;
- max ticks exceeded.

Do not make tests depend on prose-only log strings.

## Compute Cost

Creation:

- O(seed entities plus seed subsystem records).

Submit:

- O(command admission cost plus command log append).

Tick:

- O(SessionTick cost).

Step:

- O(one SessionTick cost).

Run until idle:

- O(tick count times SessionTick cost).

Reset:

- O(size of baseline state plus command log reset).

Load replacement:

- O(size of loaded state plus hash validation).

Accessors:

- O(1).

## Tests And Verification

Covered by:

- `tests/unit/session_state_tests.cpp`;
- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/clock_tests.cpp`;
- `tests/unit/camera_mode_policy_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/unit/save_load_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- `create` builds exact first-room initial state;
- creation initializes `nextCommandId == 1`;
- invalid create request fails without partial valid session;
- `submitCommand` logs rejected commands;
- lifecycle-prefiltered rejections are logged with stable nonzero `commandId`
  values before returning;
- rejected commands do not execute;
- accepted commands append before execution;
- accepted gameplay commands enqueue exactly once after append succeeds;
- rejected commands and immediate/control commands do not enqueue;
- failed command-log append does not advance `nextCommandId`;
- `cmd_interact_oob` logs `OutOfRange`;
- retry can find rejected source through command log;
- pause/resume/tactical commands update clock/camera through policies;
- step while paused executes exactly one tick;
- reset restores baseline and clears command log under clear policy;
- load replacement is all-or-nothing;
- save/load paths do not write/read files in session;
- no renderer/app/raw input state is reachable.

## Completion Criteria

- `src/runtime/session/Session.cpp` exists in `/Users/kogaryu/iggy3d`.
- It includes `Session.hpp` first.
- It implements every public API declared by the header.
- It creates first-room session state deterministically.
- It submits commands through admission and command log.
- It never executes rejected commands.
- It delegates subsystem rules to owning systems.
- It resets from baseline deterministically.
- It applies loaded state all-or-nothing.
- It has no app, projection, renderer, socket, test, or old `iggy`
  dependency.
