# `src/app/AppConfig.cpp`

Updated: 2026-06-20

Exact purpose: declare app-facing configuration produced by CLI parsing without leaking CLI concepts into runtime.

## Build Position

- priority rank: 29
- tier: Tier 2: Content And Configuration
- module: `app support`
- file kind: `source`

## Ownership

This file owns:

- fixture path
- save path
- replay path
- selected camera default
- verbosity
- validation mode

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- config/core helpers
- standard filesystem/string containers
- no gameplay mutation rules

## Data Contract

- paths are `std::filesystem::path` in the implementation file
- runtime config is embedded by value
- flags stay app-local

## Semantics

- invalid CLI data becomes diagnostics before runtime creation
- app config can select scenario but cannot bypass package validation

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(number of CLI arguments) during app startup, O(1) afterward.
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

- `src/app/AppConfig.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
