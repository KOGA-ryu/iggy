# `src/projection/scene/SceneProjection.cpp`

Updated: 2026-06-20

Exact purpose: implement read-only projection from `SessionState` to renderer-facing scene items and camera facts.

## Build Position

- priority rank: 105
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/projection/scene`
- file kind: `source`

## Ownership

This file owns:

- runtime-to-scene conversion
- stable item order
- camera projection handoff values

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core values
- runtime state read-only headers
- no app, renderer API, or mutation dependencies

## Data Contract

- one item per active renderable entity
- camera mode/target facts
- optional objective/interaction highlighting flags

## Semantics

- projection never mutates runtime
- projection has no GPU/API dependency
- old renderer bridge is forbidden

## Detailed Design Contract

Implementation algorithm:

1. reserve output capacity from known world object counts;
2. emit player scene item(s) in stable player slot order;
3. scan world entities in stable entity id order;
4. skip inactive entities unless `includeInactive` is true;
5. map runtime entity categories to `SceneItemKind`;
6. copy transform, bounds, ids, active/visible state, and inert asset reference;
7. append objective and tactical markers in deterministic id order when enabled;
8. compute counts from emitted items;
9. copy source tick and current state hash without recomputing gameplay.

Failure behavior: invalid optional references are omitted deterministically unless
the public result type carries a diagnostic. The implementation must never
repair runtime state from projection.

Cost is O(projectable runtime objects plus markers). It allocates only the
result vector and performs no filesystem, renderer, network, or wall-clock work.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(entity count).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
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

- `src/projection/scene/SceneProjection.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
