# `src/runtime/command/CommandAdmission.cpp`

Updated: 2026-06-20

Exact purpose: implement the read-only command admission algorithm declared by
`CommandAdmission.hpp`, preserving deterministic first-failure rejection order
before any gameplay mutation or command execution occurs.

## Build Position

- priority rank: 59
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/command`
- file kind: `source`

This source file is the executable implementation of the admission gate. It
must be boring, explicit, deterministic, and easy to audit because every command
that mutates gameplay passes through this logic.

## Ownership

This file owns:

- implementation of `admitCommand`;
- implementation of `acceptCommand`;
- implementation of `rejectCommand`;
- implementation of shape, actor, session, target, reach, and retry checks;
- exact first-failure rule order from `CommandAdmission.hpp`;
- mapping read-only target/reach results to `CommandRejectionReason`.

It must not own:

- command record construction policy beyond setting admission/rejection fields;
- `commandId` allocation;
- command log append;
- authority policy broader than actor/player binding checks;
- gameplay execution;
- world mutation;
- clock/camera mutation;
- interaction effects;
- movement effects;
- save/load file IO;
- renderer picking;
- network transport;
- old `/Users/kogaryu/iggy` adapters.

## Required Include Order

Implementation must include the paired header first:

```cpp
#include "runtime/command/CommandAdmission.hpp"
```

Then include only required implementation dependencies:

```cpp
#include <cmath>

#include "config/RuntimeConfig.hpp"
#include "runtime/clock/Clock.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"
```

Only include headers that are actually required by implementation. Do not
include app, projection, renderer, tests, socket, or old `iggy` headers.

## Internal Helper Layout

Prefer unnamed-namespace helpers in this order:

1. context validation helper;
2. command shape helper;
3. player slot helper;
4. actor binding helper;
5. clock helper;
6. retry source lookup/reconstruction helper;
7. target existence/activity helper;
8. targetability helper;
9. point validation helper;
10. reach/range helper;
11. kind-specific helper;
12. final accept/reject helpers if not public.

Each helper must return either:

```cpp
CommandRejectionReason
```

or a small local result containing the rejection reason and any read-only lookup
facts needed by later checks.

Helpers must not mutate inputs.

## Public Function Behavior

### `rejectCommand`

Implementation:

1. take command by value;
2. set `command.admission = CommandAdmissionStatus::Rejected`;
3. set `command.rejection = reason`;
4. if `reason == None`, replace with `InternalError` or assert in debug and
   still return a non-`None` failure in release;
5. return `CommandAdmissionResult{command, command.rejection}`.

Rules:

- rejected command keeps original `commandId`, sequence, actor, source, and
  payload;
- rejection must not clear target/retry data because replay needs original
  intent.

### `acceptCommand`

Implementation:

1. take command by value;
2. set `command.admission = CommandAdmissionStatus::Accepted`;
3. set `command.rejection = CommandRejectionReason::None`;
4. return `CommandAdmissionResult{command, None}`.

Rules:

- accepted command keeps target/retry data;
- accepted command does not execute here;
- accepted command is not appended to log here.

### `admitCommand`

Implementation must follow the exact first-failure order:

1. validate context;
2. validate command shape;
3. validate player slot;
4. validate actor binding;
5. validate clock mode;
6. validate retry source for `Retry`;
7. validate target existence/activity;
8. validate targetability for command kind;
9. validate target point;
10. validate reach/range;
11. validate kind-specific final rules;
12. accept.

Pseudo-code shape:

```cpp
CommandAdmissionResult admitCommand(
    const CommandAdmissionContext& context,
    const CommandAdmissionRequest& request) {
  CommandRecord command = request.command;

  if (auto reason = validateContext(context); reason != None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateCommandShape(command); reason != None) {
    return rejectCommand(command, reason);
  }
  // Continue in documented order.
  return acceptCommand(command);
}
```

Do not compress the rule order into clever tables unless tests still make the
first-failure order obvious.

## Context Validation

Required checks:

- `context.world != nullptr`;
- `context.players != nullptr`;
- `context.clock != nullptr`;
- `context.commandLog != nullptr` when command kind is `Retry`;
- `context.config != nullptr` when distance/movement defaults require config.

Failure behavior:

- return `InternalError` for missing required runtime context;
- do not accept commands with missing required context;
- do not dereference null pointers.

Compute cost:

- O(1).

## Command Shape Validation

Required checks:

- `command.kind != CommandKind::None`;
- `command.admission == Pending` unless caller explicitly allows re-admission;
- command kind requiring actor has valid actor id;
- command kind requiring entity target has `payload.target.hasEntity == true`;
- command kind requiring point target has `payload.target.hasPoint == true`;
- command kind forbidding target does not rely on target data;
- retry command has `payload.retrySourceCommandId != kInvalidCommandId`.

Failure mapping:

- malformed kind/shape: `InvalidCommand`;
- missing retry source `commandId`: `RetrySourceMissing`;
- missing point for move: `InvalidTargetPoint`;
- missing entity for interact/inspect: `InvalidTarget`.

Compute cost:

- O(1).

## Player Slot Validation

Required checks:

- command kind needs player slot unless explicitly tool-owned;
- `playerSlot != kInvalidPlayerSlotId`;
- player slot exists in `PlayerRoster`;
- player slot kind is allowed to issue command if roster exposes slot kind.

Failure mapping:

- invalid or missing slot: `InvalidPlayerSlot`.

Compute cost:

- O(player count) unless roster exposes direct lookup.

## Actor Binding Validation

Required for:

- `Move`;
- `Interact`;
- `Inspect` if inspect requires actor;
- future combat commands.

Required checks:

- actor id is valid;
- actor exists in `WorldState`;
- actor is active/usable if world exposes active state;
- command player slot controls actor through `PlayerRoster`.

Failure mapping:

- missing/invalid actor: `InvalidActor`;
- actor exists but is not controlled by slot: `ActorNotControlledBySlot`.

Compute cost:

- O(entity count plus player count) in first build.

## Clock Validation

Required checks:

- paused mode allows only permitted commands;
- `StepTacticalTick` requires paused mode.

Lifecycle ownership:

- Session owns lifecycle gating before command admission.
- `Session` owns lifecycle gating before calling `CommandAdmission`.
- `CommandAdmissionContext` carries `ClockState`, not `SessionLifecycle`.
- `Session::submitCommand` rejects or routes commands when lifecycle is not
  playable before admission is called.
- `CommandAdmission` owns clock/pause-specific validation, reach/target
  validation, roster/actor binding, retry re-admission, and command-level
  rejection reasons available from its context.
- `CommandAdmission` never infers lifecycle from clock mode and never reads
  lifecycle from hidden/global state.

Paused behavior:

- `Resume`: allowed while paused;
- `StepTacticalTick`: allowed only while paused;
- `Reset`: allowed while paused;
- `Save`: allowed while paused;
- `Load`: allowed while paused;
- `Pause`: accepted idempotently if already paused;
- `Move`, `Interact`, `Inspect`, `Wait`, `ToggleTacticalMode`: rejected while
  paused.

Failure mapping:

- blocked due to paused state: `SessionPaused`;
- step while not paused: `StepRequiresPaused`.

Compute cost:

- O(1).

## Retry Validation And Reconstruction

For `CommandKind::Retry`, this source must implement retry as re-admission of
the original command intent under current state.

Required lookup:

1. find source command by `payload.retrySourceCommandId` in `CommandLog`;
2. verify source exists;
3. verify source admission is `Rejected`;
4. verify source kind is retryable.

Failure mapping:

- source `commandId` invalid or missing: `RetrySourceMissing`;
- source not rejected: `RetrySourceNotRejected`;
- source kind unsupported: `RetryUnsupportedKind`.

Unsupported source kinds:

- `Retry`;
- `Reset`;
- `Save`;
- `Load`;
- `None`.

Reconstruction rules:

- effective validation inside admission reads original rejected command
  kind/payload/actor/target/point/interaction intent from the source rejected
  command;
- retry record keeps its own `commandId`, sequence, submitting player slot,
  admission status, rejection reason, and retry metadata in the returned
  accepted/rejected retry command;
- admission does not create a stored normalized command record;
- do not mutate command log;
- do not execute source command in admission;
- do not erase the original rejection record;
- if original intent now passes, accept the retry command;
- session execution dispatches a normalized effective command intent resolved by
  `SessionTick`.

First complete retry execution model:

- `CommandAdmission` accepts or rejects the `Retry` command record only.
- An accepted retry record remains `CommandKind::Retry` in the command log.
- The returned retry command record keeps the retry `commandId`, sequence,
  submitting player slot, admission status, rejection reason, and
  `CommandPayload::retrySourceCommandId`.
- Admission never calls `InteractionSystem`, `MovementSystem`, or any gameplay
  mutation.
- `SessionTick` resolves an accepted retry into an `EffectiveCommandIntent`
  copied from the original rejected command.
- For `cmd_retry_key`, the effective command kind is `Interact`; the executed
  command has the retry `commandId` so events, results, and log linkage point at
  `cmd_retry_key`.
- `EffectiveCommandIntent::sourceCommandId` stores the accepted retry
  `commandId`.
- `EffectiveCommandIntent::retrySourceCommandId` stores the original rejected
  interact `commandId`.
- Subsystems receive the normalized effective command, not the raw `Retry`
  command.

Implementation approach:

- create a local `CommandRecord effective = sourceCommand`;
- read source command kind, payload, actor, target, point, and interaction intent
  from `effective`;
- validate `effective` against the current world, roster, clock, target, and
  reach context;
- validate `effective` through normal rules;
- return accepted/rejected result for the retry command with the same rejection
  reason the effective command produced.

SessionTick overwrite contract:

- `SessionTick` later creates `EffectiveCommandIntent` by copying the original
  rejected command's executable intent fields and overwriting exactly these
  `CommandRecord` fields from the accepted retry record: `commandId`, `sequence`,
  `playerSlot`, `admission`, and `rejection`.
- `sourceCommandId` and `retrySourceCommandId` are fields on
  `EffectiveCommandIntent`, not fields on `CommandRecord`.
- `EffectiveCommandIntent::sourceCommandId` is the accepted retry `commandId`.
- `EffectiveCommandIntent::retrySourceCommandId` is the original rejected
  `commandId`.
- No actor, target, target point, interaction id, command kind, or executable
  payload fields are overwritten from the retry command.
- The effective interact for `cmd_retry_key` keeps original actor, target, and
  interaction payload from `cmd_interact_oob`, while events/results use the retry
  `commandId` and retry source linkage.

Acceptance requirement:

- source `cmd_interact_oob` rejected as `OutOfRange`;
- after `cmd_move_to_key`, retry must pass reach and be accepted.
- `cmd_retry_key` must not execute during admission and must execute exactly once
  later through `SessionTick` as the original interact intent.

Compute cost:

- O(command count) source lookup unless `CommandLog` provides `commandId`
  lookup.

## Target Existence And Activity

Required for:

- `Interact`;
- `Inspect`;
- future combat/entity-targeted commands;
- retry of entity-targeted commands.

Required checks:

- target entity id is valid;
- target exists in `WorldState`;
- target is active if command requires active target.

Failure mapping:

- invalid/missing target: `InvalidTarget`;
- inactive target: `TargetInactive`.

Compute cost:

- O(entity count).

## Targetability Validation

Required checks:

- target entity kind supports the command kind;
- pickup supports `Interact` and `Inspect`;
- marker supports tactical movement target only if represented as entity target;
- self-target is rejected unless command explicitly allows it;
- inactive target rules are already handled before this stage.

Failure mapping:

- unsupported target for command: `InvalidTarget`.

Implementation calls read-only `TargetQuery` helpers for targetability facts.
Admission still owns the final rejection reason.

Compute cost:

- O(1) after target lookup, unless delegated query scans.

## Target Point Validation

Required for:

- `Move`;
- future point-targeted tactical commands.

Required checks:

- point is present;
- point coordinates are finite;
- point is within configured world bounds if bounds exist.

Failure mapping:

- absent or invalid point: `InvalidTargetPoint`.

Do not use renderer coordinates or screen-space picking data here.

Compute cost:

- O(1).

## Reach And Range Validation

Required for:

- `Interact`;
- future melee/combat/use commands if reach-gated.

Required checks:

- actor exists;
- target exists;
- reach query computes distance from actor transform position to target entity
  transform position;
- distance <= configured interaction range.

Failure mapping:

- range failure: `OutOfRange`;
- future nav/path impossible case: `TargetNotReachable`.

Acceptance-sensitive case:

```text
player at (0,0,0), gold_key at (3,0,0), range 1.500 -> OutOfRange
```

Do not return `InvalidTarget`, `TargetNotReachable`, or prose-only diagnostics
for this case.

Compute cost:

- O(entity lookup plus constant math).

## Kind-Specific Final Rules

### `Move`

Final rules:

- admission owns one-command movement distance legality for admitted `Move`
  commands;
- compute distance from actor transform position to command target point;
- distance must be within `RuntimeConfig::movementDistanceMeters`;
- too-far `Move` commands reject during admission with
  `CommandRejectionReason::MovementTooFar`;
- `MovementSystem` still guards internally and returns
  `MovementBlockedReason::MovementTooFar` only for direct MovementSystem calls,
  replay/load/internal invariant failures, or tests that bypass admission;
- acceptance path admits `(2,0,0)` and `(2,0,1)`.

Failure mapping:

- too far: `MovementTooFar`.

### `Interact`

Final rules:

- target interaction kind exists;
- target is not consumed/inactive;
- reach passed.

Failure mapping:

- unsupported target: `InvalidTarget`;
- out of reach: `OutOfRange`.

Range ownership:

- `CommandAdmission` reads `RuntimeConfig::interactionRangeMeters`.
- The value must be finite and greater than `0.0f`; otherwise admission returns
  `InternalError` before calling `ReachQuery`.
- Admission passes that explicit value into `ReachQueryRequest::maxRangeMeters`.
- `ReachQuery` does not read `RuntimeConfig`, does not apply defaults, and does
  not silently repair a non-positive range.
- The first-room acceptance config value is exactly `1.500`.

### `Inspect`

Final rules:

- target inspectable.

Inspect does not mutate state after acceptance.

### `Wait`

Final rules:

- no target required;
- accepted only in allowed clock/session modes.

### `ToggleTacticalMode`

Final rules:

- no target required;
- accepted in normal/slow mode;
- rejected while paused.

### `Pause`

Final rules:

- no target required;
- accepted in normal/slow mode;
- idempotent accepted while already paused.

### `Resume`

Final rules:

- no target required;
- accepted while paused;
- accepted idempotently outside paused with no state change.

### `StepTacticalTick`

Final rules:

- no target required;
- accepted only while paused.

### `Retry`

Final rules:

- source command is retryable;
- effective original intent passes current validation.

### `Reset`

Final rules:

- baseline exists, if admission can inspect that;
- otherwise `Session` performs final reset availability check.

### `Save`

Final rules:

- save service/path is not inspected here;
- admission only says runtime state may be saved.

### `Load`

Final rules:

- decoded envelope availability/compatibility may be checked by save/load layer;
- admission does not inspect file path.

## Acceptance Demo Algorithm

Admission must support this sequence:

1. `cmd_interact_oob`: actor and target valid, reach fails, reject
   `OutOfRange`.
2. `cmd_move_to_key`: valid actor and finite point, accept.
3. `cmd_retry_key`: source rejected, retryable, current reach passes, accept.
4. `cmd_enter_tactical`: normal mode, accept.
5. `cmd_tactical_move`: slow mode, valid point, accept.
6. `cmd_pause`: slow mode, accept.
7. `cmd_step`: paused mode, accept.
8. `cmd_resume`: paused mode, accept.
9. `cmd_wait`: slow mode after resume, not paused, accept.
10. `cmd_exit_tactical`: slow mode, accept.

If any command in this sequence admits differently, the acceptance demo fails.

## Save Replay Multiplayer Notes

Save/load:

- this source sets `admission` and `rejection` fields that must be persisted in
  `CommandRecord`;
- it must not store diagnostics as the only source of rejection truth.

Replay:

- running the same command at the same logical point must return the same
  accepted/rejected status;
- rejected replay command must reproduce exact rejection reason;
- retry must depend on current replay state, not on final saved state.

Multiplayer:

- decoded remote/local multiplayer commands must run through this same
  implementation after authority passes;
- rejection order after authority must be deterministic and slot-independent
  except for actor binding and permissions;
- this source must not know about sockets or transport.

## Diagnostics And Errors

This file returns rejection enum values. It may also create diagnostic details
only if the header/API allows it, but diagnostics are secondary.

Required rejection values:

- `OutOfRange` for first acceptance interaction;
- `StepRequiresPaused` for step while not paused;
- `RetrySourceMissing` for missing retry source;
- `RetrySourceNotRejected` for accepted/non-rejected retry source;
- `RetryUnsupportedKind` for nonretryable source;
- `InvalidTargetPoint` for invalid move point;
- `ActorNotControlledBySlot` for slot/actor mismatch.

Do not encode behavior only in log text.

## Compute Cost

Worst-case baseline costs:

- context/shape/session checks: O(1);
- player slot lookup: O(player count);
- actor lookup: O(entity count);
- target lookup: O(entity count);
- reach: O(entity lookup plus constant math);
- retry source lookup: O(command count);
- total retry of entity-targeted command: O(command count plus entity count).

The first-room acceptance fixture is tiny, so no index is required. Future
indexes must preserve first-failure order and replay determinism.

## Tests And Verification

Covered by:

- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/target_reach_tests.cpp`;
- `tests/unit/session_state_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required implementation tests:

- `rejectCommand` never returns rejection `None`;
- `acceptCommand` always clears rejection to `None`;
- context nulls do not crash;
- rule order returns the first expected failure;
- invalid actor fails before target checks;
- invalid target fails before reach checks;
- initial valid actor/target out-of-range interact returns `OutOfRange`;
- retry missing source `commandId` returns `RetrySourceMissing`;
- retry accepted source returns `RetrySourceNotRejected`;
- retry unsupported source returns `RetryUnsupportedKind`;
- retry original interact after movement returns accepted;
- admission does not mutate world, clock, camera, command log, inventory, or
  objective state.

## Completion Criteria

- `src/runtime/command/CommandAdmission.cpp` exists in `/Users/kogaryu/iggy3d`.
- It includes `CommandAdmission.hpp` first.
- It implements every public API declared by the header.
- It applies the exact first-failure order.
- It returns `OutOfRange` for the first acceptance interaction.
- It accepts retry after movement into range.
- It is read-only with respect to runtime state.
- It does not append to command log.
- It does not execute commands.
- It has no app, projection, renderer, tests, sockets, or old `iggy`
  dependency.
