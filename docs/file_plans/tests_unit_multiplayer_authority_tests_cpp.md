# `tests/unit/multiplayer_authority_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove authority and local multiplayer ordering behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 131
- tier: Tier 7: Durability Replay Multiplayer
- module: `unit tests`
- file kind: `test`

## Ownership

This file owns:

- test scenarios
- assertions
- fixture setup helpers local to this test file
- regression coverage for documented semantics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d headers
- test framework selected by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- local player slot 0 accepted
- wrong slot rejected
- local multiplayer merge order deterministic
- replication packet codec round-trips

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests assert exact rejection/status codes when behavior is part of runtime contract

## Detailed Design Contract

Test cases required:

- local-only authority accepts player slot 0 and rejects nonzero slots before
  command admission;
- local multiplayer accepts four configured slots and rejects invalid slot 4;
- actor ownership mismatch returns `ActorNotControlledBySlot`;
- rejected authority commands are not appended to command log and do not mutate
  runtime state, `SaveEnvelope`, or `StateHash`;
- local command queues merge deterministically by tick, slot, sequence;
- local multiplayer step exposes exact `LocalMultiplayerStepStatus` values:
  `Ok`, `InvalidQueue`, `InvalidSlot`, `InvalidLocalSequence`,
  `AuthorityRejected`, `SessionSubmitFailed`, and `CommandLimitExceeded`;
- local multiplayer failure tests assert first failing source slot, local
  sequence, source authority status or append status, and all-or-nothing
  behavior with destination session unchanged and no command from the failed
  step committed;
- replication packet encode/decode roundtrips command submit, ack/reject, and
  state hash report values;
- replication codec emits byte-identical deterministic tagged text for identical
  packet values, starting with `iggy3d.replication_packet.v1`;
- `ReplicationEncodeResult` exposes exact codec status, packet validation
  status, encoded text, diagnostic field/offset, and command index fields;
- `ReplicationDecodeResult` exposes exact codec status, decoded packet on `Ok`,
  packet validation status for `InvalidPacket`, diagnostic field/line/offset,
  and command index fields;
- replication codec uses indexed command fields such as
  `command.0.commandId`, `command.0.kind`, and `command.0.sequence`;
- replication codec rejects duplicate keys, unsupported version, invalid
  enum/status names, invalid ids/sequences, duplicate command ids inside a
  packet, and `PacketTooLarge`;
- parse-level failures assert the exact `ReplicationCodecStatus`, including at
  least `MissingField` or `DuplicateKey`;
- packet-level validation failures assert codec status `InvalidPacket` plus the
  exact carried `ReplicationPacketStatus`, including at least duplicate command
  id or invalid retry source;
- replication codec rejects malformed `state.hash` values and ack/reject packets
  whose packet-level status disagrees with the single command record status as
  `InvalidPacket`;
- replication codec round-trips `command.N.commandId`, and retry source
  references are asserted against `CommandRecord::commandId` values rather than
  command sequences or command vector indexes;
- valid replication roundtrip asserts `commandId`, `sequence`,
  `retrySourceCommandId`, player slot, command kind, admission/rejection,
  `hasTargetEntity`, `targetEntity`, `hasTargetPoint`, and `targetPoint`;
- decoded command payloads are still passed through authority/admission before
  execution.

Tests must not use sockets, wall-clock timing, renderer/window/GPU, or old iggy
code. Assertions compare stable status enums and merged command order.

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime stays small; fixture-level tests may scan all demo entities and commands.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use exact authority, local-step, packet, and codec status values for expected
  failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- tests prove multiplayer authority and packet values without owning transport or
  save truth;
- decoded commands must still flow through normal authority/admission;
- state hash packet fields are compared as values only.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/multiplayer_authority_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
