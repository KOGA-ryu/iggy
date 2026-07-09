# Runtime Diagnostics

File:

- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeEvent.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeMetrics.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/diagnostics/RuntimeSummary.cpp`

Verified at: `2b549d47`

## Owns

- Runtime transient event packet shape: `RuntimeEventKind` and `RuntimeEvent`.
- Runtime aggregate metric packet shape: `RuntimeMetrics`.
- Runtime summary/proof packet shape and formatted summary text.
- Stable summary extraction for selected session, command log, inventory, objective, combat, camera, clock, retry, and state hash facts.

## Does Not Own

- Producing session events during ticks.
- Updating metrics counters during gameplay.
- State hash computation.
- Save/load proof execution.
- App receipt or renderer presentation.

## Reads

- `SessionState` identity, lifecycle, outcome, clock, camera, world, inventory, objectives, combat, command log, and current state hash.
- Optional proof statuses and retry execution ids/sequences from `RuntimeSummaryInput`.
- `CommandLogCounts` and selected command records for first rejection, accepted retry, and last accepted attack.

## Writes / Mutates

- Does not mutate session state.
- Returns `RuntimeSummary` and formatted text.
- Missing input state returns a summary with save-roundtrip failure and passed-through proof statuses.

## Calls Out To / Wires Out To

- Calls `CommandLog::counts`.
- Calls world/entity lookup helpers and `formatStateHash`.
- `CommandReplay` uses summary build/format for replay comparison.
- `SessionTick` produces `RuntimeEvent` rows and updates transient metrics.

## Called By / Entry Points

- `buildRuntimeSummary(...)`
- `formatRuntimeSummary(...)`
- Session transient state stores `RuntimeEvent` rows and `RuntimeMetrics`.

## Invariants

- Diagnostics are observability/proof packets; they are not simulation authority.
- Summary formatting order is a comparison contract for replay/golden-style checks.
- Summary state hash is formatted from `SessionState::currentStateHash`.
- Command counts come from the command log rather than recomputing unrelated session facts.
- Transient events/metrics are cleared by session transient lifecycle, not persisted as authoritative state.

## Tests / Proof Commands

- `rg -n "RuntimeEvent|RuntimeMetrics|buildRuntimeSummary|formatRuntimeSummary|runtime_debug_snapshot_tests|render_replay_invariance_tests" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `runtime_debug_snapshot_tests`, `session_state_tests`, and `render_replay_invariance_tests` cover diagnostics consumers and summary behavior.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.*`
- `/Users/kogaryu/iggy3d/src/runtime/replay/CommandReplay.*`
- `/Users/kogaryu/iggy3d/src/runtime/replay/StateHash.*`
- `/Users/kogaryu/iggy3d/src/runtime/debug/RuntimeDebugSnapshot.*`

## Update When

- Runtime event kinds, metric fields, summary fields, summary formatting, proof status semantics, or summary source facts change.

## Do Not Update When

- Only app receipt names, renderer debug views, or internal session execution changes without changing diagnostics packet contracts.
