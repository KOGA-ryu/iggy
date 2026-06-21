# `src/runtime/session/SessionState.hpp`

Updated: 2026-06-20

Exact purpose: declare the aggregate authoritative runtime truth for an active
`iggy3d` session, including lifecycle, world, players, clock, camera, command
history, subsystem state, baseline reset data, and identity metadata.

## Build Position

- priority rank: 63
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/session`
- file kind: `header`

This file is the structural center of runtime. It does not implement gameplay
rules, but it declares the state shape that movement, interaction, save/load,
replay, diagnostics, projection, and acceptance all read.

## Ownership

This file owns the aggregate state type, not the local rules of every subsystem.

This file owns:

- session lifecycle enum;
- session outcome enum;
- session identity fields;
- aggregate `SessionState` struct;
- baseline reset snapshot type;
- transient session flags that are not owned by a narrower subsystem;
- explicit distinction between save truth and derived/transient buffers.

This file contains by value or owned aggregate:

- `WorldState`;
- `PlayerRoster`;
- `ClockState`;
- `CameraState`;
- `CommandLog`;
- `InventoryState`;
- `CombatState`;
- `AiState`;
- `ObjectiveState`;
- baseline reset state;
- runtime events and metrics if the implementation stores them at session scope.

It must not own:

- package file parsing;
- fixture file IO;
- CLI paths;
- raw keyboard, mouse, controller, or touch input;
- renderer handles;
- GPU resources;
- projection buffers as authoritative state;
- old `/Users/kogaryu/iggy` state or adapters;
- command admission rules;
- local movement, interaction, combat, AI, inventory, or objective algorithms.

## Required Header Shape

The implementation file must be:

```text
src/runtime/session/SessionState.hpp
```

Required include style:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/ai/AiState.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/diagnostics/RuntimeEvent.hpp"
#include "runtime/diagnostics/RuntimeMetrics.hpp"
#include "runtime/inventory/InventoryState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/world/WorldState.hpp"
```

If include cycles appear, use forward declarations only where value ownership is
not required. Since `SessionState` owns these aggregates by value, headers for
owned value types will usually be required.

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Enums

### `SessionLifecycle`

Declare:

```cpp
enum class SessionLifecycle : std::uint8_t {
  Loading,
  Playing,
  Paused,
  Complete,
  Failed,
};
```

Semantics:

- `Loading`: content has not yet created a playable state.
- `Playing`: normal or slow-time runtime can advance.
- `Paused`: automatic advancement is blocked; step may advance exactly one tick.
- `Complete`: acceptance/demo objective reached completed outcome.
- `Failed`: unrecoverable runtime/session error for the current state.

First complete build pause ownership is locked: `ClockState` owns paused mode
and lifecycle remains `Playing` during tactical pause/step/resume. The
`Paused` lifecycle value is reserved for a later lifecycle-level pause feature
and is not used by the first acceptance flow.

### `SessionOutcome`

Declare:

```cpp
enum class SessionOutcome : std::uint8_t {
  None,
  DemoComplete,
  Victory,
  Defeat,
  Failed,
};
```

Semantics:

- `None`: objective outcome not reached.
- `DemoComplete`: first-room acceptance demo completed.
- `Victory`: future game victory state.
- `Defeat`: future game loss state.
- `Failed`: unrecoverable runtime error.

The acceptance demo completion rule is locked: completing
`collect_gold_key=Complete` sets the objective status and outcome
`DemoComplete`, but lifecycle remains `Playing` so the scripted tactical,
pause, step, resume, wait, and camera-exit commands can still run. After the
final scripted gameplay command has executed and the session is idle, the
headless demo/session finalization path sets lifecycle `Complete`.
`RuntimeSummary` and acceptance tests read final lifecycle `Complete` and
outcome `DemoComplete`.

### Reserved `SessionResetPolicy`

Session reset policy is not saved as a session-state enum in the first complete
build. `CommandLogResetPolicy::Clear` owns the command-log reset behavior.
If a future state-level reset policy is exposed, it must not change first-build
acceptance semantics:

```cpp
enum class SessionResetPolicy : std::uint8_t {
  ClearCommandLog,
  NewCommandEpoch,
};
```

The first complete build uses clear reset behavior only.

## Required Value Types

### `SessionIdentity`

Declare this value type:

```cpp
struct SessionIdentity {
  std::string packageId;
  std::string scenarioId;
  std::uint64_t sessionSeed = 0;
  std::uint32_t schemaVersion = 1;
};
```

Semantics:

- `packageId` must be `iggy3d.first_room` for the first acceptance fixture.
- `scenarioId` must be `first_room.runtime_loop` for the first acceptance
  fixture.
- `sessionSeed` is deterministic and must not be used for unseeded randomness.
- `schemaVersion` participates in save compatibility if not centralized
  elsewhere.

### `BaselineSnapshot`

Declare this value type:

```cpp
struct BaselineSnapshot {
  SessionIdentity identity;
  WorldState world;
  PlayerRoster players;
  ClockState clock;
  CameraState camera;
  InventoryState inventory;
  CombatState combat;
  AiState ai;
  ObjectiveState objectives;
  std::uint64_t baselineHash = 0;
};
```

Semantics:

- this is the reset source of truth;
- it is created after package/scenario validation and session seed creation;
- it must not include projection buffers;
- it must not include app paths;
- it must not include renderer resources;
- it excludes command log because reset clears command history through
  `CommandLogResetPolicy::Clear`;
- command-log epoch is owned by `CommandLog`, not by baseline;
- `nextCommandId` is reset with baseline because command-id allocation is session
  state, not command-log state.

### `SessionTransientState`

Declare this value type:

```cpp
struct SessionTransientState {
  std::vector<RuntimeEvent> events;
  RuntimeMetrics metrics;
  std::vector<CommandSequence> pendingExecutionSequences;
  bool cameraInputClearRequested = false;
  bool summaryDirty = true;
  bool stateHashDirty = true;
};
```

Semantics:

- transient state explains or optimizes runtime operation;
- it is not authoritative gameplay truth;
- `pendingExecutionSequences` names accepted gameplay command records that
  still need one tick execution; it is an execution queue, not command history;
- save/load must regenerate or clear transient data;
- reset clears events and resets session metrics.

If the implementation stores events/metrics elsewhere, those stores retain the
same transient ownership and exclusion rules.

## Required `SessionState` Fields

Declare `SessionState` with these fields:

```cpp
struct SessionState {
  SessionIdentity identity;
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;

  WorldState world;
  PlayerRoster players;
  ClockState clock;
  CameraState camera;
  CommandLog commandLog;
  CommandId nextCommandId = 1;

  InventoryState inventory;
  CombatState combat;
  AiState ai;
  ObjectiveState objectives;

  BaselineSnapshot baseline;
  SessionTransientState transient;

  std::uint64_t currentStateHash = 0;
};
```

Each concept above must exist with these ownership boundaries. The field names
shown in the `SessionState` shape are exact current-build state contract names
for save, load, replay, state hash, and session APIs.

`nextCommandId` is authoritative session state. It is the next stable nonzero
`CommandRecord::commandId` that `Session::submitCommand` will assign. It starts
at `1` for fresh scenarios and reset baselines, advances only after successful
command-log append, and must always be greater than every
`CommandRecord::commandId` stored in `commandLog.records()`.

## Field Ownership And Mutation

### `identity`

Owned by session.

Mutated:

- during session creation;
- during load transaction;
- during reset only by restoring baseline identity.

Included in save: yes.

Included in hash: yes.

### `lifecycle`

Owned by session.

Mutated:

- session creation sets `Playing`;
- unrecoverable failure sets `Failed`;
- objective completion sets outcome `DemoComplete` and leaves lifecycle
  `Playing` until scripted gameplay finalization sets `Complete`;
- load transaction restores saved value if compatible.

Included in save: yes.

Included in hash: yes.

### `outcome`

Owned by session/objective boundary.

Mutated:

- objective system suggests outcome;
- session applies outcome transition;
- reset restores `None`;
- load restores saved value.

Included in save: yes.

Included in hash: yes.

### `world`

Owned locally by `WorldState`.

Mutated through:

- `WorldState` APIs;
- system calls routed through `WorldState`;
- reset/load aggregate replacement through `Session`.

Included in save: yes.

Included in hash: yes.

### `players`

Owned locally by `PlayerRoster`.

Mutated through:

- roster creation;
- actor binding APIs;
- reset/load aggregate replacement.

Included in save: yes.

Included in hash: yes.

### `clock`

Owned locally by `ClockState` and `Clock`.

Mutated through:

- clock/session control commands;
- reset/load aggregate replacement.

Included in save: yes.

Included in hash: yes.

### `camera`

Owned locally by `CameraState` and `CameraModePolicy`.

Mutated through:

- camera mode policy;
- reset/load aggregate replacement.

Included in save: yes.

Included in hash: yes.

### `commandLog`

Owned locally by `CommandLog`.

Mutated through:

- command submission/admission path;
- replay if replay owns a separate session;
- reset with `CommandLogResetPolicy::Clear`;
- load aggregate replacement.

Included in save: yes.

Included in hash: yes.

### `inventory`, `combat`, `ai`, `objectives`

Owned by their subsystem state types.

Mutated through:

- owning system APIs;
- reset/load aggregate replacement.

Included in save: yes.

Included in hash: yes.

### `baseline`

Owned by session.

Mutated:

- session creation sets baseline;
- loading rebuilds baseline from compatible package/scenario identity;
- normal gameplay does not mutate baseline.

Included in save: no. The first complete build does not save full baseline; it
rebuilds baseline from package/scenario id for replay/reset proof.

Included in hash: no for active state hash. `baselineHash` may be reported
separately as a diagnostic value.

### `transient`

Owned by session/diagnostics.

Mutated:

- event emission;
- metric collection;
- dirty-flag updates;
- reset/load clearing or regeneration.

Included in save: no.

Included in hash: no. Runtime events, metrics, dirty flags, and summary cache
state are excluded from first-build `StateHash`.

### `currentStateHash`

Owned by replay/hash boundary and cached in session.

Mutated:

- after session creation;
- after accepted/rejected command log changes;
- after gameplay mutation;
- after load;
- after reset.

Included in save: as metadata, yes.

Included in hash: no, to avoid self-referential hashing.

## Required Invariants

The implementation must preserve these invariants:

- `identity.packageId` is non-empty after successful session creation.
- `identity.scenarioId` is non-empty after successful session creation.
- `lifecycle != Loading` after successful session creation.
- `players` contains player slot 0 for the acceptance fixture.
- player slot 0 actor exists in `world`.
- `clock` and `camera` modes are compatible through `CameraModePolicy`.
- `baseline` is valid before reset is allowed.
- `currentStateHash` can be recomputed from state at any time.
- no renderer object is reachable from `SessionState`.
- no app file path is reachable from `SessionState`.
- no raw input event is reachable from `SessionState`.
- all vectors/lists whose order affects hash or replay use deterministic order.

## Session Creation Semantics

Session creation consumes validated seed data from
`FixtureScenarioLoader`/content boundary and produces `SessionState`.

Required creation results for first-room fixture:

- lifecycle `Playing`;
- outcome `None`;
- package id `iggy3d.first_room`;
- scenario id `first_room.runtime_loop`;
- world has exactly three entities;
- player slot 0 bound to `player`;
- clock `Normal`;
- camera `ThirdPerson`;
- inventory empty;
- objective `collect_gold_key=Active`;
- command log empty;
- `nextCommandId == 1`;
- baseline hash computed.

Content loader does not own the resulting state after creation.

## Reset Semantics

Reset is owned by `Session`, using fields declared here.

Required reset result:

- restore baseline world;
- restore baseline players;
- restore baseline clock;
- restore baseline camera;
- restore baseline inventory/combat/AI/objectives;
- lifecycle `Playing`;
- outcome `None`;
- clear transient events;
- reset command log with `CommandLogResetPolicy::Clear`, which clears records,
  resets next sequence to `1`, and increments command-log epoch by `1`;
- reset `nextCommandId` to `1`;
- recompute state hash;
- emit/reset diagnostic event through the runtime event path if events are
  retained after reset.

Reset must not be implemented as app-side manual field edits.

## Save Load Semantics

`SessionState` is the source for `SaveEnvelope`, but it is not the codec.

Save includes:

- identity;
- lifecycle;
- outcome;
- world;
- players;
- clock;
- camera;
- command log;
- next command id;
- inventory;
- combat;
- AI;
- objectives;
- current state hash as metadata.

Save excludes:

- transient events;
- runtime metrics;
- pending execution queues;
- projection;
- renderer;
- app paths;
- raw input;
- `currentStateHash` as a field that hashes itself.

Load transaction:

1. decode envelope outside `SessionState`;
2. validate compatibility;
3. construct candidate state;
4. recompute hash;
5. replace session state all-or-nothing through `Session`;
6. regenerate or clear transient state.

## Replay And State Hash Semantics

Replay depends on `SessionState` being fully deterministic.

Hash inclusion must be explicit:

- include identity;
- include lifecycle/outcome;
- include world state in stable entity order;
- include player roster in slot order;
- include clock and camera;
- include `nextCommandId`;
- include inventory in stable item order;
- include combat/AI/objective state in stable order;
- include command log records, next sequence, and epoch.

Hash must exclude:

- cached `currentStateHash`;
- transient events;
- metrics;
- projection;
- app state;
- renderer state.

Replay must create a fresh `SessionState` from baseline seed and drive it
through normal command admission/session tick paths.

## Multiplayer Semantics

`SessionState` must be compatible with future multiplayer:

- `PlayerRoster` supports more than one slot;
- command log records player slot/source;
- authority mode can be derived from roster/session state;
- session state does not assume a single hardcoded actor outside the first-room
  fixture;
- no network socket state is stored in session;
- decoded replication packet values feed command admission, not direct system
  mutation.

## Diagnostics And Errors

This header should not own diagnostic algorithms, but state shape must allow
diagnostics to explain:

- current lifecycle;
- current outcome;
- last or first rejection through command log;
- state hash;
- baseline hash;
- command counts;
- player/camera/clock state;
- objective status.

Expected failures involving invalid state should be reported by `Session`,
`SaveLoad`, `CommandAdmission`, or owning systems. This header only declares the
state needed for those diagnostics.

## Compute Cost

State construction:

- O(entity count plus player count plus subsystem state sizes).

Reset:

- O(size of baseline-owned state).

Save mapping:

- O(size of save truth).

Hash:

- O(size of hash-included state).

Memory:

- O(world entities plus command log plus subsystem state plus optional baseline
  copy).

The baseline snapshot duplicates meaningful session state. This is acceptable
for the first complete runtime because it keeps reset deterministic and simple.
Future memory reduction must preserve reset, save/load, and replay behavior.

## Tests And Verification

Covered by:

- `tests/unit/session_state_tests.cpp`;
- `tests/unit/save_load_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required unit assertions:

- default `SessionState` is `Loading` and `None`;
- created first-room state satisfies all creation semantics;
- state contains no renderer/app/raw input fields;
- reset restores baseline;
- load candidate replacement is all-or-nothing through `Session`;
- state hash excludes `currentStateHash`;
- player slot 0 actor exists in world;
- clock/camera compatibility holds.

Required acceptance assertions:

- final state matches `docs/acceptance_demo.md`;
- save/load preserves state hash;
- reset returns to baseline hash;
- replay reaches final hash.

## Completion Criteria

- `src/runtime/session/SessionState.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares lifecycle and outcome enums.
- It declares session identity, baseline snapshot, transient state if used, and
  aggregate `SessionState`.
- All authoritative runtime state has one owned field or documented owner.
- Save truth and transient/derived data are separated.
- Reset has enough baseline data to be deterministic.
- Replay and state hash can traverse the state deterministically.
- The header includes no old `/Users/kogaryu/iggy` code.
- It builds without app, projection, renderer, tests, or old `iggy`
  dependencies.
