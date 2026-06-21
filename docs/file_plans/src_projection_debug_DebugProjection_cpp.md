# `src/projection/debug/DebugProjection.cpp`

Updated: 2026-06-20

Exact purpose: implement read-only debug output for target/reach/camera/session diagnostics.

## Build Position

- priority rank: 107
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/projection/debug`
- file kind: `source`

## Ownership

This file owns:

- debug lines/markers/text records
- reach radius visualization data
- target query result projection

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

- debug records reference entity ids and points
- no UI widget ownership
- stable ordering for tests

## Semantics

- debug projection explains runtime state but cannot change it
- headless tools can print or ignore it

## Detailed Design Contract

Implementation algorithm:

1. read current clock/camera/objective/hash facts from `SessionState`;
2. append command rejection markers from transient runtime events in append
   order;
3. append target/reach markers for current interactable candidates if available
   from runtime truth;
4. append objective, clock, camera, and hash markers in fixed category order;
5. append replay divergence markers only from explicit replay proof input;
6. sort only where source collections are not already stable; never sort by
   pointer address or unordered-container iteration.

The implementation must not create gameplay events, alter metrics, admit
commands, change pause/tactical state, or update save/replay data. After a load,
debug projection is rebuilt from the loaded state and current transient proof
events; absence of prior transient events is acceptable and deterministic.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(entity count plus event count).
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

- `src/projection/debug/DebugProjection.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
