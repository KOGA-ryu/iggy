# `CMakeLists.txt`

Updated: 2026-06-20

Exact purpose: define the standalone `iggy3d` CMake project, library, apps, fixtures, and test entry points without old `iggy` dependencies.

## Build Position

- priority rank: 5
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- project name/version
- C++ standard
- library target `iggy3d`
- app targets
- test enablement
- install/export shape if needed

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

- requires C++20 or later
- adds `src` include directory only for this repo
- includes local cmake helper files
- registers apps and tests when options allow

## Semantics

- configure must fail if an old iggy include path/target is required
- library sources are explicit and grouped by module
- fixtures are referenced by tests/apps as source tree data

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- Configure cost is O(number of source/test files); build cost is compiler driven.
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

- `CMakeLists.txt` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
