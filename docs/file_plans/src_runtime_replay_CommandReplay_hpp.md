# `src/runtime/replay/CommandReplay.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic replay from baseline fixture plus command log.

## Build Position

- priority rank: 121
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/replay`
- file kind: `header`

## Ownership

This file owns:

- baseline session creation handoff
- command replay loop
- hash comparison
- replay diagnostics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- accepted and rejected command records
- expected final state hash
- replay result status

## Semantics

- replay submits commands through normal session admission/execution
- rejected commands must reject for the same reason
- divergence reports first mismatching command/tick

## Detailed Design Contract

Declare `enum class CommandReplayStatus : std::uint8_t` with `Matched`,
`AdmissionDiverged`, `RejectionReasonDiverged`, `StateHashDiverged`,
`SummaryDiverged`, `CommandMissing`, `ExecutionFailed`, and
`InvalidBaseline`.

Declare `CommandReplayRequest` with baseline session creation data, command log
records, required expected final hash, and required expected final summary
text/fields for acceptance replay. Lower-level hash-only helper APIs are
permitted only outside the acceptance `CommandReplayRequest` path.

Declare `CommandReplayResult` with status, first mismatching command id/sequence,
expected/actual admission status, expected/actual rejection reason,
expected/actual hash, expected/actual summary, final tick, and diagnostics.
When status is `SummaryDiverged`, expected/actual summary fields are populated
and command id/sequence/tick are populated when a specific command context is
available.

Invariants:

- replay creates a fresh session from the same package/scenario baseline;
- every command is re-submitted through normal authority/admission/session APIs;
- replay input contains only command records that passed authority and reached
  command admission/logging in the source run;
- accepted commands must accept and execute through `SessionTick`;
- rejected commands must reject for the same reason and must not mutate state;
- replay never copies final state, inventories, objectives, or hash from source;
- acceptance replay compares the generated final summary to the expected summary
  after final hash comparison;
- replay has no renderer, projection, socket, raw input, or wall-clock dependency.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(command count times session operation cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- replay failures use `CommandReplayStatus` and the declared
  `CommandReplayResult` fields.
- expected/actual command id, sequence, admission, rejection, hash, summary,
  final tick, and diagnostic text are the only replay failure detail fields in
  this header.
- logging text is never the only machine-readable outcome.

## Save Replay Multiplayer Notes

- replay request data is proof input, not save truth;
- replay consumes `CommandLog` records and normal session APIs, then compares
  final `StateHash`;
- replay diagnostics are derived and excluded from `SaveEnvelope`.
- authority rejections are outside replay input for the first complete build
  because they are not durable command-log records.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/replay/CommandReplay.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
