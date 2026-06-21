# `src/projection/debug/DebugProjection.hpp`

Updated: 2026-06-20

Exact purpose: declare read-only debug output for target/reach/camera/session diagnostics.

## Build Position

- priority rank: 106
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/projection/debug`
- file kind: `header`

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

Declare `enum class DebugProjectionKind : std::uint8_t` with at least
`TargetCandidate`, `ReachRadius`, `CommandRejected`, `ClockMode`, `CameraMode`,
`ObjectiveState`, `StateHash`, and `ReplayDivergence`.

Declare `DebugProjectionItem` with kind, source tick, command id, player slot,
actor entity, target entity, optional rejection reason, optional world point,
optional world bounds/radius, and short stable label code.

Declare `DebugProjectionResult` with `std::vector<DebugProjectionItem> items`
and source tick/hash. The source is `SessionState` plus read-only
`RuntimeEvent`/metrics inputs where needed.

Invariants:

- debug projection is a diagnostic view, not save truth and not replay input;
- labels are stable ids/codes, never localized prose required for logic;
- `CommandRejected` debug items preserve the exact rejection reason;
- debug categories emit in deterministic category order, then source tick/order;
- no renderer API, console logging, file IO, raw input, or mutation dependency.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/projection/debug/DebugProjection.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
