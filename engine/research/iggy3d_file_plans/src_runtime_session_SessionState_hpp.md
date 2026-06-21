# `src/runtime/session/SessionState.hpp`

Updated: 2026-06-20

Exact purpose: declare the aggregate runtime truth for an active `iggy3d` session.

## Build Position

- priority rank: 63
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/session`
- file kind: `header`

## Ownership

This file owns:

- lifecycle enum
- world state
- player roster
- clock state
- camera state
- command log
- subsystem states
- baseline reset seed
- diagnostic/event buffers

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

- lifecycle loading, playing, paused, complete, failed
- world/player/clock/camera by value
- inventory/combat/ai/objective state by value
- package/scenario ids

## Semantics

- this is save truth except derived diagnostics/projection buffers
- session reset restores baseline seed and clears transient events
- session state has no renderer objects

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(total owned state).
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

- `src/runtime/session/SessionState.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
