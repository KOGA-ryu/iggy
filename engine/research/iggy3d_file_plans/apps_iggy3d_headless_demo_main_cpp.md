# `apps/iggy3d_headless_demo/main.cpp`

Updated: 2026-06-20

Exact purpose: run the complete first-room gameplay loop headlessly and emit deterministic summary output.

## Build Position

- priority rank: 134
- tier: Tier 8: Product Proof And Tools
- module: `iggy3d_headless_demo`
- file kind: `app`

## Ownership

This file owns:

- demo CLI entry point
- fixture load
- scripted command sequence
- save/load/replay proof calls
- summary printing
- process exit code

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d library headers
- app support helpers
- standard library
- no renderer or old iggy dependency

## Data Contract

- uses `AppConfig` and `CliParser`
- default fixture `fixtures/demos/first_room`
- script commands from `docs/acceptance_demo.md`

## Semantics

- does not create renderer/window
- exits nonzero on validation, replay, save/load, or summary mismatch failure
- prints only deterministic summary unless verbose diagnostics requested

## Implementation Plan

1. parse CLI arguments through `CliParser` into `AppConfig`;
2. load and validate requested package, fixture, save, or replay inputs through public library APIs;
3. execute the app-specific runtime/tool flow without owning gameplay rules;
4. print deterministic output and return nonzero on validation, runtime, save/load, replay, or summary failure.

## Compute Cost

- O(fixture bytes plus scripted command/tick count plus save/replay cost).
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

- `apps/iggy3d_headless_demo/main.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
