# `src/runtime/save/SaveCodec.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic save envelope encoding and decoding.

## Build Position

- priority rank: 115
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `source`

## Ownership

This file owns:

- deterministic tagged text codec rules
- field ordering
- round-trip parse diagnostics
- escaping rules

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- first complete build uses a first-party deterministic tagged text format
- all sections written in stable order

## Semantics

- codec does not create gameplay effects
- decode returns envelope or diagnostics
- unknown keys are rejected in the first complete build

## Detailed Design Contract

Implementation algorithm:

1. write `iggy3d.save_envelope.v1` as the first line;
2. encode sections in exactly this order: metadata, session, world, players,
   clock, camera, commandLog, inventory, combat, ai, objectives;
3. encode repeated values with deterministic indexes, including
   `world.entity.0.*`, `players.slot.0.*`, `commandLog.record.0.*`,
   `inventory.player.0.stack.0.*`, `combat.combatant.0.*`, `ai.actor.0.*`, and
   `objectives.record.0.*`;
4. emit and parse fields within each section in the exact key order declared by
   `SaveCodec.hpp`; that header key-order list is the required source order for
   this implementation;
5. percent-escape string values for `%`, LF, CR, and `=`;
6. format numeric values with deterministic precision matching state/hash rules;
7. reject duplicate keys, missing required keys, unsupported version, invalid
   section order, invalid enum tokens, invalid numbers, invalid ids/sequences,
   malformed percent escapes, `SaveTooLarge`, and unknown keys during decode;
8. return a decoded `SaveEnvelope` value without touching `Session`;
9. leave saved payload hash comparison to `SaveLoad` because the first complete
   build codec does not own an internal checksum.

Failure diagnostics include section, field, line/offset when available, and
status. The codec must be suitable for app tools and unit tests without any
filesystem, renderer, network, or platform dependency.

Result construction:

- `encodeSaveEnvelope` returns `SaveEncodeResult`;
- on `Ok`, `encodedText` contains the full deterministic tagged text payload and
  `savedStateHash` mirrors `envelope.metadata.savedStateHash`;
- on non-`Ok`, `encodedText` is ignored and diagnostic section/key/line/offset
  identify the failure;
- `decodeSaveEnvelope` returns `SaveDecodeResult`;
- on `Ok`, `envelope` contains the complete decoded value;
- on non-`Ok`, `envelope` is ignored and diagnostic fields identify duplicate
  key, missing field, malformed number, invalid enum, invalid section order,
  size, or unknown-key context.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(save bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use `SaveCodecStatus` plus section/key/line context for expected failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- codec owns bytes-to-`SaveEnvelope` mapping only;
- save truth is the decoded `SaveEnvelope`, not the text buffer;
- replay uses decoded command records through normal replay/session APIs.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/save/SaveCodec.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
