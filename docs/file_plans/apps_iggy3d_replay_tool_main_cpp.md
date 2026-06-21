# `apps/iggy3d_replay_tool/main.cpp`

Updated: 2026-06-20

Exact purpose: replay a command log or save-contained command history and verify deterministic state hash.

## Build Position

- priority rank: 136
- tier: Tier 8: Product Proof And Tools
- module: `iggy3d_replay_tool`
- file kind: `app`

## Ownership

This file owns:

- replay CLI entry point
- baseline fixture selection
- command log loading
- hash comparison
- diagnostic output

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

- input save/replay path
- expected hash option
- uses CommandReplay and StateHash

## Semantics

- all commands go through normal admission/execution
- divergence exits nonzero and reports first mismatch
- tool owns no gameplay rules

## Detailed Design Contract

CLI ownership:

- `--fixture <path>` selects baseline package/scenario;
- `--save <path>` and `--command-log <path>` are both first-build supported
  replay input modes;
- `--expect-hash <hex>` compares final hash;
- `--expect-summary <path>` optionally compares formatted summary.

Execution flow:

1. load/validate baseline fixture;
2. load replay input through save/codec APIs or canonical fixture command log;
3. run `CommandReplay` with normal authority/admission/session execution;
4. compute final state hash and optional summary;
5. report first divergence with command id, sequence, tick, expected/actual
   admission/rejection/hash;
6. exit 0 only on replay match and expected hash/summary match when provided.

The tool owns input/output and process status only. It must not implement
gameplay rules, bypass rejected commands, create renderer state, open sockets, or
patch final state to match expected output.

## Implementation Plan

1. parse CLI arguments through `CliParser` into `AppConfig`;
2. load and validate requested package, fixture, save, or replay inputs through public library APIs;
3. execute the app-specific runtime/tool flow without owning gameplay rules;
4. print deterministic output and return nonzero on validation, runtime, save/load, replay, or summary failure.

## Compute Cost

- O(command count times session operation cost).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- the app owns replay CLI/process IO only;
- runtime replay APIs own command admission/execution and state hash comparison;
- the tool must report divergence without editing command logs or runtime state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `apps/iggy3d_replay_tool/main.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
