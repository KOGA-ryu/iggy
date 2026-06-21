# `src/config/RuntimeConfig.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic runtime defaults shared by sessions, tests, and tools.

## Build Position

- priority rank: 27
- tier: Tier 2: Content And Configuration
- module: `config`
- file kind: `header`

## Ownership

This file owns:

- tick rate and fixed step defaults
- interaction range defaults
- movement limits
- camera defaults
- save compatibility constants

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core value headers only if needed
- no app CLI or runtime mutation dependencies

## Data Contract

- `fixedTickHz` default 20
- `normalTimeScale` 1.0
- `slowTimeScale` 0.15
- `interactionRangeMeters` 1.50
- `movementMetersPerCommand` 3.00
- `defaultRealtimeCamera` close third-person

## Semantics

- values are plain data with constexpr defaults
- runtime code receives a copy at session creation
- tests may override values explicitly

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) read-only value access.
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

- `src/config/RuntimeConfig.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
