# `fixtures/demos/first_room/expected_summary.txt`

Updated: 2026-06-20

Exact purpose: store the byte-exact expected output of `iggy3d_headless_demo` for the complete runtime acceptance proof.

## Build Position

- priority rank: 132
- tier: Tier 8: Product Proof And Tools
- module: `first_room fixture`
- file kind: `fixture`

## Ownership

This file owns:

- summary field order
- expected final player state
- command counts
- first rejection reason
- clock/camera final mode
- state hash line

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

- scenario `iggy3d.first_room`
- outcome `DemoComplete`
- final player position `(2.000,0.000,1.000)`
- inventory contains `gold_key:1`
- first rejection `OutOfRange`
- final clock `Normal`
- final camera `ThirdPerson`

## Semantics

- after implementation first green locks the literal state hash value here
- acceptance compares full file contents, not loose substrings

## Implementation Plan

1. write deterministic fixture data in the documented schema;
2. keep entity order stable because ids derive from it;
3. avoid renderer-only or old iggy references;
4. make acceptance-sensitive values explicit.

## Compute Cost

- O(summary bytes) to compare.
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

- `fixtures/demos/first_room/expected_summary.txt` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
