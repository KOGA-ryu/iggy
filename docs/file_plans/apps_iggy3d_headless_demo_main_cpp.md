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

## Detailed Design Contract

CLI ownership:

- `--fixture <path>` defaults to `fixtures/demos/first_room`;
- `--expect-summary <path>` optionally compares exact output;
- `--verbose` allows diagnostics on stderr only;
- stdout in normal mode is only the formatted runtime summary.

Execution flow:

1. load and validate package/scenario;
2. create a session from first-room seed data;
3. submit `cmd_interact_oob` and prove `OutOfRange`;
4. move player into reach and retry interaction successfully;
5. prove inventory/objective/gold key active state;
6. enter tactical slow-time camera, then return to normal third person;
7. pause, step one tick, and resume;
8. save, encode/decode, and load into a fresh session;
9. reset branch to baseline using command-log reset policy `Clear`;
10. replay command log through normal admission/session path;
11. build summary and compare expected summary when requested.

Exit code is 0 only when validation, runtime script, save/load, reset, replay,
and optional summary comparison all pass. No renderer/window/native graphics path
is created by this app.

## Implementation Plan

1. parse CLI arguments through `CliParser` into `AppConfig`;
2. load and validate requested package, fixture, save, or replay inputs through public library APIs;
3. execute the app-specific runtime/tool flow without owning gameplay rules;
4. print deterministic output and return nonzero on validation, runtime, save/load, replay, or summary failure.

## Compute Cost

- O(fixture bytes plus scripted command/tick count plus save/replay cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- the app owns CLI/process IO and orchestration only;
- runtime library APIs own gameplay, save/load, replay, and hash truth;
- app diagnostics must report mismatches without patching runtime state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `apps/iggy3d_headless_demo/main.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
