# `src/runtime/replay/CommandReplay.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic replay from baseline fixture plus command log.

## Build Position

- priority rank: 122
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/replay`
- file kind: `source`

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

Implementation algorithm:

1. build a fresh `Session` from the request baseline;
2. iterate command records by log sequence;
3. submit each command through the same public path used by the original
   runtime;
4. compare accepted/rejected admission result to the source record;
5. compare rejection reason when rejected;
6. tick/step through normal session execution for accepted commands;
7. compute final state hash and compare to expected hash;
8. build final runtime summary and compare to the required expected summary for
   the acceptance replay path;
9. return first divergence with command id, sequence, tick, hash facts, and
   summary facts when applicable.

The implementation must not patch state to match the source. It must not skip
gameplay command-log records such as tactical slow-time camera, pause, step,
resume, wait, or retry. Save, load, reset, and replay proof phases are dedicated
proof APIs in the first complete build and are not command records in the
acceptance replay input.

Status mapping:

- admission mismatch returns `AdmissionDiverged`;
- rejection-reason mismatch returns `RejectionReasonDiverged`;
- execution failure returns `ExecutionFailed`;
- final hash mismatch returns `StateHashDiverged`;
- final summary mismatch after a matching hash returns `SummaryDiverged`;
- a missing command record or invalid replay input command identity returns
  `CommandMissing`;
- baseline creation failure returns `InvalidBaseline`;
- matching hash and summary return `Matched`.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(command count times session operation cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- replay reconstructs state only by baseline creation plus normal command
  submission/execution;
- replay diagnostics and divergence details are excluded from `SaveEnvelope`;
- multiplayer readiness depends on replay preserving player slot, command id,
  sequence, and retry source command id from `CommandLog` records.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/replay/CommandReplay.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
