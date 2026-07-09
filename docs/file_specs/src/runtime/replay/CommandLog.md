# Command Log

File:

- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandLog.cpp`

Verified at: `2b549d47`

## Owns

- Runtime command log storage for accepted and rejected `CommandRecord` rows.
- Command log append, restore-for-load, find-by-id, clear/reset, sequence, epoch, and aggregate counts.
- Append and restore validation for command ids, sequences, admission invariants, retry source validity, player slot shape, command kind, and payload shape.

## Does Not Own

- Command admission policy before a command reaches the log.
- Command execution or session tick scheduling.
- Save codec text serialization of command log records.
- State hash computation beyond being one hashed input.

## Reads

- Incoming `CommandRecord` rows.
- Existing logged rows for duplicate command ids, retry source checks, and count aggregation.
- Command shape helpers from `runtime/command/Command.hpp`.

## Writes / Mutates

- Mutates `records_`, `nextSequence_`, and `epoch_`.
- Assigns the next sequence during successful append.
- Replaces records/sequence/epoch during successful restore.
- Does not mutate command admission inputs outside the returned stored copy.

## Calls Out To / Wires Out To

- `Session` appends admission results and uses append status.
- `CommandAdmission` reads `findById` for retry validation.
- Save/load restores command logs through `restoreForLoad`.
- Runtime summaries and state hash read records and counts.

## Called By / Entry Points

- `CommandLog::append(...)`
- `CommandLog::restoreForLoad(...)`
- `CommandLog::findById(...)`
- `CommandLog::counts(...)`
- `CommandLog::clear(...)`
- `CommandLog::reset(...)`

## Invariants

- Appended commands must have a valid command id, invalid incoming sequence, valid non-pending admission, and valid payload for their kind.
- Accepted commands must have no rejection reason; rejected commands must have a rejection reason.
- Retry commands must reference an earlier rejected command.
- Restore requires monotonic sequence order starting at one and next sequence exactly after the last restored record.
- AI move/attack/wait commands may use an invalid player slot; local invalid-slot commands may not.
- `Clear` reset clears records and advances epoch.

## Tests / Proof Commands

- `rg -n "CommandLog|CommandLogAppendStatus|CommandLogRestoreStatus|stateHashExclusionsAndCommandLogPolicy" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `session_state_tests`, `save_load_tests`, `command_admission_tests`, and command-focused tests cover log append/restore/retry behavior.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.*`
- `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`

## Update When

- Command log append/restore invariants, count categories, reset/epoch semantics, retry source policy, or command payload validation changes.

## Do Not Update When

- Only command admission rejection order, command execution, save file formatting, or app receipt copy changes.
