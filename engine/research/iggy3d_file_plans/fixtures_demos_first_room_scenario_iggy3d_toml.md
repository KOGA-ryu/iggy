# `fixtures/demos/first_room/scenario.iggy3d.toml`

Updated: 2026-06-20

Exact purpose: define the deterministic first-room scenario used by unit, acceptance, replay, and headless demo tools.

## Build Position

- priority rank: 38
- tier: Tier 2: Content And Configuration
- module: `first_room fixture`
- file kind: `fixture`

## Ownership

This file owns:

- seed entity order
- player spawn
- gold key target
- tactical marker
- objective seed
- interaction/reach parameters

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- documented first-party fixture schema only
- no old iggy asset/schema dependency

## Data Contract

- entity 1 local player at `(0,0,0)`
- entity 2 `gold_key` pickup at `(3,0,0)` with bounds and item id
- entity 3 `tactical_marker_alpha` at `(2,0,1)`
- objective `collect_gold_key` active at session start

## Semantics

- fixture order defines stable ids
- initial key is outside 1.50m reach
- moving to `(2,0,0)` puts player in reach
- no renderer assets required

## Implementation Plan

1. write deterministic fixture data in the documented schema;
2. keep entity order stable because ids derive from it;
3. avoid renderer-only or old iggy references;
4. make acceptance-sensitive values explicit.

## Compute Cost

- O(file bytes plus seed entity count).
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

- `fixtures/demos/first_room/scenario.iggy3d.toml` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
