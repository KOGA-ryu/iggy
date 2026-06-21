# `src/runtime/session/SessionRunner.hpp`

Updated: 2026-06-20

Exact purpose: declare multi-tick session advancement for headless demo, tests, replay, and future app loops.

## Build Position

- priority rank: 101
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/session`
- file kind: `header`

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

Declare `SessionRunnerStatus`: `Idle`, `Advanced`, `Complete`, `Failed`,
`MaxTicksExceeded`.

Declare `SessionRunnerRunRequest` with `Session* session`, max ticks,
`stopWhenIdle`, and `stopWhenComplete`.

Declare `SessionRunnerRunResult` with status, ticks attempted, ticks advanced,
final lifecycle/outcome, and final hash.

Responsibilities:

- call `Session::tick()` or `SessionTick` through `Session`;
- respect paused clock by not advancing automatic ticks;
- expose `stepPausedOnce(Session&)` for `StepTacticalTick`;
- never submit commands directly unless an app/test scripted helper does it;
- never own command legality, save codec, replay, projection, or app output.

Acceptance use: process each accepted gameplay command until idle, prove paused
automatic run advances zero ticks, prove step advances one tick, and fail hard
on max-tick exhaustion.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(tick count times SessionTick cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runner requests/results are transient control values and are excluded from
  `SaveEnvelope` and `StateHash`;
- runner advances state only through `Session`/`SessionTick`, so replay can
  reproduce the same tick and hash results;
- pause/step proof must leave clock paused after exactly one forced step.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/session/SessionRunner.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
