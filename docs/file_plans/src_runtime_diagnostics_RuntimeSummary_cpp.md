# `src/runtime/diagnostics/RuntimeSummary.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic text summary generation for demos, acceptance fixtures, and debugging.

## Build Position

- priority rank: 110
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `source`

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

- scenario id, lifecycle, outcome, final tick, player position, inventory,
  objective status, key/marker active flags, clock/camera mode, previous realtime
  camera, command counts, first rejection, retry command-id linkage, state hash

## Semantics

- summary is derived read-only output
- same state produces byte-identical summary
- tests compare the full expected summary file once hash is locked

## Detailed Design Contract

Implement:

- `RuntimeSummary buildRuntimeSummary(const RuntimeSummaryInput& input)`;
- `std::string formatRuntimeSummary(const RuntimeSummary& summary)`;
- an optional pure comparison helper returning first differing line when an
  expected summary string is provided by an app/test layer.

Formatting rules:

- exactly one `key=value` field per line in the order declared by the header;
- terminate output with a single trailing newline;
- do not emit extra diagnostics on stdout in non-verbose acceptance mode;
- derive command counters from `CommandLog` facts, not from `RuntimeMetrics` or
  formatted text;
- format only the first-build summary command fields: `commands.submitted`,
  `commands.accepted`, `commands.rejected`, and `commands.retry`;
- format visible retry linkage fields immediately after `first_rejection`:
  `retry.original_rejected_command_id`, `retry.retry_command_id`,
  `retry.sourceCommandId`, `retry.retrySourceCommandId`,
  `retry.executed.command_id`, and `retry.executed.sequence`;
- derive retry linkage from command-log labels/records and the effective retry
  execution result, not from ad hoc labels or runtime metrics;
- do not format control or wait counts in the acceptance summary; those remain
  derivable diagnostics from command records;
- metrics may be read only to corroborate diagnostics and must not override
  command-log counts;
- derive `first_rejection` from the first rejected command and exact rejection
  reason;
- never recompute gameplay while formatting.

Failure behavior: missing required input produces a structured diagnostic or
`RuntimeProofStatus::Fail`; it must not crash, read files, mutate session state,
or infer success from partial data.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(state size plus output bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- formatted summary text is derived output and is excluded from `SaveEnvelope`
  and `StateHash`;
- summary generation must not mutate replay, save/load, command log, or runtime
  metrics state;
- replay proof is represented by caller-provided status/hash facts.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/diagnostics/RuntimeSummary.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
