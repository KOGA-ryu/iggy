# `src/runtime/multiplayer/LocalMultiplayerSession.cpp`

Updated: 2026-06-20

Exact purpose: implement local deterministic coordination for multiple player slots before real networking exists.

## Build Position

- priority rank: 130
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `source`

## Ownership

This file owns:

- per-slot command queues
- stable merge ordering
- authority handoff to `Authority` and `Session`

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

- commands grouped by source slot
- merged by tick then slot then sequence
- result summary per slot

## Semantics

- no network sockets
- does not bypass session admission
- models future multiplayer ordering in local tests

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(total queued commands log total queued commands) if sorted; O(total queued commands) if queues are preordered by slot/tick.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use `Diagnostic` or stable status/rejection values for expected failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- if this file owns save truth, it must define exact fields included in `SaveEnvelope`;
- if this file owns derived data, it must be regenerable and excluded from save truth;
- if this file affects commands, replay must reproduce the same result and state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/LocalMultiplayerSession.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
