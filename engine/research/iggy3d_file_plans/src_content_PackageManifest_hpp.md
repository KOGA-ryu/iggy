# `src/content/PackageManifest.hpp`

Updated: 2026-06-20

Exact purpose: declare the in-memory representation of an `iggy3d` content package manifest.

## Build Position

- priority rank: 32
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Ownership

This file owns:

- package id
- schema version
- scenario list
- asset reference names as inert strings
- declared entities/objectives metadata

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

- manifest version
- package name
- scenario file paths
- required runtime version
- optional author/debug metadata

## Semantics

- manifest data is validated before session creation
- manifest does not own live entities
- asset references are names only until a renderer/content backend consumes them later

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(field count plus string bytes).
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

- `src/content/PackageManifest.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
