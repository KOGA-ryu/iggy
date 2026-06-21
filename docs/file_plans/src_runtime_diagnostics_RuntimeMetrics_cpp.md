# `src/runtime/diagnostics/RuntimeMetrics.cpp`

Updated: 2026-06-20

Exact purpose: implement lightweight deterministic counters for runtime cost and acceptance reporting.

## Build Position

- priority rank: 98
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `source`

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

Implement pure helpers declared by `RuntimeMetrics.hpp`: construct empty
metrics, record a runtime event, record a command record, combine two metric
snapshots, and reset metrics by policy.

Rules:

- document one canonical source for each counter so event recording and
  command-log scans do not double-count;
- first build derives acceptance command counts from `CommandLog` for
  summary and use metrics for tick/event proof counters;
- no helper reads wall-clock time, files, globals, or unordered containers;
- required current event kinds must be handled explicitly.

Tests must cover movement twice, interaction once, objective once, and
save/load/reset/replay proof counters once for the first-room flow.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(1) counter increments.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runtime metrics are transient diagnostics and are excluded from `SaveEnvelope`
  and `StateHash`;
- command count summary truth comes from `CommandLog`; metrics are
  corroborating counters only;
- replay may record metrics during proof, but replay matching is based on
  admission results, durable state, and state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/diagnostics/RuntimeMetrics.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
