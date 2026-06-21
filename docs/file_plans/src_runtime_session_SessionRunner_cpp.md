# `src/runtime/session/SessionRunner.cpp`

Updated: 2026-06-20

Exact purpose: implement multi-tick session advancement for headless demo, tests, replay, and future app loops.

## Build Position

- priority rank: 102
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/session`
- file kind: `source`

## Ownership

This file owns:

- run-until-idle helpers
- fixed tick stepping
- pause/step behavior
- acceptance script operation sequencing

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

- runner config max ticks
- stop conditions
- summary result
- diagnostics

## Semantics

- runner is deterministic for identical command streams
- runner does not own gameplay rules
- runner cannot bypass session admission

## Detailed Design Contract

Implement `SessionRunner` as a deterministic loop:

1. validate non-null session;
2. stop on `Complete`/`Failed` when configured;
3. return `Idle` without ticking when clock is paused;
4. call `session.tick()`;
5. count only ticks that actually step;
6. stop when idle if configured;
7. return `MaxTicksExceeded` if no stop condition is reached.

`stepPausedOnce` validates paused state, calls the one-step path once, returns
tick/hash facts, and leaves clock paused.

Failure behavior: no wall-clock sleeps, no busy loop on paused sessions, no
mutation outside `Session` APIs, and max-tick diagnostics include current
tick/hash.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(tick count times SessionTick cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runner loop state is transient and is excluded from `SaveEnvelope` and
  `StateHash`;
- all durable mutation happens through `Session::tick()`/`SessionTick`;
- replay uses the same tick/step behavior rather than a replay-only shortcut.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/session/SessionRunner.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
