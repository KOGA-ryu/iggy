# `src/runtime/diagnostics/RuntimeMetrics.hpp`

Updated: 2026-06-20

Exact purpose: declare lightweight deterministic counters for runtime cost and acceptance reporting.

## Build Position

- priority rank: 97
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `header`

## Ownership

This file owns:

- tick count
- command counts
- entity scan counts
- target query counts
- save/replay counts

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

- counters are integers
- metrics can be reset per session/demo run
- no wall-clock timing in deterministic metrics

## Semantics

- metrics are diagnostic derived data, not gameplay truth
- metrics must not affect command admission or system execution

## Detailed Design Contract

Declare `RuntimeMetrics` as deterministic counters, not wall-clock telemetry.
Required counters: ticks run, submitted/accepted/rejected commands, retry
commands, movement executions, interaction executions, acquired items,
completed objectives, clock transitions, camera transitions, save creations,
load completions, reset completions, and replay completions.

Declare `RuntimeMetricsResetPolicy`, with first-build behavior `ClearOnReset`.

Invariants:

- counters are derived from command log, tick execution, and runtime events;
- acceptance summary command counts derive from `CommandLog`; metrics are
  corroborating diagnostics and never the source of command count truth;
- reset branch uses cleared metrics unless whole-process tool metrics live
  outside `SessionState`;
- metrics are excluded from save/hash in the first build;
- future per-slot counters derive in deterministic slot order.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) counter increments.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runtime metrics are transient diagnostics and are excluded from `SaveEnvelope`
  and `StateHash` in the first build;
- `CommandLog` owns acceptance summary command counts; metrics may corroborate
  but cannot override them;
- multiplayer counters must derive by deterministic slot order.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/diagnostics/RuntimeMetrics.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
