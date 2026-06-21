# `cmake/iggy3d_options.cmake`

Updated: 2026-06-20

Exact purpose: declare build options for tests, warnings, sanitizers, tools, and future renderer toggles.

## Build Position

- priority rank: 6
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- `IGGY3D_BUILD_TESTS`
- `IGGY3D_BUILD_TOOLS`
- `IGGY3D_WARNINGS_AS_ERRORS`
- optional sanitizer flags

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- CMake only
- owned source/test paths in `/Users/kogaryu/iggy3d`
- no old iggy targets or include directories

## Data Contract

- defaults favor internal demo development: tests and tools on in dev builds
- renderer option is absent or off until runtime acceptance is green

## Semantics

- options do not import old iggy targets
- option names use `IGGY3D_` prefix only

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- O(option count) at configure time.
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

- `cmake/iggy3d_options.cmake` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
