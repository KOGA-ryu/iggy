# `src/runtime/command/CommandAdmission.hpp`

Updated: 2026-06-20

Exact purpose: declare the read-only command legality contract that turns a
pending `CommandRecord` into an accepted or rejected record before any gameplay
mutation occurs.

## Build Position

- priority rank: 58
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/command`
- file kind: `header`

This file is the gate between command values and gameplay execution. It defines
the admission API, the read-only context it may inspect, the first-failure rule
order, and the exact rejection reason contract consumed by command log, replay,
acceptance tests, diagnostics, and future multiplayer authority.

## Ownership

This file owns:

- admission request/result value shape;
- read-only admission context shape;
- rule-order contract;
- admission function declarations;
- command kind requirement helpers if they need admission context;
- retry admission contract;
- exact rejection reason selection contract.

It must not own:

- raw input conversion;
- player authority policy internals;
- command log append;
- gameplay mutation;
- movement execution;
- interaction execution;
- save/load file IO;
- target query implementation;
- reach query implementation;
- renderer picking;
- network sockets;
- old `/Users/kogaryu/iggy` adapters.

## Required Header Shape

The implementation file must be:

```text
src/runtime/command/CommandAdmission.hpp
```

Required include style:

```cpp
#pragma once

#include "core/result/Result.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"
```

If including `CommandLog`, `TargetQuery`, or `ReachQuery` creates cycles, this
header may forward declare request/result types and include their headers in
`CommandAdmission.cpp`. The public API must still make the needed read-only
dependencies explicit.

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Public Types

### `CommandAdmissionContext`

Declare this exact read-only context:

```cpp
struct CommandAdmissionContext {
  const WorldState* world = nullptr;
  const PlayerRoster* players = nullptr;
  const ClockState* clock = nullptr;
  const CommandLog* commandLog = nullptr;
  const RuntimeConfig* config = nullptr;
};
```

If `RuntimeConfig` is not included in this header, use a forward declaration.

Semantics:

- all pointers/references are read-only;
- missing required context rejects as `InternalError` or returns invalid result
  before producing an accepted command;
- admission never mutates context;
- admission never appends to `CommandLog`;
- admission never changes `WorldState`;
- target/reach checks must read from context only.

### `CommandAdmissionRequest`

Declare this exact request:

```cpp
struct CommandAdmissionRequest {
  CommandRecord command;
  bool allowSessionControlWhilePaused = true;
};
```

Semantics:

- request owns a copy of the pending command;
- admission returns a new command record with admission/rejection fields set;
- caller decides whether to append returned record to command log;
- session-control commands are allowed while paused only according to documented
  command kind rules.

### `CommandAdmissionResult`

Declare this exact result:

```cpp
struct CommandAdmissionResult {
  CommandRecord command;
  CommandRejectionReason firstFailure = CommandRejectionReason::None;
};
```

Semantics:

- `command.admission` must be `Accepted` or `Rejected`;
- if accepted, `command.rejection == None`;
- if rejected, `command.rejection == firstFailure`;
- `firstFailure` is the first failing rule in the documented order;
- result carries no side effects.

The complete-build API returns `CommandAdmissionResult` directly. Malformed
context and internal errors are represented as rejected command records with the
documented `CommandRejectionReason`; gameplay legality is always represented by
accepted/rejected command records.

## Required API

Declare these exact public functions:

```cpp
CommandAdmissionResult admitCommand(
    const CommandAdmissionContext& context,
    const CommandAdmissionRequest& request);

CommandAdmissionResult rejectCommand(
    CommandRecord command,
    CommandRejectionReason reason);

CommandAdmissionResult acceptCommand(CommandRecord command);
```

Any decomposition below this public API is private implementation detail.
Private helpers must remain read-only and deterministic and must preserve the
rule order in this document.

## Admission Rule Order

First failing rule wins. The implementation must apply rules in this order
unless this document and tests are updated.

1. Context validity.
2. Command shape validity.
3. Player slot validity.
4. Authority-facing actor binding validity.
5. Clock mode validity.
6. Retry source validity for `Retry`.
7. Target existence and activity.
8. Targetability for command kind.
9. Target point finite/valid.
10. Reach/range validation.
11. Kind-specific final admission.

This order is acceptance-sensitive because replay must reproduce the same
rejection reason.

## Rule Details

### 1. Context Validity

Required context:

- `world`;
- `players`;
- `clock`;
- `commandLog` for retry;
- `config` if movement/reach defaults are read through config.

Failure:

- missing required context returns `InternalError` or a failed `Result`;
- it must never return `Accepted`.

### 2. Command Shape Validity

Reject `InvalidCommand` when:

- `kind == None`;
- `commandId == kInvalidCommandId`;
- admission is already accepted/rejected when pending is required;
- command kind requires actor but actor is invalid;
- command kind requires target but target is absent;
- command kind forbids target but invalid target data would affect semantics.

Command identity timing:

- `Session` command construction owns assigning stable nonzero
  `CommandRecord::commandId` before lifecycle prefiltering and before
  `CommandAdmission`.
- `CommandLog::append` owns assigning deterministic `CommandRecord::sequence`
  when the sequence is invalid/unset.
- `CommandAdmission` never assigns `commandId` and never assigns `sequence`.
- Admission may read `commandId` for diagnostics and retry/source linkage, but
  must not mutate `commandId`.

### 3. Player Slot Validity

Reject `InvalidPlayerSlot` when:

- `playerSlot == kInvalidPlayerSlotId`;
- player slot does not exist in `PlayerRoster`;
- command kind requires a player slot and none is supplied.

The complete build uses player slot `0` for submitted gameplay commands.
Reset/save/load proof APIs are outside the gameplay admission path and do not
require a command payload player slot.

### 4. Actor Binding Validity

Reject `InvalidActor` when:

- command kind requires actor and actor id is invalid;
- actor id is not present in `WorldState`;
- actor entity is inactive/defeated if the owning systems expose that state.

Reject `ActorNotControlledBySlot` when:

- actor exists but is not bound to command player slot;
- command source is not allowed to control that actor.

`Authority` owns broader authorization. This file owns local actor/slot binding
legality if authority has not already rejected the command.

### 5. Clock Validity

Lifecycle ownership:

- Session owns lifecycle gating before command admission.
- `Session` owns lifecycle gating before calling `CommandAdmission`.
- `CommandAdmissionContext` carries `ClockState`, not `SessionLifecycle`.
- `Session::submitCommand` rejects or routes commands when lifecycle is not
  playable before admission is called.
- `CommandAdmission` owns clock/pause-specific validation, reach/target
  validation, roster/actor binding, retry re-admission, and command-level
  rejection reasons available from `CommandAdmissionContext`.
- `CommandAdmission` does not inspect lifecycle and does not produce lifecycle
  rejection decisions from hidden state.

Reject `SessionPaused` when:

- paused mode blocks this command kind;
- command is not `Resume`, `StepTacticalTick`, `Reset`, `Save`, `Load`, or
  `Pause`.

Reject `StepRequiresPaused` when:

- `StepTacticalTick` is submitted while not paused.

Allowed while paused:

- `Resume`;
- `StepTacticalTick`;
- `Reset`;
- `Save`;
- `Load`;
- `Pause`.

`Pause` while already paused is accepted as an idempotent no-op.
`ToggleTacticalMode`, `Move`, `Interact`, `Inspect`, and `Wait` reject while
paused with `SessionPaused`.

### 6. Retry Source Validity

For `Retry`, inspect `CommandLog`.

Reject `RetrySourceMissing` when:

- `payload.retrySourceCommandId == kInvalidCommandId`;
- source `commandId` does not exist in log.

Reject `RetrySourceNotRejected` when:

- source exists but `admission != Rejected`.

Reject `RetryUnsupportedKind` when:

- source kind is `Retry`;
- source kind is `Reset`;
- source kind is `Save`;
- source kind is `Load`;
- source kind is a future command explicitly marked nonretryable.

If retry is valid:

- effective validation inside admission reads original rejected command
  kind/payload/actor/target/point/interaction intent from the source rejected
  command;
- retry record keeps its own `commandId`, sequence, submitting player slot,
  admission status, rejection reason, and retry metadata in the returned
  accepted/rejected retry command;
- use current actor/world/clock/target state;
- run normal admission checks again;
- if admitted, returned record is the retry command with accepted status, and
  `SessionTick` later resolves it into an `EffectiveCommandIntent`.

Retry must not bypass authority, target, or reach checks.
The command log keeps the accepted retry record as `CommandKind::Retry`; owning
systems receive only the normalized effective command produced by `SessionTick`.
Admission does not create a stored normalized command record and does not append
to the log.

`SessionTick` later creates `EffectiveCommandIntent` by copying the original
rejected command's executable intent fields, then overwriting exactly these
`CommandRecord` fields from the accepted retry record: `commandId`, `sequence`,
`playerSlot`, `admission`, and `rejection`. The `EffectiveCommandIntent` wrapper
stores `sourceCommandId` as the accepted retry `commandId` and
`retrySourceCommandId` as the original rejected `commandId`. No actor, target, target
point, command kind, or interaction payload field is overwritten.

`CommandPayload::retrySourceCommandId` is the retry command payload input used to
locate the rejected command. It is distinct from
`EffectiveCommandIntent::retrySourceCommandId`, which is execution-result linkage
after retry resolution.

### 7. Target Existence And Activity

For entity-target commands, reject `InvalidTarget` when:

- target entity id is invalid;
- target entity does not exist in world.

Reject `TargetInactive` when:

- target exists but is inactive and the command kind requires active target.

Complete-build `Inspect` requires the target entity to be active and rejects
inactive targets as `TargetInactive`.

### 8. Targetability For Command Kind

Reject `InvalidTarget` when:

- target exists but does not support the requested command kind;
- self-targeting is attempted by a command kind that forbids self-targeting;
- marker/pickup/door/combat target kind does not match command semantics.

Admission uses `TargetQuery`/`targetSupportsCommandKind` for targetability facts
and owns turning that result into the exact rejection reason.

### 9. Target Point Validity

Reject `InvalidTargetPoint` when:

- point target is required and absent;
- point target contains non-finite values;
- point target is outside accepted runtime coordinate bounds if such bounds are
  introduced.

Move commands in the first acceptance demo require finite point targets.

### 10. Reach And Range Validation

For reach-gated commands, call `ReachQuery`.

Reject `OutOfRange` when:

- actor and target are valid;
- target is active and targetable;
- distance exceeds configured reach.

Acceptance-sensitive case:

```text
Interact(player at 0,0,0, gold_key at 3,0,0) -> OutOfRange
```

Do not return `TargetNotReachable` for this first demo case. `TargetNotReachable`
is reserved for future path/nav style failures, while `OutOfRange` is the exact
range rejection.

### 11. Kind-Specific Final Admission

Accept command when:

- all required checks for its kind pass;
- admission status becomes `Accepted`;
- rejection becomes `None`.

Reject with the relevant reason when a final kind-specific rule fails:

- movement too far: `MovementTooFar`;
- save unavailable: `SaveUnavailable`;
- load unavailable: `LoadUnavailable`;
- reset unavailable: `ResetUnavailable`;
- incompatible save: `IncompatibleSave`.

## Command Kind Admission Contract

### `Move`

Requires:

- valid player slot;
- valid actor;
- actor controlled by slot;
- valid target point;
- movement distance from actor transform position to target point is within
  `RuntimeConfig::movementDistanceMeters`.

May reject:

- `InvalidPlayerSlot`;
- `InvalidActor`;
- `ActorNotControlledBySlot`;
- `InvalidTargetPoint`;
- `MovementTooFar`;
- `SessionPaused`.

Execution owner after acceptance:

- `MovementSystem`.

### `Interact`

Requires:

- valid player slot;
- valid actor;
- actor controlled by slot;
- valid active entity target;
- target supports interaction;
- reach succeeds.

May reject:

- `InvalidActor`;
- `ActorNotControlledBySlot`;
- `InvalidTarget`;
- `TargetInactive`;
- `OutOfRange`;
- `SessionPaused`.

Acceptance first command must reject `OutOfRange`.

Execution owner after acceptance:

- `InteractionSystem`.

### `Inspect`

Requires:

- valid player slot;
- target entity.

May reject:

- `InvalidTarget`;
- `TargetInactive`.

Complete-build `Inspect` requires valid player slot and target entity, is not
reach-gated, and never rejects `OutOfRange`.

### `Wait`

Requires:

- valid player slot.

May reject:

- `InvalidPlayerSlot`;
- `SessionPaused`.

Wait is accepted in normal and slow modes. Wait while paused rejects with
`SessionPaused`.

### `ToggleTacticalMode`

Requires:

- valid player slot.

May reject:

- `InvalidPlayerSlot`;
- `SessionPaused`.

Allowed in normal and slow mode. Toggle while paused rejects with
`SessionPaused`.

### `Pause`

Requires:

- valid player slot.

May reject:

- `InvalidPlayerSlot`.

Repeated pause behavior:

- accepted idempotent no-op while already paused.

### `Resume`

Requires:

- valid player slot;
- clock paused or normal/slow mode for idempotent no-op.

May reject:

- `InvalidPlayerSlot`.

Repeated resume behavior:

- accepted idempotent no-op if already running.

### `StepTacticalTick`

Requires:

- valid player slot;
- paused clock.

May reject:

- `InvalidPlayerSlot`;
- `StepRequiresPaused`.

Execution owner after acceptance:

- session runner / step path.

### `Retry`

Requires:

- valid player slot;
- valid rejected source `commandId`;
- retryable source kind;
- source intent valid under current state.

May reject:

- `RetrySourceMissing`;
- `RetrySourceNotRejected`;
- `RetryUnsupportedKind`;
- any reason the original intent would currently reject with.

Acceptance demo requirement:

- retry of `cmd_interact_oob` after movement must be accepted.

### `Reset`

Requires:

- valid player slot or tool source;
- baseline available.

May reject:

- `InvalidPlayerSlot`;
- `ResetUnavailable`.

Execution owner:

- `Session`.

### `Save`

Requires:

- valid player slot or tool source;
- save system available.

May reject:

- `SaveUnavailable`.

Admission must not inspect or store file paths.

### `Load`

Requires:

- valid player slot or tool source;
- decoded compatible envelope available through save/load path.

May reject:

- `LoadUnavailable`;
- `IncompatibleSave`.

Admission must not inspect or store file paths.

## Acceptance Demo Admission Requirements

The first-room acceptance demo requires this exact behavior:

| Label | Command | Expected Admission Result |
| --- | --- | --- |
| `cmd_interact_oob` | `Interact(player, gold_key)` from `(0,0,0)` | rejected `OutOfRange` |
| `cmd_move_to_key` | `Move(player, (2,0,0))` | accepted |
| `cmd_retry_key` | `Retry(cmd_interact_oob)` after move | accepted |
| `cmd_enter_tactical` | `ToggleTacticalMode` | accepted |
| `cmd_tactical_move` | `Move(player, (2,0,1))` | accepted |
| `cmd_pause` | `Pause` | accepted |
| `cmd_step` | `StepTacticalTick` while paused | accepted |
| `cmd_resume` | `Resume` | accepted |
| `cmd_wait` | `Wait` in slow mode, not paused | accepted |
| `cmd_exit_tactical` | `ToggleTacticalMode` | accepted |

The first rejection reason must be exactly:

```text
OutOfRange
```

No other reason is acceptable for `cmd_interact_oob`.

## Save Replay Multiplayer Notes

Admission affects save/replay through command records.

Save must preserve:

- admission status;
- rejection reason;
- `commandId`;
- sequence;
- retry source `commandId`;
- target fields.

Replay must reproduce:

- same accepted/rejected status for each command;
- same rejection reason for rejected commands;
- same retry admission after movement changes world state.

Multiplayer must:

- run decoded remote command values through authority and admission;
- reject unauthorized slot before local legality if authority is separate;
- preserve deterministic rejection reason order after authority passes;
- never dispatch remote packets directly to systems.

## Diagnostics And Errors

This file owns rejection reason choice, not human diagnostic text.

Expected diagnostic producers:

- `Authority` may reject before this file for `UnauthorizedSlot`.
- `CommandAdmission` assigns gameplay legality rejection reason.
- `ReachQuery` computes range facts used for `OutOfRange`.
- `CommandLog` stores the final admitted/rejected record.
- `RuntimeEvent` reports accepted/rejected event.

Diagnostics must include enough context for:

- `commandId`;
- sequence;
- player slot;
- actor `EntityId`;
- target `EntityId` or target point;
- rejection reason;
- issued/scheduled tick.

## Compute Cost

Baseline costs:

- pure session-control admission: O(1);
- player slot lookup: O(player count) unless roster has direct lookup;
- actor/target lookup: O(entity count);
- targetability validation: O(1) after entity lookup;
- reach validation: O(entity lookup plus constant math);
- retry source lookup: O(command count) unless command log has an index.

Acceptance scale is tiny. Future indexes are allowed only if they preserve
deterministic behavior and replay results.

## Tests And Verification

Covered by:

- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/target_reach_tests.cpp`;
- `tests/unit/session_state_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required unit assertions:

- invalid command kind rejects `InvalidCommand`;
- missing player slot rejects `InvalidPlayerSlot`;
- invalid actor rejects `InvalidActor`;
- actor/slot mismatch rejects `ActorNotControlledBySlot`;
- inactive target rejects `TargetInactive`;
- initial key interaction rejects exactly `OutOfRange`;
- move to finite in-range point accepts;
- move to invalid point rejects `InvalidTargetPoint`;
- step while not paused rejects `StepRequiresPaused`;
- step while paused accepts;
- retry missing source rejects `RetrySourceMissing`;
- retry accepted source rejects `RetrySourceNotRejected`;
- retry out-of-range interact after movement accepts;
- rejected commands do not mutate world;
- admission does not append to command log by itself.

Required acceptance assertions:

- `cmd_interact_oob` rejection reason is `OutOfRange`;
- `cmd_retry_key` accepts only after movement into reach;
- all accepted commands are logged and executed through normal session path;
- replay reproduces the same admission outcomes.

## Completion Criteria

- `src/runtime/command/CommandAdmission.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares admission context, request, result, and public admission API.
- It documents and supports first-failure rule order.
- It can reject `cmd_interact_oob` as exactly `OutOfRange`.
- It can accept retry after movement into reach.
- It is read-only and cannot mutate world, session, command log, or systems.
- It does not include app, renderer, projection, tests, sockets, or old `iggy`
  dependencies.
- It gives `CommandAdmission.cpp` enough contract detail to implement without
  chat context.
