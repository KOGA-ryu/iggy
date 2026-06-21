# `src/runtime/diagnostics/RuntimeSummary.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic text summary generation for demos, acceptance fixtures, and debugging.

## Build Position

- priority rank: 109
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `header`

## Ownership

This file owns:

- summary field order
- formatting rules
- state/count extraction
- expected summary comparison support

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

- scenario id, outcome, final tick, player position, inventory, objective status, clock/camera mode, command counts, first rejection, state hash

## Semantics

- summary is derived read-only output
- same state produces byte-identical summary
- tests compare the full expected summary file once hash is locked

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(state size plus output bytes).
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

- `src/runtime/diagnostics/RuntimeSummary.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
