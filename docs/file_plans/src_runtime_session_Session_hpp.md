# `src/runtime/session/Session.hpp`

Updated: 2026-06-20

Exact purpose: declare the high-level runtime session facade for creation,
command submission, command admission/log coordination, clock/camera control,
tick entry points, save/load handoff, reset, and read-only state access.

## Build Position

- priority rank: 64
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/session`
- file kind: `header`

This header defines how app/tools/tests interact with runtime. It is the public
door into gameplay state, but it does not own the local rules of movement,
interaction, targeting, inventory, combat, AI, objective, save codec, or replay.

## Ownership

This file owns:

- public `Session` class declaration;
- session creation request shape and `Result<Session>` factory contract;
- command submission request/result shape;
- reset request/result shape;
- load application request/result shape through `replaceStateFromLoad`;
- state access boundary;
- command admission/log coordination API;
- tick/step entry point declarations;
- lifecycle/outcome read helpers;
- state replacement boundary for all-or-nothing load.

It must not own:

- package file parsing;
- scenario file parsing;
- raw input conversion;
- renderer/window behavior;
- projection generation;
- command admission implementation;
- command execution implementation;
- save codec implementation;
- replay loop implementation;
- movement/interaction/combat/AI/objective algorithms;
- network sockets or packet transport;
- old `/Users/kogaryu/iggy` adapters.

## Required Header Shape

The implementation file must be:

```text
src/runtime/session/Session.hpp
```

Required include style:

```cpp
#pragma once

#include <string>

#include "core/result/Result.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/command/CommandAdmission.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/SessionState.hpp"
```

Forward declarations:

```cpp
namespace iggy3d {
struct RuntimeConfig;
struct FixtureScenarioSeed;
struct SaveEnvelope;
struct RuntimeEvent;
}
```

Use forward declarations for non-owned runtime values. Do not include app, projection,
renderer, test, socket, or old `iggy` headers.

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Public Types

### `SessionCreateRequest`

Declare the exact request:

```cpp
struct SessionCreateRequest {
  RuntimeConfig config;
  FixtureScenarioSeed seed;
};
```

The first complete build uses the content-owned validated seed type named
`FixtureScenarioSeed`. The request must not contain file paths as runtime truth.

Semantics:

- seed has already been loaded and validated by content;
- runtime copies seed facts into owned `SessionState`;
- content does not own state after creation;
- config is copied into session or used to initialize deterministic defaults.

### `SessionCommandResult`

Declare:

```cpp
struct SessionCommandResult {
  CommandRecord command;
  CommandAdmissionResult admission;
  CommandLogAppendStatus appendStatus = CommandLogAppendStatus::Ok;
  bool appendedToLog = false;
  bool executedImmediately = false;
};
```

Semantics:

- `command` is the final stored/admitted command record;
- `admission` records accepted/rejected reason;
- `appendStatus` mirrors `CommandLog::append`; only `Ok` records can enqueue or
  execute;
- `appendedToLog` proves command log storage happened;
- `executedImmediately` is true only for session-control commands whose effect
  occurs during submission.

Gameplay commands are admitted/logged first, then executed through the session
tick/runner path.

### `SessionResetResult`

Declare:

```cpp
struct SessionResetResult {
  bool reset = false;
  std::uint64_t baselineHash = 0;
  std::uint64_t currentHash = 0;
};
```

Semantics:

- `reset` true means baseline was restored;
- baseline hash and current hash must match for acceptance reset proof;
- reset uses `CommandLogResetPolicy::Clear`.

### `SessionLoadStatus`

Declare:

```cpp
enum class SessionLoadStatus : std::uint8_t {
  Ok,
  InvalidCandidateState,
  InvalidCommandIdCursor,
  InvalidCommandLogState,
  InvalidLifecycleState,
  ReplacementRejected,
};
```

### `SessionLoadResult`

Declare:

```cpp
struct SessionLoadResult {
  SessionLoadStatus status = SessionLoadStatus::Ok;
  StateHashValue previousHash = 0;
  StateHashValue loadedHash = 0;
  std::string diagnostic;
};
```

Semantics:

- `Ok` is the only success status;
- `previousHash` is always the pre-call hash;
- `loadedHash` is valid only when `status == Ok`;
- non-`Ok` results leave existing `SessionState` unchanged;
- load is all-or-nothing;
- save compatibility and envelope decoding are not owned by this header.

## Required `Session` Class API

Declare the exact public class API:

```cpp
class Session {
public:
  Session();
  explicit Session(SessionState state);

  static Result<Session> create(const SessionCreateRequest& request);

  const SessionState& state() const;
  SessionState& mutableStateForOwnedSystems();

  SessionLifecycle lifecycle() const;
  SessionOutcome outcome() const;
  std::uint64_t stateHash() const;

  SessionCommandResult submitCommand(const CommandRecord& command);

  StatusResult tick();
  StatusResult stepOneTick();
  StatusResult runUntilIdle(std::uint32_t maxTicks);

  SessionResetResult resetToBaseline();
  SessionLoadResult replaceStateFromLoad(SessionState loadedState);

private:
  SessionState state_;
};
```

The public API names shown above are the required first complete build names.
The implementation header must not introduce alternate public names for these
session entry points.

Important boundary:

- `state()` is the normal read-only access path.
- `mutableStateForOwnedSystems()` is internal coordinator access. If present, it
  must not be exposed to apps/tools.

First complete build policy: keep mutable access private to `Session.cpp` and
friend no app/tool code.

## Creation Semantics

`Session::create` must:

1. validate request has a valid seed and deterministic config;
2. create `SessionState`;
3. copy package id and scenario id into identity;
4. allocate world entities in seed order;
5. create player roster and bind slot 0 to player;
6. set clock to `Normal`;
7. set camera to default realtime mode, `ThirdPerson` for first-room demo;
8. initialize inventory, combat, AI, and objectives;
9. initialize empty command log;
10. build baseline snapshot;
11. initialize `nextCommandId = 1`;
12. compute baseline and current state hash;
13. set lifecycle `Playing`;
14. set outcome `None`;
15. clear transient events/metrics.

For first-room acceptance fixture, created state must contain:

- package id `iggy3d.first_room`;
- scenario id `first_room.runtime_loop`;
- three entities;
- local player slot 0 bound to player;
- empty inventory;
- objective `collect_gold_key=Active`;
- clock `Normal`;
- camera `ThirdPerson`;
- empty command log.
- `nextCommandId == 1`.

## Command Submission Semantics

`submitCommand` must coordinate:

1. copy/construct the pending command record;
2. prepare `candidateCommandId = state_.nextCommandId` and assign it to
   `CommandRecord::commandId`;
3. check `SessionLifecycle`;
4. reject commands when lifecycle is terminal before
   `CommandAdmission` is called;
5. construct admission context from current state;
6. call `CommandAdmission`;
7. append accepted or rejected command record to `CommandLog`;
8. let `CommandLog::append` assign deterministic sequence if unset;
9. branch on `CommandLogAppendResult::status`; non-`Ok` returns
   `SessionCommandResult` with the exact `appendStatus` and leaves
   `nextCommandId`, command log, execution queue, and gameplay state unchanged;
10. after successful append, commit `state_.nextCommandId = candidateCommandId + 1`;
11. if the successfully appended command is accepted and executes on tick,
   append the stored `CommandRecord::sequence` exactly once to
   `state_.transient.pendingExecutionSequences`;
11. emit command accepted/rejected runtime event;
12. apply immediate session-control effects only if the command kind is defined
   as immediate;
13. leave gameplay effects for tick/system execution.

Rules:

- Session owns lifecycle gating before command admission.
- `Session` owns lifecycle gating; `CommandAdmissionContext` carries
  `ClockState`, not `SessionLifecycle`;
- lifecycle `Complete` and `Failed` are terminal for gameplay `submitCommand`
  calls and return/log a session-level rejection such as `SessionNotPlaying`
  before admission;
- the first build does not use `SessionLifecycle::Paused`; pause, resume, and
  step read/write `ClockState`;
- `Session` command construction owns `commandId` assignment before lifecycle
  prefiltering and before `CommandAdmission`;
- `SessionState::nextCommandId` is authoritative session state for command-id
  allocation;
- fresh scenario creation and reset baseline initialize `nextCommandId = 1`;
- `submitCommand` commits the next-command-id cursor only after
  `CommandLog::append` succeeds;
- lifecycle/admission rejections advance the cursor after successful rejected
  append because they are recorded command records;
- unrecordable malformed/internal append failures do not advance the cursor;
- accepted, rejected, and lifecycle-prefiltered rejected commands all carry
  stable nonzero `commandId` values before command-log append;
- `CommandAdmission` never assigns `commandId` or sequence;
- replay order is sequence-driven and retry source lookup is
  `commandId`-driven;
- rejected commands are logged and never executed;
- accepted commands are logged before execution;
- accepted gameplay commands that execute on tick enter
  `pendingExecutionSequences` exactly once after append succeeds;
- immediate/control commands and rejected commands never enter
  `pendingExecutionSequences`;
- admission remains read-only;
- command log append is deterministic;
- retry is submitted as a command and references source `commandId`;
- app code must not append directly to command log;
- app code must not dispatch systems directly.

Immediate session-control effects include:

- `Pause`;
- `Resume`;
- `ToggleTacticalMode`;
- `StepTacticalTick` scheduling;

Save, load, reset, and replay proof phases use dedicated proof APIs in the
first complete build, not gameplay `submitCommand` records.

`Session.cpp` documents which control commands execute during submission versus
tick. Acceptance asserts final state and command log order.

## Tick And Step Semantics

`tick()` returns `StatusResult`. `ResultStatus::Ok` means the operation completed
and all resulting state mutations/events are visible in session state. Non-`Ok`
means the operation failed before partial tick mutation.

`tick()` must:

- execute one deterministic tick when clock/lifecycle allow it;
- dispatch accepted unexecuted gameplay commands through `SessionTick`;
- update objective/outcome;
- update metrics/events;
- update or dirty state hash/summary;
- never read wall-clock time.

`stepOneTick()` must:

- require paused clock/session or route through `StepTacticalTick` command;
- execute exactly one deterministic tick;
- leave clock paused afterward.

`runUntilIdle(maxTicks)` must:

- call tick deterministically until no work remains, lifecycle completes, or
  max ticks is reached;
- return failure if max ticks is exceeded before expected idle/completion.

`SessionRunner` owns multi-tick orchestration, while this header exposes the
lower-level hooks needed by `SessionRunner` and the documented acceptance demo
execution path.

## Reset Semantics

`resetToBaseline()` must:

1. restore baseline world;
2. restore baseline player roster;
3. restore baseline clock;
4. restore baseline camera;
5. restore baseline inventory/combat/AI/objectives;
6. set lifecycle `Playing`;
7. set outcome `None`;
8. reset command log with `CommandLogResetPolicy::Clear`;
9. set `SessionState::nextCommandId = 1`;
10. clear `state_.transient.pendingExecutionSequences`;
11. clear transient events/metrics;
12. recompute current hash;
13. return baseline/current hash facts.

Reset must not:

- parse fixture files;
- ask app code to manually restore fields;
- preserve projection as truth;
- preserve renderer state;
- mutate another session branch unless called on that branch.

Acceptance reset proof uses a separate branch session and expects baseline hash
to match current hash after reset, `nextCommandId == 1`, command log clear under
`CommandLogResetPolicy::Clear`, and no pending gameplay command sequences.

## Load Replacement Semantics

`replaceStateFromLoad(SessionState loadedState)` must be all-or-nothing and
return `SessionLoadResult`.

Required behavior:

1. capture `previousHash` before validation;
2. validate loaded state is structurally valid enough to become active;
3. validate `loadedState.nextCommandId` is nonzero and greater than every loaded
   command record id;
4. validate command log records, `nextSequence`, and `epoch`;
5. validate lifecycle/outcome pair;
6. clear/regenerate transient data on a candidate;
7. recompute loaded hash;
8. if any validation fails, leave `state_` unchanged and return the exact
   non-`Ok` `SessionLoadStatus`;
9. if final replacement fails after validation, leave `state_` unchanged and
   return `SessionLoadStatus::ReplacementRejected`;
10. replace `state_` once and return `SessionLoadStatus::Ok` with
    `previousHash` and `loadedHash`.

Status mapping:

- valid replacement returns `SessionLoadStatus::Ok`;
- malformed candidate sections return `InvalidCandidateState`;
- zero or stale `nextCommandId` returns `InvalidCommandIdCursor`;
- invalid command log records/cursors return `InvalidCommandLogState`;
- invalid lifecycle/outcome pair returns `InvalidLifecycleState`;
- failure in the final replacement boundary after validation returns
  `ReplacementRejected`.

This function does not decode bytes and does not decide compatibility. That
belongs to `SaveCodec` and `SaveCompatibility`.

## Save Handoff Semantics

Session does not write files.

Session exposes enough state for `SaveLoad` to create an envelope:

- read-only `state()`;
- current hash;
- command log;
- identity;
- lifecycle/outcome.

First complete build save/load proof uses `SaveLoad` APIs directly, not
gameplay command records. Save/load proof APIs do not alter command counts or
the saved ten-record gameplay command log.

## Replay Semantics

Replay must use normal session APIs:

- create fresh session from baseline seed;
- submit command records through `submitCommand` or replay-specific API that
  still uses admission and log;
- execute accepted commands through normal tick/system path;
- compare final hash.

Session must not expose an API that lets replay force state mutations around
admission.

## Multiplayer Semantics

Session must preserve multiplayer shape:

- command submission accepts player slot values;
- command admission context includes player roster;
- command log stores player slot/source;
- local multiplayer coordinator can call session with merged command order;
- remote decoded commands later enter through same submission API.

Session must not own:

- sockets;
- remote transport;
- matchmaking;
- packet serialization.

## Diagnostics And Errors

Session operations must return structured results.

Expected diagnostic/status cases:

- invalid create request;
- missing player slot 0 after creation;
- command rejected with exact reason;
- reset unavailable;
- load replacement invalid;
- max ticks exceeded;
- state hash mismatch after load/replay if checked here.

Human-readable text may exist, but stable enum/status values must drive tests.

## Compute Cost

Creation:

- O(seed entity count plus subsystem seed sizes).

Command submission:

- O(admission cost plus command log append).

Tick:

- O(commands this tick plus system scans).

Reset:

- O(size of baseline snapshot).

Load replacement:

- O(size of loaded state).

Read-only accessors:

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

- create first-room session has expected identity/world/player/clock/camera
  state;
- submit rejected `Interact` logs rejection and does not execute interaction;
- lifecycle-prefiltered rejection receives stable nonzero `commandId` and is
  logged without calling `CommandAdmission`;
- submit accepted `Move` logs command before execution;
- accepted movement, interaction, and retry commands enqueue their stored
  command-log sequence exactly once;
- rejected commands, including `cmd_interact_oob`, never enqueue;
- retry command can reference rejected `commandId` through log;
- pause/resume/tactical control commands update clock/camera through owned
  policies;
- step executes exactly one tick when paused;
- reset restores baseline;
- load replacement is all-or-nothing;
- session exposes no renderer/app/raw input state;
- command submission path does not bypass admission.

## Completion Criteria

- `src/runtime/session/Session.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares creation, command submission, tick/step, reset, load replacement,
  and state access APIs.
- It keeps app/tool interaction behind the session facade.
- It coordinates command admission and command log ownership without owning
  local system rules.
- It supports acceptance demo command flow.
- It supports save/load/replay handoff without file IO.
- It preserves future multiplayer command submission shape.
- It has no app, projection, renderer, socket, test, or old `iggy` dependency.
