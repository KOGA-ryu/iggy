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

## Detailed Design Contract

Implementation algorithm:

1. validate slot count and per-slot queue sequence monotonicity;
2. flatten queued commands into a temporary vector;
3. stable-sort by requested tick, slot, and local sequence;
4. if command count exceeds request limit, return `CommandLimitExceeded` before
   submitting anything;
5. authorize each command in merged order;
6. authority failure returns `AuthorityRejected` with first failing source slot,
   local sequence, and `AuthorityDecisionStatus`;
7. submit allowed commands through `Session::submitCommand` on a candidate
   session/transaction;
8. non-`Ok` command append/session result returns `SessionSubmitFailed` with the
   exact append status from `SessionCommandResult`;
9. record submitted/accepted/rejected counts and merged command order only for a
   successful candidate;
10. commit the candidate session once after all submissions succeed;
11. return resulting state hash facts after the step.

Failure policy:

- invalid queue shape returns `InvalidQueue`;
- unknown or duplicate local slot returns `InvalidSlot`;
- non-monotonic per-slot local sequence returns `InvalidLocalSequence`;
- command-limit overflow returns `CommandLimitExceeded`;
- authority rejection returns `AuthorityRejected`;
- session append/submit failure returns `SessionSubmitFailed`;
- failed local steps leave the destination session unchanged and commit no
  command from that step.

The implementation must not open sockets, create threads, inspect raw devices,
or apply command effects directly. It is a deterministic harness for couch
co-op, four-player battle readiness, replay ordering, and future online packet
contracts.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(total queued commands log total queued commands) if sorted; O(total queued commands) if queues are preordered by slot/tick.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- queue merge state is transient and is excluded from `SaveEnvelope` and
  `StateHash`;
- each submitted command reaches durable replay truth only through `Session` and
  `CommandLog`;
- online networking can reuse the ordering policy later, but this source owns no
  sockets, packet retries, or remote authority state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/LocalMultiplayerSession.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
