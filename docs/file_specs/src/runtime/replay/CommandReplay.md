# Command Replay

File:

- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.cpp`

Verified at: `2b549d47`

## Owns

- Runtime command replay request/result/status contract.
- Replaying a source command log from a `SessionCreateRequest` baseline.
- Admission, rejection reason, command identity, state hash, runtime summary, and execution/finalization divergence reporting.

## Does Not Own

- Command log storage or append validation.
- Command admission policy.
- Session tick implementation.
- State hash field selection.
- Runtime summary field formatting rules.

## Reads

- Baseline session creation request.
- Source `CommandRecord` list with ordered command ids/sequences/admission outcomes.
- Expected final state hash and optional expected runtime summary text.
- Proof status inputs for save roundtrip, reset baseline, and replay hash.

## Writes / Mutates

- Creates and mutates a local replay `Session`.
- Returns replay result packets; does not mutate caller-owned session state.

## Calls Out To / Wires Out To

- Calls `Session::create(...)`.
- Calls `Session::submitCommand(...)` with a pending proposal derived from each source command.
- Calls `runSession(...)` for queued accepted commands.
- Calls `finalizeDemoIfComplete(...)` when requested.
- Calls `buildRuntimeSummary`, `formatRuntimeSummary`, and `Session::stateHash`.

## Called By / Entry Points

- `replayCommands(...)`

## Invariants

- Source commands must be non-empty and sequence-ordered from one.
- Replay strips command id, sequence, tick, admission, and rejection from proposals before submitting.
- Replayed command identity must match the expected logged command id and sequence.
- Rejected commands compare admission and rejection reason, then continue without ticking.
- Immediate session-control commands must execute immediately.
- Queued accepted commands must advance through `runSession`.
- Optional expected final hash and summary text turn mismatches into explicit divergence statuses.

## Tests / Proof Commands

- `rg -n "replayCommands|CommandReplayStatus|render_replay_invariance_tests|RuntimeSummary" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `render_replay_invariance_tests` exercises replay/summary/hash invariance paths.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionRunner.*`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.*`
- `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.*`

## Update When

- Replay proposal construction, accepted-command execution policy, finalization policy, divergence statuses, hash checks, or summary checks change.

## Do Not Update When

- Only command log append validation, session internals, or summary field formatting changes without replay behavior changes.
