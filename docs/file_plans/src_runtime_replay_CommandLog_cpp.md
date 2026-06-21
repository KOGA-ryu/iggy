# `src/runtime/replay/CommandLog.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic command history storage for admission
results, retry lookup, diagnostics, save/load, replay, summary counts,
state-hash input, and future multiplayer ordering.

## Build Position

- priority rank: 62
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/replay`
- file kind: `source`

This file implements the storage and query behavior declared by
`CommandLog.hpp`. It must be deterministic, side-effect limited, and easy to
audit because replay, retry, save/load, and acceptance summary all depend on it.

## Ownership

This file owns implementation of:

- `CommandLog` construction;
- `records`;
- `empty`;
- `size`;
- `nextSequence`;
- `append`;
- `restoreForLoad`;
- `clear`;
- `reset`;
- `findById`;
- `findRejectedById`;
- `latestRejected`;
- `counts`;
- any private invariant helpers used by those functions.

It must not own:

- command admission;
- command execution;
- command replay loop;
- state hashing algorithm;
- save codec serialization;
- authority policy;
- gameplay mutation;
- app CLI behavior;
- renderer resources;
- network sockets;
- old `/Users/kogaryu/iggy` adapters.

## Required Include Order

Implementation must include paired header first:

```cpp
#include "runtime/replay/CommandLog.hpp"
```

Then include only minimal standard headers:

```cpp
#include <algorithm>
#include <cassert>
```

Only include more if directly required. This file should not need world,
session, movement, interaction, app, projection, renderer, socket, or old `iggy`
headers.

## Constructor Implementation

`CommandLog::CommandLog()` must initialize:

- empty records vector;
- `nextSequence_ = 1`;
- `epoch_ = 0`.

`CommandLog` does not own `commandId` allocation.

Constructor must not:

- allocate unnecessary records;
- read files;
- read time;
- inspect runtime state;
- use global mutable state.

## Accessor Implementation

### `records()`

Return a const reference to canonical record storage.

Rules:

- vector order is sequence order;
- caller must not mutate returned records;
- caller must not rely on pointer stability after future append/reset.

Cost:

- O(1).

### `empty()`

Return `records_.empty()`.

Cost:

- O(1).

### `size()`

Return `records_.size()`.

Cost:

- O(1).

### `nextSequence()`

Return `nextSequence_`.

Cost:

- O(1).

## Append Implementation

`append(const CommandRecord& record)` is the only normal write path for command
history.

Required algorithm:

1. validate `record.admission`;
2. validate `record.rejection`;
3. require `record.commandId != kInvalidCommandId`;
4. reject before mutation if `record.commandId` already exists in `records_`;
5. require `record.sequence == kInvalidCommandSequence`;
6. reject before mutation for any caller-supplied sequence;
7. validate player slot, command kind, retry source, target payload, and tick
   facts that are structural command-record invariants;
8. copy the input record, assign `sequence = nextSequence_`, and append the copy
   to `records_`;
9. advance `nextSequence_` to exactly the stored sequence plus `1`;
10. return `CommandLogAppendStatus::Ok` with stored record, index, sequence,
    epoch, and command id.

Required pseudo-code shape:

```cpp
CommandLogAppendResult CommandLog::append(const CommandRecord& record) {
  if (record.commandId == kInvalidCommandId) {
    return appendFailure(CommandLogAppendStatus::InvalidCommandId, record);
  }
  if (containsCommandId(record.commandId)) {
    return appendFailure(CommandLogAppendStatus::DuplicateCommandId, record);
  }
  if (record.sequence != kInvalidCommandSequence) {
    return appendFailure(CommandLogAppendStatus::InvalidSequence, record);
  }
  if (!isValidAdmissionInvariant(record)) {
    return appendFailure(CommandLogAppendStatus::InvalidAdmissionState, record);
  }
  if (!isValidCommandPayload(record)) {
    return appendFailure(payloadFailureStatus(record), record);
  }

  CommandRecord stored = record;
  stored.sequence = nextSequence_;

  const std::size_t index = records_.size();
  records_.push_back(stored);

  nextSequence_ = stored.sequence + 1;

  return CommandLogAppendResult{
      CommandLogAppendStatus::Ok,
      records_.back(),
      index,
      records_.back().sequence,
      epoch_,
      records_.back().commandId,
      Diagnostic{}};
}
```

Use explicit overflow handling if sequences reach maximum value. Overflow should
return `InvalidSequence` before mutation in release and assert in debug.

## Admission Invariant Validation

Append must enforce:

- accepted command has `rejection == None`;
- rejected command has `rejection != None`;
- pending command must not be appended.

- pending append is invalid and returns `InvalidAdmissionState` before mutation
  in release and asserts in debug;
- accepted with non-`None` rejection is invalid;
- rejected with `None` rejection is invalid;
- no release path may normalize or repair admission, rejection, command id,
  sequence, retry source, player slot, target, or tick fields.

The exact behavior must be tested. Do not silently store inconsistent records,
and do not mutate the log on validation failure.

## Sequence Advancement

When appending a record:

- `commandId` must already be nonzero and unique in this log;
- sequence must be invalid/unset on input;
- normal append never accepts caller-supplied sequence;
- caller-supplied lower, current, skipped, duplicate, zero, and non-monotonic
  sequences return `InvalidSequence` before mutation in release and assert in
  debug;
- assign `nextSequence_` to the stored copy;
- after append, advance `nextSequence_` to exactly the stored sequence plus `1`.

Duplicate handling must be deterministic because save/load and replay depend on
it.

## Duplicate Detection

This source should provide a private helper:

```cpp
bool containsCommandId(CommandId commandId) const;
```

implemented as a local linear scan over `records_`.

Rules:

- invalid `commandId` returns `InvalidCommandId` before append;
- duplicate nonzero `commandId` returns `DuplicateCommandId`;
- duplicate append must not store a second record with the same `commandId`.

Normal append is for runtime/session append only. Load reconstruction must not
call `append` for decoded records with preserved sequences.

## `restoreForLoad()` Implementation

`restoreForLoad(std::vector<CommandRecord> records, CommandSequence
nextSequence, std::uint64_t epoch)` is the only command-log mutation path for
save/load restore.

Validation algorithm:

1. scan all records without mutating `records_`, `nextSequence_`, or `epoch_`;
2. require every `commandId` to be nonzero;
3. require command ids to be unique;
4. require sequence values to be strict increasing starting at `1` with no gaps;
5. require admission to be `Accepted` or `Rejected`;
6. require accepted records to carry rejection `None`;
7. require rejected records to carry non-`None` rejection;
8. require every retry source to reference an earlier rejected
   `CommandRecord::commandId`;
9. require `nextSequence == lastSequence + 1`, or `nextSequence == 1` when the
   record vector is empty;
10. after every validation succeeds, move the input vector into `records_`, set
    `nextSequence_ = nextSequence`, and set `epoch_ = epoch`;
11. return `CommandLogRestoreStatus::Restored`.

Failure mapping:

- duplicate command id returns `DuplicateCommandId`;
- invalid command id returns `InvalidCommandId`;
- zero or invalid sequence returns `InvalidSequence`;
- duplicate, skipped, or out-of-order sequence returns
  `NonMonotonicSequence`;
- invalid admission/rejection pairing returns `InvalidAdmissionState`;
- missing, later, or non-rejected retry source returns `InvalidRetrySource`;
- invalid `nextSequence` returns `InvalidNextSequence`.

Every failure returns before mutation and includes the first failing
`recordIndex`, `commandId`, and `sequence` when a record caused the failure.

## `clear()` Implementation

`clear()` must:

- remove all records;
- reset `nextSequence_` to `1`;
- leave `epoch_` unchanged.

Cost:

- O(command count).

## `reset()` Implementation

`reset(CommandLogResetPolicy policy)` must implement:

### `Clear`

- perform `clear()` behavior by removing all records and resetting
  `nextSequence_` to `1`;
- increment `epoch_` by exactly `1`;
- acceptance reset proof expects empty command log after reset when clear policy
  is used.

### `NewEpoch`

`NewEpoch` is reserved only. The first complete build must not expose
functional `NewEpoch` behavior. Calling it must assert in debug or return a
structured unsupported-policy failure in release without changing records,
`nextSequence_`, or `epoch_`.

`reset(CommandLogResetPolicy::Clear)` is the only acceptance reset path.

## `findById()` Implementation

Algorithm:

- scan `records_` from front to back;
- return first record with matching `commandId`;
- return null/not-found result if none.

Rules:

- invalid `commandId` returns not found;
- duplicate `commandId` values should not exist;
- if duplicates exist due to corrupted load, first match is returned but load
  validation should reject duplicates before normal use.

Cost:

- O(command count).

## `findRejectedById()` Implementation

Algorithm:

- call or mirror `findById`;
- return record only if admission is `Rejected`;
- otherwise return not found.

Rules:

- accepted records are not retry sources;
- pending records are not retry sources;
- this is the primary lookup used by retry admission.

Cost:

- O(command count).

## `latestRejected()` Implementation

Algorithm:

- scan `records_` from back to front;
- return first record whose admission is `Rejected`;
- return null if none.

Rules:

- useful for UI/tool retry convenience;
- acceptance retry uses explicit source `commandId`, not latest-rejected
  fallback;
- must not mutate log.

Cost:

- O(command count) worst case.

## `counts()` Implementation

Counts must be derived deterministically from `records_`.

Algorithm:

1. initialize `CommandLogCounts` to zero;
2. iterate records in vector order;
3. increment `submitted` for each record;
4. increment accepted/rejected from `admission`;
5. increment retry when `kind == Retry`;
6. increment movement when `kind == Move`;
7. increment interaction when `kind == Interact`;
8. increment control only for `ToggleTacticalMode`, `Pause`, `Resume`, and
   `StepTacticalTick`;
9. return counts.

Control kinds for first build:

- `ToggleTacticalMode`;
- `Pause`;
- `Resume`;
- `StepTacticalTick`;
- save/load/reset proof phases are excluded from the first-build gameplay
  command log.

`Retry` increments only `retry`. The effective interaction caused by accepted
retry execution does not increment command-log interaction. `Wait` contributes
only to submitted/accepted in the first complete build. The runtime summary does
not print a wait count.

Acceptance count target:

```text
submitted=10
accepted=9
rejected=1
retry=1
```

Cost:

- O(command count).

## Acceptance Demo Record Sequence

The command log must be able to store this sequence:

1. `cmd_interact_oob`: rejected `Interact`, reason `OutOfRange`;
2. `cmd_move_to_key`: accepted `Move`;
3. `cmd_retry_key`: accepted `Retry`, source `commandId` points to command 1;
4. `cmd_enter_tactical`: accepted `ToggleTacticalMode`;
5. `cmd_tactical_move`: accepted `Move`;
6. `cmd_pause`: accepted `Pause`;
7. `cmd_step`: accepted `StepTacticalTick`;
8. `cmd_resume`: accepted `Resume`;
9. `cmd_wait`: accepted `Wait`;
10. `cmd_exit_tactical`: accepted `ToggleTacticalMode`.

Expected:

- vector order equals listed order;
- sequence values are strictly increasing;
- `commandId` values are nonzero and unique;
- command 3 retry source references command 1 `commandId`;
- `findRejectedById(command1.commandId)` returns command 1;
- `latestRejected()` returns command 1 until another rejection is appended;
- `counts()` returns acceptance summary counts.

## Save Load Implementation Notes

This source does not encode save files. It must preserve enough state for
`SaveCodec` and `SaveLoad`.

Load reconstruction uses an explicit validation/reconstruction path:

- call `restoreForLoad(decodedRecords, decodedNextSequence, decodedEpoch)`;
- reject malformed records before mutation;
- preserve decoded `commandId` values and sequences exactly;
- set `nextSequence_` and `epoch_` exactly from validated decoded values.

Save/load code must use `restoreForLoad` and must not call normal runtime
`append` for decoded command records.

## Replay Implementation Notes

Replay iterates `records()` in order.

This source must ensure:

- order is stable;
- records are immutable through public read API;
- accepted/rejected status is stored exactly;
- retry source lookup works during replay;
- no hidden filtering removes rejected records.

Rejected records are not executed by command log. Replay/session code decides
how to re-admit and compare them.

## State Hash Implementation Notes

This source does not hash state.

`StateHash` includes command log state and must:

- iterate `records()` in canonical vector order;
- include all save-truth command fields;
- include `nextSequence_`;
- include `epoch_`;
- exclude vector capacity and pointer values.

This implementation must avoid nondeterministic ordering so hashing remains
stable.

## Multiplayer Implementation Notes

Global order:

- this log stores already-merged command order;
- local multiplayer merge happens before append;
- each record retains player slot/source for authority and replay diagnostics.

This source must not:

- sort commands by unordered map state;
- know about network sockets;
- know about packet bytes;
- override authority decisions.

## Diagnostics And Errors

This file supports diagnostics by preserving command facts.

Required diagnostic facts:

- record index;
- `commandId`;
- sequence;
- player slot;
- command kind;
- admission;
- rejection reason;
- retry source `commandId` when present.

Do not store prose diagnostics as command truth.

## Compute Cost

Baseline first build:

- constructor/accessors: O(1);
- append: amortized O(1) plus O(command count) duplicate scan if enabled;
- find by `commandId`: O(command count);
- find rejected by `commandId`: O(command count);
- latest rejected: O(command count);
- counts: O(command count);
- clear/reset: O(command count);

Future index optimization may add derived `commandId` lookup:

- index must be rebuildable from records;
- index must not become save truth unless explicitly documented;
- vector order remains canonical.

## Tests And Verification

Covered by:

- `tests/unit/command_admission_tests.cpp`;
- `tests/unit/replay_state_hash_tests.cpp`;
- `tests/unit/save_load_tests.cpp`;
- `tests/acceptance/complete_runtime_demo_tests.cpp`.

Required assertions:

- constructor creates empty log with next sequence `1`;
- append requires nonzero `commandId` supplied by `Session`;
- append assigns sequence when missing;
- append rejects/asserts duplicate `commandId`;
- append stores accepted, rejected, and lifecycle-prefiltered rejected records
  with stable nonzero `commandId`;
- accepted append stores rejection `None`;
- rejected append stores non-`None` reason;
- pending append is not silently stored as valid;
- find by `commandId` returns correct record;
- find rejected by `commandId` ignores accepted record;
- retry source lookup uses `commandId`, not sequence or vector index;
- latest rejected scans backward;
- counts match acceptance command sequence;
- clear/reset clear records and reset counters under clear policy;
- no method mutates world/session/subsystem state;
- no method uses wall-clock time or random state.

## Completion Criteria

- `src/runtime/replay/CommandLog.cpp` exists in `/Users/kogaryu/iggy3d`.
- It includes `CommandLog.hpp` first.
- It implements every public API declared by the header.
- It requires deterministic `commandId` values and assigns deterministic
  sequences.
- It preserves accepted and rejected command records.
- It supports rejected command lookup for retry.
- It returns acceptance command counts.
- It supports clear reset policy.
- It has no app, projection, renderer, socket, test, or old `iggy`
  dependency.
