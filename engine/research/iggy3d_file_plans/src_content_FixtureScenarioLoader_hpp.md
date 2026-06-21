# `src/content/FixtureScenarioLoader.hpp`

Updated: 2026-06-20

Exact purpose: load the first-room scenario fixture into deterministic seed data for `Session` creation.

## Build Position

- priority rank: 39
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Ownership

This file owns:

- scenario file parse
- entity seed records
- objective seed records
- player spawn binding

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core diagnostics/result/math/id helpers as needed
- standard filesystem/string containers
- no active runtime session dependency

## Data Contract

- player at `(0,0,0)`
- gold key at `(3,0,0)`
- tactical marker at `(2,0,1)`
- objective `collect_gold_key`

## Semantics

- fixture order defines deterministic entity id allocation
- scenario loading ends before runtime ticking starts
- loader reports diagnostics instead of partially mutating a session

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(scenario file bytes plus entity count).
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

- `src/content/FixtureScenarioLoader.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
