# `src/runtime/replay/CommandLog.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic command history for admission results,
retry lookup, diagnostics, save/load, replay, summary counts, state-hash input,
and future multiplayer ordering.

## Build Position

- priority rank: 61
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/replay`
- file kind: `header`

This header defines the command log data model and public API. It does not
admit, execute, or replay commands by itself. It stores what happened so other
systems can prove determinism.

## Ownership

This file owns:

- ordered command record storage;
- next sequence policy;
- append request/result value shape;
- command count summary fields;
- lookup by `commandId`;
- rejected-command lookup for retry;
- reset/epoch policy declarations;
- command-log save truth declaration.

It must not own:

- command admission rules;
- authority policy;
- gameplay execution;
- target/reach queries;
- movement or interaction effects;
- save codec encoding;
- replay execution loop;
- state hashing implementation;
- raw input events;
- renderer identifiers;
- network sockets;
- old `/Users/kogaryu/iggy` adapters.

## Required Header Shape

The implementation file must be:

```text
src/runtime/replay/CommandLog.hpp
```

Required include style:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "runtime/command/Command.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

This header must not include `SessionState`, systems, app code, projection,
renderer, socket, or old `iggy` headers.

## Required Enums

### `CommandLogResetPolicy`

Declare:

```cpp
enum class CommandLogResetPolicy : std::uint8_t {
  Clear,
  NewEpoch,
};
```

First complete build reset policy:

- `Clear` for reset branch proof.
- `NewEpoch` is declared as a reserved enum value only and is not used by the
  first complete build.

## Required Value Types

### `CommandLogCounts`

Declare:

```cpp
struct CommandLogCounts {
  std::uint64_t submitted = 0;
  std::uint64_t accepted = 0;
  std::uint64_t rejected = 0;
  std::uint64_t retry = 0;
  std::uint64_t movement = 0;
  std::uint64_t interaction = 0;
  std::uint64_t control = 0;
};
```

Semantics:

- `submitted` counts all records appended to gameplay command log;
- `accepted` counts records with admission `Accepted`;
- `rejected` counts records with admission `Rejected`;
- `retry` counts command kind `Retry`;
- `Move` increments movement;
- `Interact` increments interaction by command kind;
- `Retry` increments retry only and does not increment movement, interaction, or
  control;
- `ToggleTacticalMode`, `Pause`, `Resume`, and `StepTacticalTick` increment
  control;
- `Wait` contributes to submitted/accepted but does not increment control in the
  first complete build;
- counts are derived from stored records and are not saved as cached values.

Acceptance expected counts:

- submitted: `10`;
- accepted: `9`;
- rejected: `1`;
- retry: `1`.

### `CommandLogAppendStatus`

Declare:

```cpp
enum class CommandLogAppendStatus : std::uint8_t {
  Ok,
  InvalidCommandId,
  DuplicateCommandId,
  InvalidPlayerSlot,
  InvalidKind,
  InvalidSequence,
  InvalidAdmissionState,
  InvalidRetrySource,
  InvalidCommandPayload,
};
```

### `CommandLogAppendResult`

Declare:

```cpp
struct CommandLogAppendResult {
  CommandLogAppendStatus status = CommandLogAppendStatus::Ok;
  CommandRecord record;
  std::size_t index = 0;
  CommandSequence sequence = kInvalidCommandSequence;
  std::uint64_t epoch = 0;
  CommandId commandId = kInvalidCommandId;
  std::string diagnostic;
};
```

Semantics:

- `Ok` is the only success status;
- on `Ok`, `record` is the final stored command record after sequence
  assignment;
- on `Ok`, `index` is the stable vector index for diagnostics only;
- on `Ok`, `sequence`, `epoch`, and `commandId` mirror the stored record/log
  facts after append;
- on non-`Ok`, no record is appended, `nextSequence_` is not incremented,
  `epoch_` is unchanged, and `diagnostic` names the failed invariant;
- vector index is not durable command-record identity and must not replace
  `commandId`.

### `CommandLogFindResult`

Declare:

```cpp
struct CommandLogFindResult {
  const CommandRecord* record = nullptr;
  std::size_t index = 0;
};
```

Semantics:

- null record means not found;
- returned pointer/reference is read-only;
- callers must not store pointer across log mutation if vector storage can
  reallocate.

The first complete build uses the pointer-plus-index shape shown above. The
not-found state is `record == nullptr`.

### `CommandLogRestoreStatus`

Declare:

```cpp
enum class CommandLogRestoreStatus : std::uint8_t {
  Restored,
  DuplicateCommandId,
  InvalidCommandId,
  InvalidSequence,
  NonMonotonicSequence,
  InvalidAdmissionState,
  InvalidRetrySource,
  InvalidNextSequence,
};
```

### `CommandLogRestoreResult`

Declare:

```cpp
struct CommandLogRestoreResult {
  CommandLogRestoreStatus status = CommandLogRestoreStatus::Restored;
  std::size_t recordIndex = 0;
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
};
```

Semantics:

- `Restored` is the only success status;
- `recordIndex`, `commandId`, and `sequence` identify the first invalid record
  when restoration fails;
- `InvalidNextSequence` reports the supplied cursor rather than an individual
  record when records are otherwise valid.

## Required `CommandLog` Type

Declare:

```cpp
class CommandLog {
public:
  CommandLog();

  const std::vector<CommandRecord>& records() const;
  bool empty() const;
  std::size_t size() const;

  CommandSequence nextSequence() const;

  CommandLogAppendResult append(const CommandRecord& record);
  CommandLogRestoreResult restoreForLoad(std::vector<CommandRecord> records,
                                         CommandSequence nextSequence,
                                         std::uint64_t epoch);
  void clear();
  void reset(CommandLogResetPolicy policy);

  CommandLogFindResult findById(CommandId commandId) const;
  CommandLogFindResult findRejectedById(CommandId commandId) const;
  const CommandRecord* latestRejected() const;

  CommandLogCounts counts() const;

private:
  std::vector<CommandRecord> records_;
  CommandSequence nextSequence_;
  std::uint64_t epoch_;
};
```

The stored fields are exactly `records_`, `nextSequence_`, and `epoch_` for the
first complete build. Command identity allocation belongs to
`SessionState::nextCommandId`, not `CommandLog`.

## ID And Sequence Policy

First complete build:

- `Session` command construction assigns stable nonzero
  `CommandRecord::commandId` before lifecycle prefiltering and before
  `CommandAdmission`;
- `CommandLog::append` returns `InvalidCommandId` for invalid `commandId`;
- `CommandLog::append` returns `DuplicateCommandId` for duplicate `commandId`
  values within one session/log;
- sequence is always assigned by `CommandLog::append` for normal runtime append;
- caller-supplied non-invalid sequence is invalid for normal append and returns
  `InvalidSequence`;
- sequence begins at `1`;
- sequence increments monotonically within the log epoch;
- `commandId == 0` and sequence `0` remain invalid.

Identity semantics:

- accepted and rejected commands are both appended with stable `commandId`
  values;
- lifecycle-prefiltered rejections are appended with stable `commandId` values;
- `CommandAdmission` never assigns `commandId` and never assigns sequence;
- replay order is sequence-driven;
- retry source lookup is `commandId`-driven.

Sequence semantics:

- sequence is the canonical command-log order;
- vector storage order must match sequence order;
- replay iterates by sequence/vector order;
- stable ordering must not depend on unordered containers.

## Append Semantics

`append(const CommandRecord& record)` must:

1. require `record.commandId != kInvalidCommandId`;
2. return `DuplicateCommandId` if `record.commandId` already exists in this log;
3. require `record.sequence == kInvalidCommandSequence`;
4. return `InvalidSequence` for any caller-supplied sequence;
5. assign `nextSequence_` to the stored copy;
6. require `admission` to be `Accepted` or `Rejected`;
7. require rejected commands to carry non-`None` rejection reason;
8. require accepted commands to carry `None` rejection reason;
9. push record onto `records_`;
10. advance `nextSequence_` to exactly the stored sequence plus `1`;
11. return `CommandLogAppendStatus::Ok` with stored record, index, sequence,
    epoch, and command id.

It must not:

- run command admission;
- execute the command;
- mutate world/session/subsystems;
- generate renderer output;
- write save files;
- read wall-clock time.

Malformed append behavior:

- first complete build uses the recoverable `CommandLogAppendResult` status in
  release; debug builds also assert on malformed append input;
- every non-`Ok` append result leaves `records_`, `nextSequence_`, and `epoch_`
  unchanged;
- no release path may normalize or repair admission, rejection, command id,
  sequence, player slot, target, retry source, or tick fields;
- never store an accepted command with non-`None` rejection;
- never store a rejected command with `None` rejection.

Append status mapping:

- invalid command id returns `InvalidCommandId`;
- duplicate command id returns `DuplicateCommandId`;
- invalid player slot returns `InvalidPlayerSlot`;
- `CommandKind::None` or unsupported command kind returns `InvalidKind`;
- caller-supplied sequence, zero sequence, sequence overflow, or sequence cursor
  corruption returns `InvalidSequence`;
- pending admission, accepted-with-rejection, or rejected-without-rejection
  returns `InvalidAdmissionState`;
- retry with invalid `payload.retrySourceCommandId` returns
  `InvalidRetrySource`;
- malformed target flags, missing required actor, invalid target payload, or
  invalid issued/scheduled tick facts return `InvalidCommandPayload`.

## Find And Retry Semantics

`findById`:

- returns any record by `commandId`;
- O(command count) in first build;
- returns not-found explicitly.

`findRejectedById`:

- returns record only if found and admission is `Rejected`;
- used by retry admission;
- must not return accepted records.

`latestRejected`:

- scans from back to front;
- returns most recently appended rejected command;
- useful for UI/tool retry, but acceptance retry uses explicit source
  `commandId`.

Retry contract:

- retry stores its own command record;
- retry references rejected source `commandId`;
- original rejected command remains in log;
- command log does not decide whether retry is legal;
- `CommandAdmission` decides retry legality using log lookup.

## Reset And Epoch Semantics

First complete build reset policy:

```text
CommandLogResetPolicy::Clear
```

Low-level `clear()` behavior:

- remove all records;
- reset next sequence to `1`;
- leave `epoch_` unchanged.

`reset(CommandLogResetPolicy::Clear)` behavior:

- perform low-level `clear()` behavior;
- increment `epoch_` by `1`;
- reset branch baseline summary reports empty command log.

`NewEpoch` is reserved for a later build and is not used by first complete
runtime acceptance.

## Counts Semantics

`counts()` derives from stored records every time in the first complete build.

Command kind classification:

- `Move`: movement count;
- `Interact`: interaction count;
- `Retry`: retry count only; effective interaction caused by `Retry` does not
  add a second command-log `Interact` count;
- `ToggleTacticalMode`, `Pause`, `Resume`, and `StepTacticalTick`: control
  count;
- `Wait`: submitted/accepted only and not control in the first complete build;
- save/load/reset proof phases are excluded from the first-build gameplay
  command log.

Acceptance summary requires:

```text
commands.submitted=10
commands.accepted=9
commands.rejected=1
commands.retry=1
```

No separate wait field exists in `CommandLogCounts`. The first-build runtime
summary does not print wait count; no-op wait count is derivable from command
records.

## Save Load Semantics

Command log is save truth.

Save includes exactly the log-owned fields:

- records;
- next sequence;
- epoch.

Counts are derived from stored records and are not saved as cached values.

Save includes for each command:

- `commandId`;
- sequence;
- player slot;
- actor `EntityId`;
- kind;
- source if replay/diagnostics require it;
- target entity flag and `EntityId`;
- target point flag and point;
- retry source `commandId`;
- issued tick;
- scheduled tick;
- admission status;
- rejection reason.

Load must restore through `restoreForLoad`, not through normal `append` or ad
hoc mutation:

- record order;
- `commandId` values;
- sequences;
- admission status;
- rejection reasons;
- retry source links;
- next sequence;
- epoch.

`restoreForLoad(std::vector<CommandRecord> records, CommandSequence
nextSequence, std::uint64_t epoch)` must validate all input before mutation,
then atomically replace `records_`, `nextSequence_`, and `epoch_`.

Restore must reject deterministically before mutation:

- duplicate `commandId` values;
- invalid `commandId` values;
- non-monotonic sequences;
- zero, duplicate, skipped, or invalid sequences;
- accepted record with rejection reason;
- rejected record with `None` reason;
- retry referencing a missing command;
- retry referencing a command that appears later in the vector;
- retry referencing a command that was not rejected;
- `nextSequence` not equal to `lastSequence + 1`, or not equal to `1` when
  records are empty.

Restore status mapping:

- duplicate command id returns `DuplicateCommandId`;
- invalid command id returns `InvalidCommandId`;
- invalid zero sequence returns `InvalidSequence`;
- out-of-order, duplicate, or skipped sequence returns `NonMonotonicSequence`;
- invalid admission/rejection pairing returns `InvalidAdmissionState`;
- invalid retry source returns `InvalidRetrySource`;
- invalid `nextSequence` cursor returns `InvalidNextSequence`.

Failed append or load validation leaves the destination log/session unchanged.

## Replay Semantics

Replay consumes command log records in deterministic order.

Replay requirements:

- rejected records are replayed through admission and must reject for same
  reason;
- accepted records are replayed through admission/session execution and must
  accept;
- retry records must find their source rejected command in replay log state;
- sequence order is the replay order;
- replay must report first divergence by sequence and `commandId`.

Command log does not execute replay. `CommandReplay` owns replay loop.

## State Hash Semantics

The first complete build includes command log state in `StateHash`:

- hash records in sequence order;
- include all save-truth command fields;
- exclude vector capacity and memory addresses;
- exclude debug labels;
- include next sequence and epoch.

`SaveCommandLogSection.resetPolicy` is a save schema constant that encodes
`Clear`; it is not mutable `CommandLog` state.

## Multiplayer Semantics

Command log must support future multiplayer:

- each command stores `playerSlot`;
- command source may distinguish local, AI, replay, remote, tool;
- sequence order is global deterministic order after local multiplayer merge;
- per-slot sequence can be added later in command payload or replication packet;
- log storage must not assume one player except acceptance fixture setup.

The log must not own sockets, remote transport, matchmaking, or authority
policy.

## Diagnostics And Errors

Command log supports diagnostics by exposing:

- record count;
- command counts;
- first rejected command;
- latest rejected command;
- find by `commandId`;
- find rejected by `commandId`;
- stable index/sequence for mismatch reporting.

Diagnostics text is not stored as command truth. Rejection enum values are.

## Compute Cost

Baseline first build:

- append: amortized O(1);
- find by `commandId`: O(command count);
- find rejected by `commandId`: O(command count);
- latest rejected: O(command count) worst case;
- counts: O(command count);
- clear/reset: O(command count) to destroy records;
- save mapping: O(command count);
- replay scan: O(command count), owned by `CommandReplay`.

Future `commandId` indexes may be added only if:

- vector order remains canonical;
- index is derived/rebuildable;
- save truth does not depend on unordered traversal;
- replay and state hash remain deterministic.

## Tests And Verification

Covered by:

- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/unit/save_load_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- new log is empty;
- first append preserves nonzero `commandId` supplied by `Session` and assigns
  sequence `1` when unset;
- append rejects or asserts invalid `commandId`;
- append rejects or asserts duplicate `commandId`;
- rejected commands append with stable nonzero `commandId`;
- lifecycle-prefiltered rejected commands append with stable nonzero `commandId`;
- append preserves accepted/rejected admission status;
- append rejects invalid rejection/status combinations before mutation;
- `findById` finds accepted and rejected records;
- `findRejectedById` returns only rejected records;
- `latestRejected` returns most recent rejected record;
- retry source lookup finds `cmd_interact_oob`;
- retry source lookup uses `commandId`, not sequence or vector index;
- counts match ten-command acceptance sequence;
- reset clear policy restores empty log and initial counters;
- save/load round-trip preserves records and next sequence;
- replay scans in sequence order;
- no method mutates world/session/subsystems.

## Completion Criteria

- `src/runtime/replay/CommandLog.hpp` exists in `/Users/kogaryu/iggy3d`.
- It declares `CommandLogResetPolicy`.
- It declares `CommandLogCounts`.
- It declares append/find result types.
- It declares `CommandLog` with append, find, rejected lookup, latest rejected,
  reset, records, and counts APIs.
- It supports explicit retry source lookup.
- It can represent the ten acceptance demo commands and expected counts.
- It separates command storage from admission, execution, save codec, replay
  loop, multiplayer transport, and renderer.
- It has no app, projection, renderer, socket, test, or old `iggy` dependency.
