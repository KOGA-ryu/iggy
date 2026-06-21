# `src/core/result/Result.hpp`

Updated: 2026-06-20

Exact purpose: declare the small value-based success/failure carrier used where exceptions would hide deterministic control flow.

## Build Position

- priority rank: 9
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

## Ownership

This file owns:

- status enum
- optional diagnostic payload conventions
- helper constructors

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- C++ standard library only unless explicitly justified
- no `src/runtime`, `src/content`, `src/projection`, or app includes

## Data Contract

- `StatusCode` values for ok, invalid argument, not found, rejected, incompatible, parse error, internal error
- `Result<T>` or equivalent value carrier
- `Result<void>` support if implemented

## Semantics

- no throwing for expected runtime failures
- callers must inspect failure before mutation
- diagnostic messages are stable enough for tests when part of acceptance

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) construction and move for small values; O(message bytes) only when carrying text.
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

- `src/core/result/Result.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
