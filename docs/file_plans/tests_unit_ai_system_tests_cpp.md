# `tests/unit/ai_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove deterministic AI proposal generation behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 91
- tier: Tier 5: Playable Gameplay Systems
- module: `unit tests`
- file kind: `test`

## Ownership

This file owns:

- test scenarios
- assertions
- fixture setup helpers local to this test file
- regression coverage for documented semantics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d headers
- test framework chosen by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- AI scan order stable
- dormant policy emits no proposal commands
- no direct world mutation
- same state produces same empty proposal result

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime should stay small; fixture-level tests may scan all demo entities and commands.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
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

- `tests/unit/ai_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/ai_system_tests.cpp
```

Required tests:

- `empty_ai_state_produces_no_proposals`: first-room compatible.
- `disabled_ai_actor_is_skipped`.
- `not_due_decision_tick_is_skipped`.
- `eligible_ai_actor_scanned_in_stable_order`: multiple records preserve vector
  order and increment `actorsConsidered` for enabled due actors.
- `policy_zero_produces_no_proposal`: enabled due actor with
  `deterministicPolicy == 0` returns `AiSystemStatus::Ok` and no proposals.
- `reserved_nonzero_policy_produces_no_proposal`: nonzero policy ids return no
  proposals and `AiSystemStatus::Ok` in the complete build.
- `missing_world_or_player_context_returns_invalid_world`: result status is
  `AiSystemStatus::InvalidWorld`, proposals are empty, actors considered is zero,
  and no state mutates.
- `invalid_ai_state_returns_invalid_state`: duplicate actor records, invalid
  entity ids, or invalid negative tick data return `AiSystemStatus::InvalidState`
  with no proposals and no mutation.
- `same_state_produces_same_empty_result`: repeat run from same state gives the
  same `Ok` status, empty proposals, and same `actorsConsidered`.
- `no_proposal_is_pending_or_admitted`: AI system does not call admission and
  does not create pending commands in the first complete build.
- `ai_does_not_mutate_world_command_log_or_ai_state`: dormant scan mutates no
  runtime state.
- `no_random_or_wall_clock_dependency`: test design must not depend on time.

Acceptance linkage:

- First-room acceptance does not require AI proposals, but `SessionState`
  includes `AiState`; these tests prove empty AI is deterministic and harmless.
