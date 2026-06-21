# `src/runtime/command/Command.hpp`

Updated: 2026-06-20

Exact purpose: declare the serializable command value model for gameplay,
session control, retry/reset, save/load requests, deterministic replay, and
future multiplayer replication.

## Build Position

- priority rank: 57
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/command`
- file kind: `header`

This file defines command data only. It does not validate, execute, dispatch, or
mutate gameplay. It is the shared command vocabulary consumed by authority,
admission, command log, session tick, save/load, replay, local multiplayer, and
tools.

## Ownership

This file owns:

- `commandId`/sequence value conventions;
- command kind enum;
- command source enum;
- command admission status enum;
- command rejection reason enum;
- command payload shape;
- target reference shape;
- command record shape;
- retry source reference fields;
- lightweight helper predicates if they are pure and deterministic.

It must not own:

- command validation logic;
- command execution side effects;
- command log append policy;
- authority policy;
- target discovery;
- reach calculation;
- raw input events;
- app CLI paths;
- renderer picking ids;
- network sockets;
- old `/Users/kogaryu/iggy` adapters.

## Required Header Shape

The implementation file must be:

```text
src/runtime/command/Command.hpp
```

Required include style:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/player/PlayerSlot.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

Do not include runtime systems, app code, projection code, save codec, replay
tool, renderer headers, or test headers from this file.

## Command Identity Types

Declare identity aliases:

```cpp
using CommandId = std::uint64_t;
using CommandSequence = std::uint64_t;
using CommandTick = std::uint64_t;
```

Required sentinel values:

```cpp
inline constexpr CommandId kInvalidCommandId = 0;
inline constexpr CommandSequence kInvalidCommandSequence = 0;
inline constexpr CommandTick kInvalidCommandTick = 0;
```

Semantics:

- `CommandId` is the identity type for commands;
- `CommandRecord::commandId` is assigned by `Session` command construction
  before lifecycle prefiltering and before `CommandAdmission`;
- `SessionState::nextCommandId` is the authoritative allocation cursor for
  `CommandRecord::commandId`;
- `CommandLog` never allocates `commandId`; it only rejects/asserts invalid or
  duplicate `commandId` values;
- `CommandRecord::sequence` is assigned by `CommandLog::append` when unset and
  is replay order;
- command tick is the session tick when the command was issued or scheduled;
- `PlayerSlotId` and `kInvalidPlayerSlotId` are owned by
  `runtime/player/PlayerSlot.hpp`;
- player slot identifies the source participant for authority and multiplayer;
- `commandId == 0` is invalid so acceptance labels can map to real nonzero
  `CommandId` values.

## Required Enums

### `CommandKind`

Declare:

```cpp
enum class CommandKind : std::uint8_t {
  None,
  Move,
  Interact,
  Inspect,
  Wait,
  ToggleTacticalMode,
  Pause,
  Resume,
  StepTacticalTick,
  Retry,
  Reset,
  Save,
  Load,
};
```

Semantics:

- `None`: invalid/unset command.
- `Move`: actor movement to a target point or target entity.
- `Interact`: reach-gated effect against target entity.
- `Inspect`: read-only target inspection.
- `Wait`: accepted no-op command for tick/log proof.
- `ToggleTacticalMode`: normal <-> slow/tactical mode transition.
- `Pause`: enter paused clock mode.
- `Resume`: leave paused mode and return to previous run mode.
- `StepTacticalTick`: advance exactly one tick while paused.
- `Retry`: re-attempt a previously rejected command intent.
- `Reset`: reserved command kind for non-acceptance control tooling; first
  complete build acceptance reset proof uses `Session::resetToBaseline` and does
  not submit this as a gameplay command record.
- `Save`: reserved command kind for non-acceptance control tooling; first
  complete build save proof uses `SaveLoad` proof/tool APIs and does not submit
  this as a gameplay command record.
- `Load`: reserved command kind for non-acceptance control tooling; first
  complete build load proof uses `SaveLoad` proof/tool APIs and does not submit
  this as a gameplay command record.

`Reset`, `Save`, and `Load` are excluded from the first-build acceptance
gameplay command log and command counts. App/tool code owns file paths and
process IO.

### `CommandSource`

Declare:

```cpp
enum class CommandSource : std::uint8_t {
  Unknown,
  LocalPlayer,
  Ai,
  Script,
  Replay,
  RemotePlayer,
  Tool,
};
```

Semantics:

- local gameplay input becomes `LocalPlayer` after raw input conversion;
- acceptance script uses `Script`;
- replay uses `Replay`;
- AI proposals use `Ai`;
- future decoded network packets use `RemotePlayer`;
- tools use `Tool` for save/load/replay proof requests.

Raw keyboard/mouse/controller events are not command sources and must not be
stored here.

### `CommandAdmissionStatus`

Declare:

```cpp
enum class CommandAdmissionStatus : std::uint8_t {
  Pending,
  Accepted,
  Rejected,
};
```

Semantics:

- new command records start as `Pending`;
- authority/admission code sets `Accepted` or `Rejected`;
- only `Accepted` commands can execute;
- `Rejected` commands remain in command log for diagnostics and retry.

### `CommandRejectionReason`

Declare:

```cpp
enum class CommandRejectionReason : std::uint8_t {
  None,
  InvalidCommand,
  InvalidPlayerSlot,
  UnauthorizedSlot,
  InvalidActor,
  ActorNotControlledBySlot,
  InvalidTarget,
  TargetInactive,
  TargetNotReachable,
  OutOfRange,
  InvalidTargetPoint,
  MovementTooFar,
  SessionNotPlaying,
  SessionPaused,
  StepRequiresPaused,
  RetrySourceMissing,
  RetrySourceNotRejected,
  RetryUnsupportedKind,
  ResetUnavailable,
  SaveUnavailable,
  LoadUnavailable,
  IncompatibleSave,
  InternalError,
};
```

Acceptance-sensitive value:

- first failed interaction must reject with `OutOfRange`.

Rejection reason rules:

- `None` is required for accepted commands;
- rejected commands must use a non-`None` reason;
- first failing rule wins according to `CommandAdmission` documented order;
- human text is non-normative; enum values remain stable once tests lock them.

## Required Target Shape

Declare:

```cpp
struct CommandTarget {
  bool hasEntity = false;
  EntityId entity;
  bool hasPoint = false;
  Vec3 point;
};
```

Semantics:

- entity targets are used by `Interact`, `Inspect`, retry of entity commands,
  and future combat;
- point targets are used by `Move` and tactical movement;
- both are present only when a command targets an entity with an explicit point;
- neither present is valid only for pure session-control commands;
- `CommandAdmission` decides whether the chosen kind requires entity, point, or
  neither.

This type does not own target discovery results. It stores the selected target
intent after input/script/AI/network conversion.

## Required Command Payload Shape

Declare:

```cpp
struct CommandPayload {
  CommandTarget target;
  CommandId retrySourceCommandId = kInvalidCommandId;
  std::uint64_t userData0 = 0;
  std::uint64_t userData1 = 0;
};
```

Semantics:

- `target` stores entity/point command intent;
- `CommandPayload::retrySourceCommandId` is required for `Retry` and stores a
  source `CommandRecord::commandId`;
- `userData0` and `userData1` are reserved deterministic extension fields;
- extension fields must not store pointers, renderer identifiers, app file paths, or
  raw input objects.

Named helper APIs expose descriptive accessors for `userData0/1`; the
stored field order and save/replay/replication mapping remain exactly as
declared.

## Required `CommandRecord`

Declare:

```cpp
struct CommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  CommandKind kind = CommandKind::None;
  CommandSource source = CommandSource::Unknown;
  CommandPayload payload;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};
```

Required field semantics:

- `commandId`: stable `CommandId` value assigned by `Session` command
  construction before lifecycle prefiltering and before `CommandAdmission`.
- `sequence`: deterministic command-log order assigned by `CommandLog::append`
  when unset; replay order is sequence-driven.
- `playerSlot`: command authority source.
- `actor`: entity performing command when applicable.
- `kind`: command behavior category.
- `source`: origin after raw input/network/tool conversion.
- `payload`: target/retry/extension data.
- `issuedTick`: session tick when command was created.
- `scheduledTick`: tick command should execute; equals `issuedTick` for
  immediate same-tick submissions.
- `admission`: pending/accepted/rejected status.
- `rejection`: exact rejection reason.

No field may contain:

- raw input events;
- raw network packet bytes;
- renderer object identifiers;
- pointers;
- file paths;
- old `iggy` identifiers unless converted into owned `iggy3d` identifiers.

## Acceptance Label Support

Acceptance docs use stable labels such as `cmd_interact_oob`.

Declare:

```cpp
struct CommandDebugLabel {
  CommandId commandId = kInvalidCommandId;
  const char* label = nullptr;
};
```

Labels remain test/tool/debug data and are not save truth. Labels are stored
outside `CommandRecord` in the first complete build and are excluded from state
hash because they are not deterministic runtime truth.

## Pure Helper Functions

Declare simple pure helpers:

```cpp
bool isSessionControlCommand(CommandKind kind);
bool requiresActor(CommandKind kind);
bool requiresEntityTarget(CommandKind kind);
bool requiresPointTarget(CommandKind kind);
bool isAccepted(const CommandRecord& command);
bool isRejected(const CommandRecord& command);
```

Helper rules:

- helpers must be deterministic;
- helpers must not inspect `WorldState`;
- helpers must not perform reach checks;
- helpers must not mutate a command;
- helpers must remain O(1).

Nontrivial validation belongs in `CommandAdmission`.

## Command Kind Requirements

### `Move`

Required fields:

- valid `playerSlot`;
- valid `actor`;
- finite point target.

Admission dependencies:

- actor exists;
- actor controlled by player slot;
- target point finite;
- movement distance allowed by config/system.

Execution owner:

- `MovementSystem`.

### `Interact`

Required fields:

- valid `playerSlot`;
- valid `actor`;
- entity target.

Admission dependencies:

- actor exists;
- target exists;
- target active;
- target supports interaction;
- reach succeeds.

Execution owner:

- `InteractionSystem`.

Acceptance first rejection:

- `Interact(player, gold_key)` at initial position rejects `OutOfRange`.

### `Inspect`

Required fields:

- valid actor or valid observer slot;
- entity target.

Admission dependencies:

- target exists;
- target inspectable.

Execution owner:

- none or inspect/query system if added.

Inspect must not mutate gameplay state.

### `Wait`

Required fields:

- valid player slot;
- valid actor.

Execution owner:

- session/tick no-op.

Wait affects command log and accepted command count. It does not mutate gameplay
world state directly; state-hash impact comes from the appended command-log
record, next sequence, and epoch.

### `ToggleTacticalMode`

Required fields:

- valid player slot.

Execution owner:

- session control path using `Clock` and `CameraModePolicy`.

Effects:

- normal -> slow/tactical;
- slow/tactical -> normal/realtime;
- camera previous realtime mode preserved/restored.

### `Pause`

Required fields:

- valid player slot.

Execution owner:

- session control path using `Clock`.

Effect:

- clock mode becomes paused;
- camera is preserved.

### `Resume`

Required fields:

- valid player slot.

Execution owner:

- session control path using `Clock`.

Effect:

- clock resumes previous non-paused mode.

### `StepTacticalTick`

Required fields:

- valid player slot.

Admission dependency:

- clock/session must be paused.

Execution owner:

- `SessionRunner` or session step path.

Effect:

- exactly one tick executes;
- clock remains paused afterward.

### `Retry`

Required fields:

- valid player slot;
- `payload.retrySourceCommandId` references an existing rejected
  `CommandRecord::commandId`.

Admission dependencies:

- source command exists by `payload.retrySourceCommandId`;
- source command was rejected;
- source command kind is retryable;
- retry command reuses original intent under current state;
- authority/admission checks run again.

Execution owner:

- same owner as original command if retry is admitted.

Retry must not:

- bypass authority;
- bypass reach checks;
- mutate state if re-admission fails;
- retry `Reset`, `Save`, or `Load` in the first complete build.

### `Reset`

Required fields:

- valid player slot or tool source.

Execution owner:

- reserved for non-acceptance control tooling.

Effect:

- first-build acceptance reset proof calls `Session::resetToBaseline` directly
  and records `ResetCompleted` as a proof event.

Reset command records are not part of the first-build acceptance gameplay
command log and do not affect the deterministic ten-command count.

### `Save`

Required fields:

- valid player slot or tool source.

Execution owner:

- reserved for non-acceptance control tooling.

Effect:

- first-build acceptance save proof calls `SaveLoad` directly and records
  `SaveCreated` as a proof event.

This command is not part of the first-build acceptance gameplay command log.
It must not store file paths. App/tool owns destination path.

### `Load`

Required fields:

- valid player slot or tool source.

Execution owner:

- reserved for non-acceptance control tooling.

Effect:

- first-build acceptance load proof calls `SaveLoad` directly and records
  `LoadCompleted` as a proof event. This command is not part of the first-build
  acceptance gameplay command log.

This command must not store file paths. App/tool owns source path and decoded
envelope handoff.

## Acceptance Command Labels

The first acceptance demo uses these command intents:

| Label | Kind | Required Target/Payload | Expected Admission |
| --- | --- | --- | --- |
| `cmd_interact_oob` | `Interact` | actor `player`, entity `gold_key` | rejected `OutOfRange` |
| `cmd_move_to_key` | `Move` | actor `player`, point `(2.000,0.000,0.000)` | accepted |
| `cmd_retry_key` | `Retry` | source `cmd_interact_oob` | accepted |
| `cmd_enter_tactical` | `ToggleTacticalMode` | no target | accepted |
| `cmd_tactical_move` | `Move` | actor `player`, point `(2.000,0.000,1.000)` | accepted |
| `cmd_pause` | `Pause` | no target | accepted |
| `cmd_step` | `StepTacticalTick` | no target | accepted |
| `cmd_resume` | `Resume` | no target | accepted |
| `cmd_wait` | `Wait` | no target | accepted |
| `cmd_exit_tactical` | `ToggleTacticalMode` | no target | accepted |

Expected gameplay command counts:

- submitted: `10`;
- accepted: `9`;
- rejected: `1`;
- retry: `1`.

## Save Replay Multiplayer Notes

Command records are save and replay truth.

Save must preserve:

- `commandId`;
- `sequence`;
- player slot;
- actor `EntityId`;
- kind;
- source;
- payload target entity/point flags;
- `payload.retrySourceCommandId`;
- issued/scheduled tick;
- admission status;
- rejection reason.

Replay must:

- replay rejected commands and verify same rejection reason;
- replay accepted commands through normal authority/admission/session flow;
- preserve deterministic command ordering;
- report first mismatch by `commandId`, `sequence`, and acceptance label when a
  test/tool label was supplied.

Multiplayer must:

- use `playerSlot` for authority;
- preserve packet/local sequence values in replication packet records; canonical
  runtime command ordering remains `CommandRecord::sequence`;
- serialize command records through replication packet values, not raw sockets;
- never let decoded packets bypass command admission.

## State Hash Notes

The first complete build includes command log state in `StateHash`.

Hash input includes:

- `SessionState::nextCommandId`;
- command records in canonical `CommandLog::records()` order;
- `CommandLog::nextSequence`;
- `CommandLog::epoch`;
- every command field listed as save truth in stable field order.

Hash input excludes debug labels because labels are test/tool/debug metadata,
not deterministic save truth.

## Diagnostics And Errors

This header declares rejection reason values but does not create diagnostics.

Diagnostic producers:

- `Authority` produces unauthorized/slot reasons.
- `CommandAdmission` produces invalid actor/target/session/retry reasons.
- `ReachQuery` result feeds `OutOfRange`.
- `SaveCompatibility` feeds incompatible save reasons.
- `CommandReplay` reports divergence.

Required stable acceptance reason:

```text
OutOfRange
```

No builder may replace this with a prose-only message.

## Compute Cost

Command value operations:

- construction: O(1);
- copy/move: O(1), excluding external debug-label storage;
- helper predicates: O(1);
- serialization by save/replication codec: O(1) per command plus fixed field
  bytes.

Command log scans are owned by `CommandLog`, not this header.

## Tests And Verification

Covered by:

- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/unit_multiplayer_authority_tests.cpp` plan, implemented as
  `tests/unit/multiplayer_authority_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- every command kind has stable helper behavior;
- accepted command uses `rejection=None`;
- rejected command uses non-`None` reason;
- `Retry` requires a valid rejected source `CommandRecord::commandId` in
  `payload.retrySourceCommandId`;
- `Move` can express point target;
- `Interact` can express entity target;
- session-control commands do not require entity/point target;
- `Save`/`Load` commands do not contain file paths;
- command records can be round-tripped through save/replay/replication codecs
  once those files exist.

## Completion Criteria

- `src/runtime/command/Command.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares `CommandId`, `CommandSequence`, `CommandTick`, and
  `PlayerSlotId`.
- It declares `CommandKind`, `CommandSource`, `CommandAdmissionStatus`, and
  `CommandRejectionReason`.
- It declares `CommandTarget`, `CommandPayload`, and `CommandRecord`.
- It can represent every acceptance command label in
  `docs/acceptance_demo.md`.
- It has no app, renderer, projection, test, network socket, or old `iggy`
  dependency.
- It separates command value shape from authority, admission, execution, save
  codec, and replay behavior.
- Tests can assert exact `OutOfRange` rejection for the first failed
  interaction.
