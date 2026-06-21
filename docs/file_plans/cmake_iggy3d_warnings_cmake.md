# `cmake/iggy3d_warnings.cmake`

Updated: 2026-06-20

Exact purpose: centralize compiler warning policy for `iggy3d` targets.

## Build Position

- priority rank: 7
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- warning flags by compiler
- warnings-as-errors application
- local helper function for targets

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

- MSVC/Clang/GCC branches if cross-platform is kept
- no flags applied to external dependencies unless explicitly vendored

## Semantics

- warnings policy is strict for owned code
- builders must not silence warnings by weakening source contracts

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- O(number of targets that call `iggy3d_apply_warnings`) at configure time.

## Diagnostics And Errors

- calling `iggy3d_apply_warnings` with a missing target must fail configure with
  `message(FATAL_ERROR ...)`;
- unsupported compilers receive no extra warning flags and emit one CMake
  `STATUS` line naming the compiler id;
- runtime `Diagnostic` types are not used by this CMake helper.

## Save Replay Multiplayer Notes

- warning flags are build policy only and are not save truth.
- warning policy must not change source ownership, command replay, runtime
  state, networking, or renderer dependencies.

## Tests And Verification

- configure succeeds for AppleClang/Clang/GNU/MSVC policies;
- generated compile commands for owned targets contain only target-local warning
  flags;
- no external dependency target receives warning flags from this helper.

## Completion Criteria

- `cmake/iggy3d_warnings.cmake` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Purpose
Provide one function for applying local warning policy to an `iggy3d` target without changing dependency shape.

### Required API
Define this CMake function:

```cmake
iggy3d_apply_warnings(target_name)
```

### Warning Policy
For MSVC, apply exactly:
- `/W4`
- `/WX` only when `IGGY3D_WARNINGS_AS_ERRORS` is `ON`.

For Clang/GCC, apply exactly:
- `-Wall`
- `-Wextra`
- `-Wpedantic`
- `-Wconversion`
- `-Werror` only when `IGGY3D_WARNINGS_AS_ERRORS` is `ON`.

`IGGY3D_WARNINGS_AS_ERRORS` gates error promotion.

### Forbidden Behavior
- Do not add include paths.
- Do not link libraries.
- Do not set global compiler flags outside the target.
- Do not special-case old iggy files.

### Completion Criteria
Every production, tool, and test target can opt into the same warning helper without changing gameplay semantics.
