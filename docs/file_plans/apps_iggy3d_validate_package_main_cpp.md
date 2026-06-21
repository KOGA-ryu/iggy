# `apps/iggy3d_validate_package/main.cpp`

Updated: 2026-06-20

Exact purpose: validate an `iggy3d` package manifest and scenario from the command line.

## Build Position

- priority rank: 135
- tier: Tier 8: Product Proof And Tools
- module: `iggy3d_validate_package`
- file kind: `app`

## Ownership

This file owns:

- validation CLI entry point
- diagnostic printing
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

- input path to package manifest or fixture directory
- uses PackageLoader and PackageValidator
- does not create active session unless scenario validation requires seed dry-run

## Semantics

- valid first-room fixture exits 0
- invalid package exits nonzero with structured diagnostic codes
- no renderer or old iggy dependency

## Detailed Design Contract

CLI ownership:

- accepts a package manifest path or fixture directory path;
- optional `--scenario <id>` narrows validation;
- validation diagnostics are deterministic text on stderr. The current build
  does not expose a structured machine-output flag; adding one requires an
  explicit CLI contract update.

Validation flow:

1. parse app config and normalize only the input path supplied by the user;
2. load package manifest and referenced scenario/content files;
3. validate stable ids, first-room scenario id, player spawn, `gold_key`,
   interactable reach metadata, objective wiring, camera defaults, and command
   script references used by acceptance;
4. optionally dry-create seed state without running gameplay;
5. print deterministic success/failure status and exit nonzero on any error.

The app owns file paths and process exit code only. It must not mutate package
files, create renderer state, run gameplay ticks except explicit dry validation,
or import old iggy schemas.

## Implementation Plan

1. parse CLI arguments through `CliParser` into `AppConfig`;
2. load and validate only the requested package manifest, fixture directory, and
   referenced scenario/content files through public library APIs;
3. optionally dry-create seed state only when package validation requires it;
4. print deterministic validation output and return nonzero only on package,
   fixture, scenario, or dry-seed validation failure.

## Compute Cost

- O(package and scenario bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- the app owns package-validation CLI/process IO only;
- it does not run save/load or replay proof;
- any dry-created seed state is validation input and not gameplay mutation.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `apps/iggy3d_validate_package/main.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
